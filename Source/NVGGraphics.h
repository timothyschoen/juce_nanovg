#pragma once
#include <stack>
#include <juce_gui_basics/juce_gui_basics.h>
#include "NanoVGGraphicsContext.h"


class NVGGraphics
{
public:
    
    NVGcontext* nvg;
    juce::Graphics& g;
    juce::LowLevelGraphicsContext& context;
    bool saveStatePending = false;
    
    NVGGraphics(juce::Graphics& graphics) : g(graphics), context(graphics.getInternalContext())
    {
        if(auto* ctx = dynamic_cast<NanoVGGraphicsContext*>(&context)) {
            nvg = ctx->getContext();
        }
        else {
            nvg = nullptr;
        }
    }
    
    void drawText(const juce::String& text, juce::Rectangle<float> area,
                  juce::Justification justificationType, bool useEllipsesIfTooBig) const
    {
        if (!nvg) {
            g.drawText(text, area, justificationType, useEllipsesIfTooBig);
            return;
        }

        int align = 0;
        float x, y;
        
        switch (justificationType.getOnlyHorizontalFlags())
        {
            case juce::Justification::left: {
                align = NVG_ALIGN_LEFT;
                x = area.getX();
                break;
            }
            case juce::Justification::horizontallyCentred: {
                align = NVG_ALIGN_CENTER;
                x = area.getCentreX();
                break;
            }
            case juce::Justification::right: {
                align = NVG_ALIGN_RIGHT;
                x = area.getRight();
                break;
            }
          }
        
        switch (justificationType.getOnlyVerticalFlags())
        {
            case juce::Justification::top: {
                align |= NVG_ALIGN_TOP;
                y = area.getY();
                break;
            }
            case juce::Justification::verticallyCentred: {
                align |= NVG_ALIGN_MIDDLE;
                y = area.getCentreY();
                break;
            }
            case juce::Justification::bottom: {
                align |= NVG_ALIGN_BOTTOM;
                y = area.getBottom();
                break;
            }
          }
          
        
        nvgSave(nvg);
        nvgTextAlign(nvg, align);
        //nvgTextBounds(nvg, x, y, text.toRawUTF8(), nullptr, fbounds);
        nvgText(nvg, x, y, text.toRawUTF8(), nullptr);
        
        //nvgTextBox(nvg, x, y, area.getWidth(), text.toRawUTF8(), nullptr);
        nvgRestore(nvg);
    }
    
    void drawFittedText (const juce::String& text, juce::Rectangle<int> area,
                         juce::Justification justification,
                         const int maximumNumberOfLines,
                         const float minimumHorizontalScale) const
    {
        if(!nvg)
        {
            g.drawFittedText(text, area, justification, maximumNumberOfLines, minimumHorizontalScale);
            return;
        }

         int align = 0;
         float x, y;
         
         switch (justification.getOnlyHorizontalFlags())
         {
             case juce::Justification::left: {
                 align = NVG_ALIGN_LEFT;
                 x = area.getX();
                 break;
             }
             case juce::Justification::horizontallyCentred: {
                 align = NVG_ALIGN_CENTER;
                 x = area.getCentreX();
                 break;
             }
             case juce::Justification::right: {
                 align = NVG_ALIGN_RIGHT;
                 x = area.getRight();
                 break;
             }
           }
         
         switch (justification.getOnlyVerticalFlags())
         {
             case juce::Justification::top: {
                 align |= NVG_ALIGN_TOP;
                 y = area.getY();
                 break;
             }
             case juce::Justification::verticallyCentred: {
                 align |= NVG_ALIGN_MIDDLE;
                 y = area.getCentreY();
                 break;
             }
             case juce::Justification::bottom: {
                 align |= NVG_ALIGN_BOTTOM;
                 y = area.getBottom();
                 break;
             }
           }
           
         
         nvgSave(nvg);
         nvgTextAlign(nvg, align);
         //nvgTextBounds(nvg, x, y, text.toRawUTF8(), nullptr, fbounds);
         //nvgText(nvg, x, y, text.toRawUTF8(), nullptr);
         
         nvgTextBox(nvg, x, y, area.getWidth(), text.toRawUTF8(), nullptr);
         nvgRestore(nvg);

    }
    
    void fillEllipse(juce::Rectangle<float> area) const
    {
        if(!nvg)
        {
            g.fillEllipse(area);
            return;
        }

        
        const auto cx = area.getCentreX();
        const auto cy = area.getCentreY();
        const auto rx = area.getWidth() / 2.0f;
        const auto ry = area.getWidth() / 2.0f;
        
        nvgBeginPath(nvg);
        nvgEllipse(nvg, cx, cy, rx, ry);
        nvgFill(nvg);
    }
    
    void fillEllipse (float x, float y, float w, float h) const
    {
        
        fillEllipse ({x, y, w, h});
    }
    
