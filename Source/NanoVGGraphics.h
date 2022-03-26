//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com> and Timothy Schoen <timschoen123@gmail.com>
//

#pragma once

#include <JuceHeader.h>


using namespace juce::gl;

#include <nanovg.h>

#if defined NANOVG_GL2_IMPLEMENTATION
  #define NANOVG_GL_IMPLEMENTATION 1
  #define nvgCreateContext(flags) nvgCreateGL2(flags)
  #define nvgDeleteContext(context) nvgDeleteGL2(context)
#elif defined NANOVG_GLES2_IMPLEMENTATION
  #define NANOVG_GL_IMPLEMENTATION 1
  #define nvgCreateContext(flags) nvgCreateGLES2(flags)
  #define nvgDeleteContext(context) nvgDeleteGLES2(context)
#elif defined NANOVG_GL3_IMPLEMENTATION
  #define NANOVG_GL_IMPLEMENTATION 1
  #define nvgCreateContext(flags) nvgCreateGL3(flags)
  #define nvgDeleteContext(context) nvgDeleteGL3(context)
#elif defined NANOVG_GLES3_IMPLEMENTATION
  #define NANOVG_GL_IMPLEMENTATION 1
  #define nvgCreateContext(flags) nvgCreateGLES3(flags)
  #define nvgDeleteContext(context) nvgDeleteGLES3(context)
#elif defined NANOVG_METAL_IMPLEMENTATION
  #define nvgCreateContext(layer, flags, w, h) mnvgCreateContext(layer, flags, w, h)
  #define nvgDeleteContext(context) nvgDeleteMTL(context)
  #define nvgBindFramebuffer(fb) mnvgBindFramebuffer(fb)
  #define nvgCreateFramebuffer(ctx, w, h, flags) mnvgCreateFramebuffer(ctx, w, h, flags)
  #define nvgDeleteFramebuffer(fb) mnvgDeleteFramebuffer(fb)
#endif

/**
    JUCE low level graphics context backed by nanovg.

    @note This is not a perfect translation of the JUCE
          graphics, but its still quite usable.
*/



class NanoVGGraphicsContext : public juce::LowLevelGraphicsContext
{
public:
    NanoVGGraphicsContext (void* nativeHandle, int width, int height, float scale);
    ~NanoVGGraphicsContext();

    bool isVectorDevice() const override;
    void setOrigin (juce::Point<int>) override;
    void addTransform (const juce::AffineTransform&) override;
    float getPhysicalPixelScaleFactor() override;

    bool clipToRectangle (const juce::Rectangle<int>&) override;
    bool clipToRectangleList (const juce::RectangleList<int>&) override;
    void excludeClipRectangle (const juce::Rectangle<int>&) override;
    void clipToPath (const juce::Path&, const juce::AffineTransform&) override;
    void clipToImageAlpha (const juce::Image&, const juce::AffineTransform&) override;

    bool clipRegionIntersects (const juce::Rectangle<int>&) override;
    juce::Rectangle<int> getClipBounds() const override;
    bool isClipEmpty() const override;

    void saveState() override;
    void restoreState() override;

    void beginTransparencyLayer (float opacity) override;
    void endTransparencyLayer() override;

    void setFill (const juce::FillType&) override;
    void setOpacity (float) override;
    void setInterpolationQuality (juce::Graphics::ResamplingQuality) override;

    void fillRect (const juce::Rectangle<int>&, bool) override;
    void fillRect (const juce::Rectangle<float>&) override;
    void fillRectList (const juce::RectangleList<float>&) override;

    void setPath (const juce::Path& path, const juce::AffineTransform& transform);
    
    void strokePath (const juce::Path&, const juce::PathStrokeType&, const juce::AffineTransform&) override;
    void fillPath (const juce::Path&, const juce::AffineTransform&) override;
    void drawImage (const juce::Image&, const juce::AffineTransform&) override;
    void drawLine (const juce::Line<float>&) override;

    void setFont (const juce::Font&) override;
    const juce::Font& getFont() override;
    void drawGlyph (int glyphNumber, const juce::AffineTransform&) override;
    bool drawTextLayout (const juce::AttributedString&, const juce::Rectangle<float>&) override;

    void resized (int w, int h, float scale);

    void removeCachedImages();

    NVGcontext* getContext() const { return nvg; };

    const static juce::String defaultTypefaceName;

    const static int imageCacheSize;

private:

    bool loadFontFromResources (const juce::String& typefaceName);
    void applyFillType();
    void applyStrokeType();
    void applyFont();

    int getNvgImageId (const juce::Image& image);
    void reduceImageCache();

    NVGcontext* nvg;

    int width;
    int height;
    float scale = 1.0f;

    juce::FillType fillType;
    juce::Font font;

    // Mapping glyph number to a character
    using GlyphToCharMap = std::map<int, wchar_t>;

    GlyphToCharMap getGlyphToCharMapForFont (const juce::Font& f);

    // Mapping font names to glyph-to-character tables
    std::map<juce::String, GlyphToCharMap> loadedFonts;
    const GlyphToCharMap* currentGlyphToCharMap;

    // Tracking images mapped tomtextures.
    struct NvgImage
    {
        int id {-1};            ///< Image/texture ID.
        int accessCounter {0};  ///< Usage counter.
    };


    std::map<juce::uint64, NvgImage> images;
};
