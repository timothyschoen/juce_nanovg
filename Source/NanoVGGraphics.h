#pragma once
#include <stack>
#include <juce_gui_basics/juce_gui_basics.h>
#include "NanoVGGraphicsStructs.h"


class NanoVGGraphics
    : private juce::ComponentListener
    , public juce::Timer
    , public ComponentLayer
{
public:
    NanoVGGraphics(juce::Component&);
    ~NanoVGGraphics() override;

    void componentMovedOrResized (juce::Component& component, bool wasMoved, bool wasResized) override;
    void timerCallback() override;

    virtual void contextCreated(NVGcontext*) {}

    NVGcontext* getContext() const { return nvg; }

    /** Create an IGraphics layer. Switches drawing to an offscreen bitmap for drawing
     * IControl* pOwner The control that owns the layer
     * @param r The bounds of the layer within the IGraphics context
     * @param cacheable Used to make sure the underlying bitmap can be shared between plug-in instances */
    void startLayer(ComponentLayer* pControl, const Rect& r);
    /** End an IGraphics layer. Switches drawing back to the main context
     * @return ILayerPtr a pointer to the layer, which should be kept around in order to draw it */
    Layer::Ptr endLayer();
    void pushLayer(Layer*);
    Layer* popLayer();
    void updateLayer();
    /** Test to see if a layer needs drawing, for instance if the control's bounds were changed
     * @param layer The layer to check
     * @return \c true if the layer needs to be updated */
    bool checkLayer(const Layer::Ptr& layer);
    /** Draw a layer to the main IGraphics context
     * @param layer The layer to draw
     * @param pBlend Optional blend method */
    void drawLayer(const Layer::Ptr& layer, const Blend* pBlend = nullptr);
    /** Draw a bitmap (raster) image to the graphics context
     * @param bitmap The bitmap image to draw to the graphics context
     * @param bounds The rectangular region to draw the image in
     * @param srcX The X offset in the source image to draw from
     * @param srcY The Y offset in the source image to draw from
     * @param pBlend Optional blend method */
    void drawBitmap(const APIBitmap* bitmap, const Rect& bounds, int srcX, int srcY, const Blend* pBlend = nullptr);

    // eg. 2 for mac retina, 1.5 for windows
    float getScreenScale() { return scale; }
    // scale deviation from  default width and height i.e stretching the UI by dragging bottom right hand corner
    float getDrawScale() { return drawScale; }

    APIBitmap* createBitmap(int width, int height, float scale, float drawScale_);

    void deleteFBO(NVGframebuffer* pBuffer);
protected:
    void render();
private:
    void onViewDestroyed();

    void clearFBOStack();

    bool initialise();
    void beginFrame();
    void endFrame();

    juce::Component& attachedComponent;

    NVGcontext* nvg = nullptr;
    NVGframebuffer* mainFrameBuffer = nullptr;
    int windowWidth = 0, windowHeight = 0;
    float scale = 1.0f, drawScale = 1.0f;
    bool inDraw = false;

    std::stack<Layer*> layers;

    juce::CriticalSection FBOLock;
    std::stack<NVGframebuffer*> FBOStack; // A stack of FBOs that requires freeing at the end of the frame

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NanoVGGraphics)
};
