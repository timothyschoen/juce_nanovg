//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com> and Timothy Schoen <timschoen123@gmail.com>
//

#include "NanoVGGraphics.h"
#include <nanovg_mtl.h>
//==============================================================================

#define STB_TRUETYPE_IMPLEMENTATION
//#include <stb/stb_truetype.h>

extern "C"{
void nvgCurrentScissor(NVGcontext* ctx, float* x, float* y, float* w, float* h);
}

//==============================================================================

const static auto allPrintableAsciiCharacters = []() -> String {
    String str;

    // Only map printable characters
    for (juce_wchar c = 32; c < 127; ++c)
        str += String::charToString (c);

    return str;
}();

const static int maxImageCacheSize = 256;

static NVGcolor nvgColour (const Colour& c)
{
    return nvgRGBA (c.getRed(), c.getGreen(), c.getBlue(), c.getAlpha());
}

static uint64_t getImageHash (const Image& image)
{
    Image::BitmapData src (image, Image::BitmapData::readOnly);
    return (uint64_t) src.data;
}

static const char* getResourceByFileName(const String& fileName, int& size)
{
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        if (String(BinaryData::originalFilenames[i]) == fileName)
            return BinaryData::getNamedResource(BinaryData::namedResourceList[i], size);
    }

    return nullptr;
}

//==============================================================================

const String NanoVGGraphicsContext::defaultTypefaceName = "Verdana-Regular";

const int NanoVGGraphicsContext::imageCacheSize = 256;

//==============================================================================

NanoVGGraphicsContext::NanoVGGraphicsContext (void* nativeHandle, int w, int h) :
      width {w},
      height {h}
{
#if JUCE_MAC
        nvg = nvgCreateContext(nativeHandle, NVG_ANTIALIAS | NVG_TRIPLE_BUFFER, width, height);
#else
        nvg = nvgCreateContext(nativeHandle, NVG_ANTIALIAS);
#endif
    
    nvgGlobalCompositeOperation(nvg, NVG_SOURCE_OVER);

    loadFontFromResources(defaultTypefaceName);
}

NanoVGGraphicsContext::~NanoVGGraphicsContext()
{
}

bool NanoVGGraphicsContext::isVectorDevice() const { return false; }

void NanoVGGraphicsContext::setOrigin (juce::Point<int> origin)
{
    nvgTranslate (nvg, origin.getX(), origin.getY());
}

void NanoVGGraphicsContext::addTransform (const AffineTransform& t)
{
    nvgTransform (nvg, t.mat00, t.mat10, t.mat01, t.mat11, t.mat02, t.mat12);
}

float NanoVGGraphicsContext::getPhysicalPixelScaleFactor() { return 1.0f; }