    void drawEllipse (float x, float y, float width, float height, float lineThickness) const
    {
        drawEllipse ({x, y, width, height}, lineThickness);
    }
    
    void drawEllipse (juce::Rectangle<float> area, float lineThickness) const
    {
        if(!nvg)
        {
            g.drawEllipse(area, lineThickness);
            return;
        }
        
        const auto cx = area.getCentreX();
        const auto cy = area.getCentreY();
        const auto rx = area.getWidth() / 2.0f;
        const auto ry = area.getWidth() / 2.0f;
        
        nvgBeginPath (nvg);
        nvgEllipse (nvg, cx, cy, rx, ry);
        nvgStrokeWidth (nvg, lineThickness);
        nvgStroke (nvg);
    }

    void drawRoundedRectangle (float x, float y, float width, float height,
                                         float cornerSize, float lineThickness) const
    {
        drawRoundedRectangle ({x, y, width, height}, cornerSize, lineThickness);
    }

    void drawRoundedRectangle (juce::Rectangle<float> r, float cornerSize, float lineThickness) const
    {
        if(!nvg)
        {
            g.drawRoundedRectangle(r, cornerSize, lineThickness);
            return;
        }
        
        nvgBeginPath (nvg);
        nvgRoundedRect (nvg, r.getX(), r.getY(), r.getWidth(), r.getHeight(), cornerSize);
        nvgStrokeWidth (nvg, lineThickness);
        nvgStroke (nvg);
    }
    
    void fillRoundedRectangle (float x, float y, float width, float height, float cornerSize) const
    {
        fillRoundedRectangle ({x, y, width, height}, cornerSize);
    }

    void fillRoundedRectangle (juce::Rectangle<float> r, const float cornerSize) const
    {
        if(!nvg)
        {
            g.fillRoundedRectangle(r, cornerSize);
            return;
        }
        
        nvgBeginPath (nvg);
        nvgRoundedRect(nvg, r.getX(), r.getY(), r.getWidth(), r.getHeight(), cornerSize);
        nvgFill(nvg);
    }

    void drawLine (float x1, float y1, float x2, float y2, float lineThickness) const
    {
        drawLine (juce::Line<float>(x1, y1, x2, y2), lineThickness);
    }
   
    void drawLine(juce::Line<float> line, const float lineThickness) const
    {
        if(!nvg)
        {
            g.drawLine(line, lineThickness);
            return;
        }
        
        const float x1 = line.getStartX();
        const float y1 = line.getStartY();
        const float x2 = line.getEndX();
        const float y2 = line.getEndY();

        nvgBeginPath(nvg);
        nvgMoveTo(nvg, x1, y1);
        nvgLineTo(nvg, x2, y2);

        nvgStrokeWidth(nvg, lineThickness);
        nvgStroke(nvg);
    }
    
    //==============================================================================
    void resetToDefaultState()
    {
        g.resetToDefaultState();
    }

    bool reduceClipRegion (juce::Rectangle<int> area)
    {
        return g.reduceClipRegion(area);
    }

    bool reduceClipRegion (int x, int y, int w, int h)
    {
        return reduceClipRegion ({x, y, w, h});
    }

    bool reduceClipRegion (const juce::RectangleList<int>& clipRegion)
    {
        return g.reduceClipRegion(clipRegion);
    }

    bool reduceClipRegion (const juce::Path& path, const juce::AffineTransform& transform)
    {
        return g.reduceClipRegion(path, transform);
    }

    bool reduceClipRegion (const juce::Image& image, const juce::AffineTransform& transform)
    {
        return g.reduceClipRegion(image, transform);
    }

    void excludeClipRegion (juce::Rectangle<int> rectangleToExclude)
    {
        g.excludeClipRegion(rectangleToExclude);
    }

    bool isClipEmpty() const
    {
        return context.isClipEmpty();
    }

    juce::Rectangle<int> getClipBounds() const
    {
        return context.getClipBounds();
    }

    void saveState()
    {
        g.saveState();
    }

    void restoreState()
    {
        g.restoreState();
    }

    void setOrigin (juce::Point<int> newOrigin)
    {
        g.setOrigin(newOrigin);
    }

    void setOrigin (int x, int y)
    {
        setOrigin ({ x, y });
    }

    void addTransform (const juce::AffineTransform& transform)
    {
        g.addTransform(transform);
    }

    bool clipRegionIntersects (juce::Rectangle<int> area) const
    {
        return g.clipRegionIntersects(area);
    }

    void beginTransparencyLayer (float layerOpacity)
    {
        g.beginTransparencyLayer(layerOpacity);
    }

    void endTransparencyLayer()
    {
        g.endTransparencyLayer();
    }

    //==============================================================================
    void setColour (juce::Colour newColour)
    {
        g.setColour(newColour);
    }

    void setOpacity (float newOpacity)
    {
        g.setOpacity(newOpacity);
    }

