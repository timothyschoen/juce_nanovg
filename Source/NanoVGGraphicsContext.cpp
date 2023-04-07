//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com> and Timothy Schoen <timschoen123@gmail.com>
//

#include "NanoVGGraphicsContext.h"
#include <BinaryData.h>

//==============================================================================

const static auto allPrintableAsciiCharacters = []() -> juce::String {
    juce::String str;

    // Only map printable characters
    for (juce::juce_wchar c = 32; c < 127; ++c)
        str += juce::String::charToString (c);

    return str;
}();

const static int maxImageCacheSize = 256;

static NVGcolor nvgColour (const juce::Colour& c)
{
    return nvgRGBA (c.getRed(), c.getGreen(), c.getBlue(), c.getAlpha());
}

static uint64_t getImageHash (const juce::Image& image)
{
    juce::Image::BitmapData src (image, juce::Image::BitmapData::readOnly);
    return (uint64_t) src.data;
}

static const char* getResourceByFileName(const juce::String& fileName, int& size)
{
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        if (juce::String(BinaryData::originalFilenames[i]) == fileName)
            return BinaryData::getNamedResource(BinaryData::namedResourceList[i], size);
    }

    return nullptr;
}

//==============================================================================

const juce::String NanoVGGraphicsContext::defaultTypefaceName = "InterBold";

const int NanoVGGraphicsContext::imageCacheSize = 256;

//==============================================================================