bool NanoVGGraphicsContext::clipToRectangle (const Rectangle<int>& rect)
{
    nvgIntersectScissor (nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    return !getClipBounds().isEmpty();
}

bool NanoVGGraphicsContext::clipToRectangleList (const RectangleList<int>& rects)
{
    for (const auto& rect : rects)
        nvgIntersectScissor (nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());

    return ! getClipBounds().isEmpty();
}

void NanoVGGraphicsContext::excludeClipRectangle (const Rectangle<int>& rect)
{
    // @todo
    jassertfalse;
}

void NanoVGGraphicsContext::clipToPath (const Path& path, const AffineTransform& t)
{
    // @todo
    const auto rect = path.getBoundsTransformed (t);
    nvgIntersectScissor (nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
}

void NanoVGGraphicsContext::clipToImageAlpha (const Image&, const AffineTransform&)
{
    // @todo
    jassertfalse;
}

bool NanoVGGraphicsContext::clipRegionIntersects (const Rectangle<int>& rect)
{
    auto clip = getClipBounds();
    return clip.intersects (rect);
}

Rectangle<int> NanoVGGraphicsContext::getClipBounds() const
{
    float x = 0.0f;
    float y = 0.0f;
    float w = width;
    float h = height;
    
    
    nvgCurrentScissor (nvg, &x, &y, &w, &h);
    return Rectangle<int>((int)x, (int)y, (int)w, (int)h);
}

bool NanoVGGraphicsContext::isClipEmpty() const
{
    float x = 0.0f;
    float y = 0.0f;
    float w = -1.0f;
    float h = -1.0f;

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

void NanoVGGraphicsContext::setFill (const FillType& f)
{
    fillType = f;
}

void NanoVGGraphicsContext::setOpacity(float op)
{
    fillType.setOpacity(op);
}

void NanoVGGraphicsContext::setInterpolationQuality (Graphics::ResamplingQuality)
{
    // @todo
}

void NanoVGGraphicsContext::fillRect (const Rectangle<int>& rect, bool /* replaceExistingContents */)
{
    nvgBeginPath (nvg);
    applyFillType();
    nvgRect (nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    nvgFill (nvg);
}

void NanoVGGraphicsContext::fillRect (const Rectangle<float>& rect)
{
    nvgBeginPath (nvg);
    applyFillType();
    nvgRect (nvg, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());
    nvgFill (nvg);
}

void NanoVGGraphicsContext::fillRectList (const RectangleList<float>& rects)
{
    for (const auto& rect : rects)
        fillRect (rect);
}

void NanoVGGraphicsContext::fillPath (const Path& path, const AffineTransform& transform)
{
    Path p (path);
    p.applyTransform (transform);

    nvgBeginPath (nvg);

    Path::Iterator i (p);

    // Flag is used to flip winding when drawing shapes with holes.
    bool solid = true;

    while (i.next())
    {
        switch (i.elementType)
        {
        case Path::Iterator::startNewSubPath:
            nvgMoveTo (nvg, i.x1, i.y1);
            break;
        case Path::Iterator::lineTo:
            nvgLineTo (nvg, i.x1, i.y1);
            break;
        case Path::Iterator::quadraticTo:
            nvgQuadTo (nvg, i.x1, i.y1, i.x2, i.y2);
            break;
        case Path::Iterator::cubicTo:
            nvgBezierTo (nvg, i.x1, i.y1, i.x2, i.y2, i.x3, i.y3);
            break;
        case Path::Iterator::closePath:
            nvgClosePath (nvg);
            nvgPathWinding (nvg, solid ? NVG_SOLID : NVG_HOLE);
            solid = ! solid;
            break;
        default:
            break;
        }
    }

    applyFillType();
    nvgFill (nvg);
}

void NanoVGGraphicsContext::drawImage (const Image& image, const AffineTransform& t)
{
    if (image.isARGB())
    {
        Image::BitmapData srcData (image, Image::BitmapData::readOnly);
        auto id = getNvgImageId (image);

        if (id < 0)
            return; // invalid image.

        Rectangle<float> rect (0.0f, 0.0f, image.getWidth(), image.getHeight());
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
    else
        jassertfalse;

}

void NanoVGGraphicsContext::drawLine (const Line<float>& line)
{
    nvgBeginPath (nvg);
    nvgMoveTo (nvg, line.getStartX(), line.getStartY());
    nvgLineTo (nvg, line.getEndX(), line.getEndY());
    applyStrokeType();
    nvgStroke (nvg);
}

void NanoVGGraphicsContext::setFont (const Font& f)
{
    font = f;
    applyFont();
}

const Font& NanoVGGraphicsContext::getFont()
{
    return font;
}

void NanoVGGraphicsContext::drawGlyph (int glyphNumber, const AffineTransform& t)
{
    if (currentGlyphToCharMap == nullptr)
        return;

    char txt[2] = {'?', 0};

    if (currentGlyphToCharMap->find (glyphNumber) != currentGlyphToCharMap->end())
        txt[0] = (char)currentGlyphToCharMap->at (glyphNumber);

    nvgFillColor (nvg, nvgColour (fillType.colour));
    nvgText (nvg, t.getTranslationX(), t.getTranslationY(), txt, &txt[1]);

}

bool NanoVGGraphicsContext::drawTextLayout (const AttributedString& str, const Rectangle<float>& rect)
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

void NanoVGGraphicsContext::resized(int w, int h)
{
    width = w;
    height = h;
}

void NanoVGGraphicsContext::removeCachedImages()
{
    for (auto it = images.begin(); it != images.end(); ++it)
        nvgDeleteImage (nvg, it->second.id);

    images.clear();
}

bool NanoVGGraphicsContext::loadFontFromResources (const String& typefaceName)
{
    auto it = loadedFonts.find (typefaceName);

    if (it != loadedFonts.end())
    {
        currentGlyphToCharMap = &it->second;
        return true; // Already loaded
    }

    int size{};
    String resName {typefaceName + ".ttf"};
    const auto* ptr {getResourceByFileName(resName, size)};

    if (ptr != nullptr && size > 0)
    {
        const int id = nvgCreateFontMem (nvg, typefaceName.toRawUTF8(),
                                         (unsigned char*)ptr, size,
                                         0 // Tell nvg to take ownership of the font data
                                        );

        if (id >= 0)
        {
            Font tmpFont (Typeface::createSystemTypefaceFor (ptr, size));
            loadedFonts[typefaceName] = getGlyphToCharMapForFont (tmpFont);
            currentGlyphToCharMap = &loadedFonts[typefaceName];
            return true;
        }
    }
    else
    {
        std::cerr << "Unabled to load " << resName << "\n";
    }

    return false;

}


void NanoVGGraphicsContext::applyFillType()
{
    if (fillType.isColour())
    {
        nvgFillColor (nvg, nvgColour (fillType.colour));
    }
    else if (fillType.isGradient())
    {
        if (ColourGradient* gradient = fillType.gradient.get())
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

void NanoVGGraphicsContext::applyStrokeType()
{
    if (fillType.isColour())
    {
        const auto& c = fillType.colour;
        nvgStrokeColor (nvg, nvgRGBA (c.getRed(), c.getGreen(), c.getBlue(), c.getAlpha()));
    }
}

void NanoVGGraphicsContext::applyFont()
{
    String name {font.getTypefacePtr()->getName()};
    name << "-" << font.getTypefaceStyle();

    if (loadFontFromResources (name))
        nvgFontFace (nvg, name.toUTF8());
    else
        nvgFontFace (nvg, defaultTypefaceName.toRawUTF8());

    nvgFontSize (nvg, font.getHeight());
}

int NanoVGGraphicsContext::getNvgImageId (const Image& image)
{
    int id = -1;
    const auto hash = getImageHash (image);
    auto it = images.find (hash);

    if (it == images.end())
    {
        // Nanovg expects images in RGBA format, so we do conversion here
        Image argbImage (image);
        argbImage.duplicateIfShared();

        argbImage = argbImage.convertedToFormat (Image::PixelFormat::ARGB);
        Image::BitmapData bitmap (argbImage, Image::BitmapData::readOnly);

        for (int y = 0; y < argbImage.getHeight(); ++y)
        {
            auto* scanLine = (uint32*) bitmap.getLinePointer (y);

            for (int x = 0; x < argbImage.getWidth(); ++x)
            {
                uint32 argb = scanLine[x];
                uint32 abgr =  (argb & 0xFF00FF00)          // a, g
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
                                                 : jmin (minAccessCounter, it->second.accessCounter);
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

NanoVGGraphicsContext::GlyphToCharMap NanoVGGraphicsContext::getGlyphToCharMapForFont (const Font& f)
{
    NanoVGGraphicsContext::GlyphToCharMap map;

    if (auto tf = f.getTypefacePtr())
    {
        Array<int> glyphs;
        Array<float> offsets;
        tf->getGlyphPositions (allPrintableAsciiCharacters, glyphs, offsets);

        // Make sure we get all the glyphs for the printable characters
        jassert (glyphs.size() == allPrintableAsciiCharacters.length());

        const auto* wstr = allPrintableAsciiCharacters.toWideCharPointer();

        for (int i = 0; i < allPrintableAsciiCharacters.length(); ++i)
            map[glyphs[i]] = wstr[i];
    }

    return map;
}