    void setGradientFill (const juce::ColourGradient& gradient)
    {
        setFillType (gradient);
    }

    void setGradientFill (juce::ColourGradient&& gradient)
    {
        setFillType (std::move (gradient));
    }

    void setTiledImageFill (const juce::Image& imageToUse, const int anchorX, const int anchorY, const float opacity)
    {
        g.setTiledImageFill(imageToUse, anchorX, anchorY, opacity);
    }

    void setFillType (const juce::FillType& newFill)
    {
        g.setFillType(newFill);
    }

    //==============================================================================
    void setFont (const juce::Font& newFont)
    {
        g.setFont(newFont);
    }

    void setFont (const float newFontHeight)
    {
        setFont (context.getFont().withHeight (newFontHeight));
    }

    juce::Font getCurrentFont() const
    {
        return context.getFont();
    }
    void fillRect (juce::Rectangle<int> r) const
    {
        context.fillRect (r, false);
    }

    void fillRect (juce::Rectangle<float> r) const
    {
        context.fillRect (r);
    }

    void fillRect (int x, int y, int width, int height) const
    {
        context.fillRect ({x, y, width, height}, false);
    }

    void fillRect (float x, float y, float width, float height) const
    {
        fillRect(juce::Rectangle<float>(x, y, width, height));
    }

    void fillRectList (const juce::RectangleList<float>& rectangles) const
    {
        context.fillRectList (rectangles);
    }

    void fillRectList (const juce::RectangleList<int>& rects) const
    {
        for (auto& r : rects)
            context.fillRect (r, false);
    }

    void fillAll() const
    {
        context.fillAll();
    }

    void fillAll (juce::Colour colourToUse) const
    {
        if (! colourToUse.isTransparent())
        {
            context.saveState();
            context.setFill (colourToUse);
            context.fillAll();
            context.restoreState();
        }
    }

    //==============================================================================
    void drawRect (float x, float y, float width, float height, float lineThickness) const
    {
        drawRect({x, y, width, height}, lineThickness);
    }

    void drawRect (int x, int y, int width, int height, int lineThickness) const
    {
        drawRect ({x, y, width, height}, lineThickness);
    }

    void drawRect (juce::Rectangle<int> r, int lineThickness) const
    {
        drawRect (r.toFloat(), (float) lineThickness);
    }

    void drawRect (juce::Rectangle<float> r, const float lineThickness) const
    {
        g.drawRect(r, lineThickness);
    }

    void drawVerticalLine (const int x, float top, float bottom) const
    {
        if (top < bottom)
            context.fillRect (juce::Rectangle<float> ((float) x, top, 1.0f, bottom - top));
    }

    void drawHorizontalLine(const int y, float left, float right) const
    {
        if (left < right)
            context.fillRect (juce::Rectangle<float> (left, (float) y, right - left, 1.0f));
    }

    void drawLine(juce::Line<float> line) const
    {
        context.drawLine (line);
    }
    
    //==============================================================================
    void drawImageAt (const juce::Image& imageToDraw, int x, int y, bool fillAlphaChannel) const
    {
        drawImageTransformed (imageToDraw,
                              juce::AffineTransform::translation ((float) x, (float) y),
                              fillAlphaChannel);
    }

    void drawImage (const juce::Image& imageToDraw, juce::Rectangle<float> targetArea,
                    juce::RectanglePlacement placementWithinTarget, bool fillAlphaChannelWithCurrentBrush) const
    {
        if (imageToDraw.isValid())
            drawImageTransformed (imageToDraw,
                                  placementWithinTarget.getTransformToFit (imageToDraw.getBounds().toFloat(), targetArea),
                                  fillAlphaChannelWithCurrentBrush);
    }

    void drawImageWithin (const juce::Image& imageToDraw, int dx, int dy, int dw, int dh,
                          juce::RectanglePlacement placementWithinTarget, bool fillAlphaChannelWithCurrentBrush) const
    {
        drawImage (imageToDraw,  juce::Rectangle<float>(dx, dy, dw, dh),
                   placementWithinTarget, fillAlphaChannelWithCurrentBrush);
    }

    void drawImage (const juce::Image& imageToDraw,
                              int dx, int dy, int dw, int dh,
                              int sx, int sy, int sw, int sh,
                              const bool fillAlphaChannelWithCurrentBrush) const
    {
        g.drawImage(imageToDraw, dx, dy, dw, dh, sx, sy, sw, sh, fillAlphaChannelWithCurrentBrush);
    }

    void drawImageTransformed (const juce::Image& imageToDraw,
                                         const juce::AffineTransform& transform,
                                         const bool fillAlphaChannelWithCurrentBrush) const
    {
        g.drawImageTransformed(imageToDraw, transform, fillAlphaChannelWithCurrentBrush);
    }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NVGGraphics)
};