NanoVGGraphicsContext::NanoVGGraphicsContext (void* nativeHandle, int w, int h, float pixelScale) :
      width {w},
      height {h},
      scale{pixelScale}
{
#if NANOVG_METAL_IMPLEMENTATION
    nvg = nvgCreateContext(nativeHandle, NVG_ANTIALIAS | NVG_TRIPLE_BUFFER, width, height);
#else
    nvg = nvgCreateContext(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
#endif

    nvgGlobalCompositeOperation(nvg, NVG_SOURCE_OVER);

    jassert(nvg);
    

    //loadFontFromResources(defaultTypefaceName);
}

NanoVGGraphicsContext::~NanoVGGraphicsContext()
{
}

bool NanoVGGraphicsContext::isVectorDevice() const { return false; }

void NanoVGGraphicsContext::setOrigin (juce::Point<int> origin)
{
    nvgTranslate (nvg, origin.getX(), origin.getY());
}

void NanoVGGraphicsContext::addTransform (const juce::AffineTransform& t)
{
    nvgTransform (nvg, t.mat00, t.mat10, t.mat01, t.mat11, t.mat02, t.mat12);
}

float NanoVGGraphicsContext::getPhysicalPixelScaleFactor() { return scale; }

bool NanoVGGraphicsContext::clipToRectangle (const juce::Rectangle<int>& rect)
{
    nvgIntersectScissor (nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    return !getClipBounds().isEmpty();
}

bool NanoVGGraphicsContext::clipToRectangleList (const juce::RectangleList<int>& rects)
{
    for (const auto& rect : rects)
        nvgIntersectScissor (nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());

    return ! getClipBounds().isEmpty();
}

void NanoVGGraphicsContext::excludeClipRectangle (const juce::Rectangle<int>& rectangle)
{
   //nvgResetScissor(nvg);
    
    juce::RectangleList<int> rects;

   // Get the rectangle coordinates in NanoVG coordinates
   float x = static_cast<float>(rectangle.getX());
   float y = static_cast<float>(rectangle.getY());
   float w = static_cast<float>(rectangle.getWidth());
   float h = static_cast<float>(rectangle.getHeight());
   nvgRect(nvg, x, y, w, h);

   // Exclude the rectangle from the clipping area
   nvgPathWinding(nvg, NVG_HOLE);
   nvgScissor(nvg, 0, 0, static_cast<float>(width), static_cast<float>(height));
}

void NanoVGGraphicsContext::clipToPath (const juce::Path& path, const juce::AffineTransform& t)
{
    // @todo
    const auto rect = path.getBoundsTransformed (t);
    nvgIntersectScissor (nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
}

void NanoVGGraphicsContext::clipToImageAlpha (const juce::Image& sourceImage, const juce::AffineTransform& transform)
{
    if (!transform.isSingularity()) {
        // Convert the image to a single-channel image if necessary
        juce::Image singleChannelImage(sourceImage);
        if (sourceImage.getFormat() != juce::Image::SingleChannel) {
            singleChannelImage = sourceImage.convertedToFormat(juce::Image::SingleChannel);
        }

        juce::Image::BitmapData bitmapData(singleChannelImage, juce::Image::BitmapData::readOnly);
        auto* pixelData = bitmapData.data;

        // Create a new Nanovg image from the bitmap data
        int width = singleChannelImage.getWidth();
        int height = singleChannelImage.getHeight();
        auto image = nvgCreateImageRGBA(nvg, width, height, 0, pixelData);
        auto paint = nvgImagePattern(nvg, 0, 0, width, height, 0, image, 1);
        
        nvgSave(nvg);
        nvgTransform(nvg, transform.mat00, transform.mat10, transform.mat01, transform.mat11, transform.mat02, transform.mat12);
        nvgScale(nvg, 1.0f, -1.0f);

        // Clip the graphics context to the alpha mask of the Nanovg image
        nvgBeginPath(nvg);
        nvgRect(nvg, 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
        nvgPathWinding(nvg, NVG_HOLE);
        nvgFillPaint(nvg, paint);
        nvgFill(nvg);

        // Restore the original transformations
        nvgRestore(nvg);
    }
}


bool NanoVGGraphicsContext::clipRegionIntersects (const juce::Rectangle<int>& rect)
{
    auto clip = getClipBounds();
    return clip.intersects (rect);
}

juce::Rectangle<int> NanoVGGraphicsContext::getClipBounds() const
{
    float x = 0.0f;
    float y = 0.0f;
    float w = width;
    float h = height;

    nvgCurrentScissor (nvg, &x, &y, &w, &h);
    return juce::Rectangle<int>((int)x, (int)y, (int)w, (int)h);
}

bool NanoVGGraphicsContext::isClipEmpty() const
{
    float x = 0.0f;
    float y = 0.0f;
    float w = width;
    float h = height;

    nvgCurrentScissor (nvg, &x, &y, &w, &h);

    return w < 0.0f || h < 0.0f;
}

void NanoVGGraphicsContext::saveState()
{
    nvgSave (nvg);
}

void NanoVGGraphicsContext::restoreState()
{
    nvgRestore (nvg);
}

void NanoVGGraphicsContext::beginTransparencyLayer (float op)
{
    saveState();
    nvgGlobalAlpha (nvg, op);
}

void NanoVGGraphicsContext::endTransparencyLayer()
{
    restoreState();
}

void NanoVGGraphicsContext::setFill (const juce::FillType& fillType)
{
    if (fillType.isColour())
    {
        auto c = nvgColour (fillType.colour);
        nvgFillColor (nvg, c);
        nvgStrokeColor (nvg, c);
    }
    else if (fillType.isGradient())
    {
        if (juce::ColourGradient* gradient = fillType.gradient.get())
        {
            const auto numColours = gradient->getNumColours();

            if (numColours == 1)
            {
                // Just a solid fill
                nvgFillColor (nvg, nvgColour (gradient->getColour (0)));
            }
            else if (numColours > 1)
            {
                NVGpaint p;

                if (gradient->isRadial)
                {
                    p = nvgRadialGradient (nvg,
                                           gradient->point1.getX(), gradient->point1.getY(),
                                           gradient->point2.getX(), gradient->point2.getY(),
                                           nvgColour (gradient->getColour (0)), nvgColour (gradient->getColour (numColours - 1)));
                }
                else
                {
                    p = nvgLinearGradient (nvg,
                                           gradient->point1.getX(), gradient->point1.getY(),
                                           gradient->point2.getX(), gradient->point2.getY(),
                                           nvgColour (gradient->getColour (0)), nvgColour (gradient->getColour (numColours - 1)));
                }

                nvgFillPaint (nvg, p);
            }
        }
    }
}

void NanoVGGraphicsContext::setOpacity(float op)
{
    auto c = nvgGetFillColor(nvg);
    nvgRGBAf(c.r, c.g, c.b, op);
    nvgFillColor(nvg, c);
    nvgStrokeColor(nvg, c);
}

void NanoVGGraphicsContext::setInterpolationQuality (juce::Graphics::ResamplingQuality)
{
    // @todo
}

void NanoVGGraphicsContext::fillRect (const juce::Rectangle<int>& rect, bool /* replaceExistingContents */)
{
    nvgBeginPath (nvg);
    nvgRect (nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    nvgFill (nvg);
}

void NanoVGGraphicsContext::fillRect (const juce::Rectangle<float>& rect)
{
    nvgBeginPath (nvg);
    nvgRect (nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    nvgFill (nvg);
}

void NanoVGGraphicsContext::fillRectList (const juce::RectangleList<float>& rects)
{
    for (const auto& rect : rects)
        fillRect (rect);
}

void NanoVGGraphicsContext::strokePath (const juce::Path& path, const juce::PathStrokeType& strokeType, const juce::AffineTransform& transform)
{
    // First set options
    switch (strokeType.getEndStyle())
    {
        case juce::PathStrokeType::EndCapStyle::butt:     nvgLineCap(nvg, NVG_BUTT);     break;
        case juce::PathStrokeType::EndCapStyle::rounded:  nvgLineCap(nvg, NVG_ROUND);    break;
        case juce::PathStrokeType::EndCapStyle::square:   nvgLineCap(nvg, NVG_SQUARE);   break;
    }

    switch (strokeType.getJointStyle())
    {
        case juce::PathStrokeType::JointStyle::mitered: nvgLineJoin(nvg, NVG_MITER);   break;
        case juce::PathStrokeType::JointStyle::curved:  nvgLineJoin(nvg, NVG_ROUND);   break;
        case juce::PathStrokeType::JointStyle::beveled: nvgLineJoin(nvg, NVG_BEVEL);   break;
    }

    nvgStrokeWidth(nvg, strokeType.getStrokeThickness());
    nvgPathWinding(nvg, NVG_CCW);
    setPath(path, transform);
    nvgStroke(nvg);
}

void NanoVGGraphicsContext::setPath (const juce::Path& path, const juce::AffineTransform& transform)
{
    juce::Path p (path);
    p.applyTransform (transform);

    nvgBeginPath (nvg);

    juce::Path::Iterator i (p);

    // Flag is used to flip winding when drawing shapes with holes.
    bool solid = true;

    while (i.next())
    {
        switch (i.elementType)
        {
            case juce::Path::Iterator::startNewSubPath:
                nvgMoveTo (nvg, i.x1, i.y1);
                break;
            case juce::Path::Iterator::lineTo:
                nvgLineTo (nvg, i.x1, i.y1);
                break;
            case juce::Path::Iterator::quadraticTo:
                nvgQuadTo (nvg, i.x1, i.y1, i.x2, i.y2);
                break;
            case juce::Path::Iterator::cubicTo:
                nvgBezierTo (nvg, i.x1, i.y1, i.x2, i.y2, i.x3, i.y3);
                break;
            case juce::Path::Iterator::closePath:
                nvgClosePath (nvg);
                nvgPathWinding (nvg, solid ? NVG_SOLID : NVG_HOLE);
                solid = ! solid;
                break;
        default:
            break;
        }
    }

}

void NanoVGGraphicsContext::fillPath (const juce::Path& path, const juce::AffineTransform& transform)
{
    setPath(path, transform);
    nvgFill (nvg);
}

void NanoVGGraphicsContext::drawImage (const juce::Image& image, const juce::AffineTransform& t)
{
    if (image.isARGB())
    {
        juce::Image::BitmapData srcData (image, juce::Image::BitmapData::readOnly);
        auto id = getNvgImageId (image);

        if (id < 0)
            return; // invalid image.

        juce::Rectangle<float> rect (0.0f, 0.0f, image.getWidth(), image.getHeight());
        rect = rect.transformedBy (t);

        NVGpaint imgPaint = nvgImagePattern (nvg,
                                             0, 0,
                                             rect.getWidth(), rect.getHeight(),
                                             0.0f,   // angle
                                             id,
                                             1.0f    // alpha
                                             );

        nvgBeginPath (nvg);
        nvgRect (nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
        nvgFillPaint (nvg, imgPaint);
        nvgFill (nvg);
    }
    else if(image.isRGB()){
        
        auto argbImage = juce::Image (juce::Image::ARGB, image.getWidth(), image.getHeight(), true);
        
        // TODO: can this be done more efficiently?
        for (int y = 0; y < image.getHeight(); ++y)
        {
            for (int x = 0; x < image.getWidth(); ++x)
            {
                argbImage.setPixelAt (x, y, image.getPixelAt (x, y).withAlpha(1.0f));
            }
        }

        // Render using ARGB image data
        drawImage(argbImage, t);
    }
    else if(image.isSingleChannel()){
        
        auto argbImage = juce::Image (juce::Image::ARGB, image.getWidth(), image.getHeight(), true);
        
        for (int y = 0; y < image.getHeight(); ++y)
        {
            for (int x = 0; x < image.getWidth(); ++x)
            {
                const auto alpha = image.getPixelAt (x, y).getAlpha();
                argbImage.setPixelAt (x, y, juce::Colour::fromRGBA(0, 0, 0, alpha));
            }
        }

        // Render using ARGB image data
        drawImage(argbImage, t);
    }
}

void NanoVGGraphicsContext::drawLine (const juce::Line<float>& line)
{
    nvgBeginPath (nvg);
    nvgMoveTo (nvg, line.getStartX(), line.getStartY());
    nvgLineTo (nvg, line.getEndX(), line.getEndY());
    nvgStroke (nvg);
}

void NanoVGGraphicsContext::setFont (const juce::Font& f)
{
    font = f;
    juce::String name {font.getTypefacePtr()->getName()};
    name << "-" << font.getTypefaceStyle();

    if (loadFontFromResources (name))
        nvgFontFace (nvg, name.toUTF8());
    else
        nvgFontFace (nvg, defaultTypefaceName.toRawUTF8());

    nvgFontSize (nvg, font.getHeight());
}

const juce::Font& NanoVGGraphicsContext::getFont()
{
    return font;
}

void NanoVGGraphicsContext::drawGlyph (int glyphNumber, const juce::AffineTransform& t)
{
    if (currentGlyphToCharMap == nullptr)
        return;

    char txt[2] = {'?', 0};

    if (currentGlyphToCharMap->find (glyphNumber) != currentGlyphToCharMap->end())
        txt[0] = (char)currentGlyphToCharMap->at (glyphNumber);

    nvgText (nvg, t.getTranslationX(), t.getTranslationY(), txt, &txt[1]);

}

bool NanoVGGraphicsContext::drawTextLayout (const juce::AttributedString& str, const juce::Rectangle<float>& rect)
{
    nvgSave (nvg);
    nvgIntersectScissor (nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());

    const std::string text = str.getText().toStdString();
    const char* textPtr = text.c_str();

    // NOTE:
    // This will not perform the correct rendering when JUCE's assumed font
    // differs from the one set by nanovg. It will probably look readable
    // and so, by if in editor, cursor position may be off because of
    // different font metrics. This should be fixed by using the same
    // fonts between JUCE and nanovg rendered.
    //
    // JUCE 6's default fonts on desktop are:
    //  Sans-serif: Verdana
    //  Serif:      Times New Roman
    //  Monospace:  Lucida Console

    float x = rect.getX();
    const float y = rect.getY();

    nvgTextAlign (nvg, NVG_ALIGN_TOP);

    for (int i = 0; i < str.getNumAttributes(); ++i)
    {
        const auto attr = str.getAttribute (i);
        setFont (attr.font);
        nvgFillColor (nvg, nvgColour (attr.colour));

        const char* begin = &textPtr[attr.range.getStart()];
        const char* end = &textPtr[attr.range.getEnd()];

        // We assume that ranges are sorted by x so that we can move
        // to the next glyph position efficiently.
        x = nvgText (nvg, x, y, begin, end);
    }

    nvgRestore (nvg);
    return true;
}

void NanoVGGraphicsContext::resized(int w, int h, float pixelScale)
{
    scale = pixelScale;
    width = w;
    height = h;
}

void NanoVGGraphicsContext::removeCachedImages()
{
    for (auto it = images.begin(); it != images.end(); ++it)
        nvgDeleteImage (nvg, it->second.id);

    images.clear();
}

bool NanoVGGraphicsContext::loadFont (const juce::String& name, const char* ptr, int size)
{
    if (ptr != nullptr && size > 0)
    {
        const int id = nvgCreateFontMem (nvg, name.toRawUTF8(),
                                         (unsigned char*)ptr, size,
                                         0 // Tell nvg to take ownership of the font data
                                        );

        if (id >= 0)
        {
            juce::Font tmpFont (juce::Typeface::createSystemTypefaceFor (ptr, (size_t)size));
            loadedFonts[name] = getGlyphToCharMapForFont (tmpFont);
            currentGlyphToCharMap = &loadedFonts[name];
            return true;
        }
    }
    else
    {
        std::cerr << "Unabled to load " << name << "\n";
        return false;
    }
}

bool NanoVGGraphicsContext::loadFontFromResources (const juce::String& typefaceName)
{
    auto it = loadedFonts.find (typefaceName);

    if (it != loadedFonts.end())
    {
        currentGlyphToCharMap = &it->second;
        return true; // Already loaded
    }

    int size;
    juce::String resName {typefaceName + "_ttf"};
    const auto* ptr {getResourceByFileName(resName, size)};

    return loadFont(typefaceName, ptr, size);
}

int NanoVGGraphicsContext::getNvgImageId (const juce::Image& image)
{
    int id = -1;
    const auto hash = getImageHash (image);
    auto it = images.find (hash);

    if (it == images.end())
    {
        // Nanovg expects images in RGBA format, so we do conversion here
        juce::Image argbImage (image);
        argbImage.duplicateIfShared();

        argbImage = argbImage.convertedToFormat (juce::Image::PixelFormat::ARGB);
        juce::Image::BitmapData bitmap (argbImage, juce::Image::BitmapData::readOnly);

        for (int y = 0; y < argbImage.getHeight(); ++y)
        {
            auto* scanLine = (juce::uint32*) bitmap.getLinePointer (y);

            for (int x = 0; x < argbImage.getWidth(); ++x)
            {
                juce::uint32 argb = scanLine[x];
                juce::uint32 abgr =  (argb & 0xFF00FF00)          // a, g
                            | ((argb & 0x000000FF) << 16)   // b
                            | ((argb & 0x00FF0000) >> 16);  // r

                scanLine[x] = abgr; // bytes order
            }
        }

        id = nvgCreateImageRGBA (nvg, argbImage.getWidth(), argbImage.getHeight(), 0, bitmap.data);

        if (images.size() >= maxImageCacheSize)
            reduceImageCache();

        images[hash] = { id, 1 };
    }
    else
    {
        it->second.accessCounter++;
        id = it->second.id;
    }

    return id;
}

void NanoVGGraphicsContext::reduceImageCache()
{
    int minAccessCounter = 0;

    for (auto it = images.begin(); it != images.end(); ++it)
    {
        minAccessCounter = minAccessCounter == 0 ? it->second.accessCounter
                                                 : juce::jmin (minAccessCounter, it->second.accessCounter);
    }

    auto it = images.begin();

    while (it != images.end())
    {
        if (it->second.accessCounter == minAccessCounter)
        {
            nvgDeleteImage (nvg, it->second.id);
            it = images.erase(it);
        }
        else
        {
            it->second.accessCounter -= minAccessCounter;
            ++it;
        }
    }
}

NanoVGGraphicsContext::GlyphToCharMap NanoVGGraphicsContext::getGlyphToCharMapForFont (const juce::Font& f)
{
    NanoVGGraphicsContext::GlyphToCharMap map;

    if (auto tf = f.getTypefacePtr())
    {
        juce::Array<int> glyphs;
        juce::Array<float> offsets;
        tf->getGlyphPositions (allPrintableAsciiCharacters, glyphs, offsets);

        // Make sure we get all the glyphs for the printable characters
        //jassert (glyphs.size() >= allPrintableAsciiCharacters.length());

        const auto* wstr = allPrintableAsciiCharacters.toWideCharPointer();

        for (int i = 0; i < allPrintableAsciiCharacters.length(); ++i)
            map[glyphs[i]] = wstr[i];
    }

    return map;
}
