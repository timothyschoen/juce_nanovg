//
//  Copyright (C) 2022 Arthur Benilov <arthur.benilov@gmail.com> and Timothy Schoen <timschoen123@gmail.com>
//

#pragma once
#include "NanoVGGraphicsContext.h"

/**
    JUCE UI component rendered using nanovg

    @note All normal JUCE components placed within this one will be
          rendered with nanovg as well.
*/

class NanoVGComponent :
#if NANOVG_METAL_IMPLEMENTATION
    public juce::Component
#elif NANOVG_GL_IMPLEMENTATION
    public juce::OpenGLAppComponent
#endif
{

    struct ComponentUpdater : public juce::ComponentListener, public juce::Timer
    {
        NanoVGComponent* owner;
        
        ComponentUpdater(NanoVGComponent* parent) : owner(parent)
        {
            owner->addComponentListener(this);
        }
        
        ~ComponentUpdater()
        {
            owner->removeComponentListener(this);
        }
        
        void timerCallback() override;
        
        void componentMovedOrResized(juce::Component& component, bool wasMoved, bool wasResized) override;
    };
    
public:
    NanoVGComponent();
    ~NanoVGComponent() override;

    NanoVGGraphicsContext* getNVGGraphicsContext() noexcept { return nvgGraphicsContext.get(); }

    // Setup anything you need here
    virtual void contextCreated(NVGcontext*) {}
    
    void addFont(const juce::String& name, const char* data, size_t size);

    juce::RectangleList<int> invalidArea;
    
    NVGframebuffer* getMainFramebuffer() { return mainFB; };
    
    void vBlank();
    
private:
    void paintComponent();

    bool currentlyPainting {false};
    bool showRenderStats {false};

    NVGframebuffer* mainFB = nullptr;
    NVGframebuffer* invalidFB = nullptr;

    std::unique_ptr<NanoVGGraphicsContext> nvgGraphicsContext = nullptr;

    void updateRenderBounds();
    
    void initialise();
    void render();
    void shutdown();

    //==========================================================================

    /** Detach the context from the currently attached component. */
    void detach();

#if JUCE_WINDOWS || JUCE_LINUX
    void updateWindowPosition (juce::Rectangle<int> bounds);
#endif

#define RENDER_ON_THREAD 0
    
    class RenderCache
        : public juce::CachedComponentImage
#if RENDER_ON_THREAD
        , private juce::ThreadPool
#endif
    {
    public:

        RenderCache (NanoVGComponent& comp)
        :
#if RENDER_ON_THREAD
        ThreadPool(1),
#endif
        vblank(&comp, [this](){
            onVBlank();
        }),
        component(comp)
        {
        }

        ~RenderCache()
        {
#if RENDER_ON_THREAD
            removeAllJobs(true, 1000);
#endif
        }
        
        void paint (juce::Graphics& g) override
        {
        }

        bool invalidateAll() override
        {
            component.invalidArea = component.getLocalBounds();
            
#if RENDER_ON_THREAD
            addJob([this](){
                component.render();
            });
#endif
            return false;
        }
        
        bool invalidate (const juce::Rectangle<int>& rect) override
        {
            component.invalidArea.add(rect);
            
#if RENDER_ON_THREAD
            addJob([this](){
                component.render();
            });
#endif
            return false;
        }
        
        void onVBlank()
        {
#if !RENDER_ON_THREAD
            component.render();
#endif
            
            component.paintComponent();
        }

        void releaseResources() override
        {
        }

    private:
        
        NanoVGComponent& component;
        juce::VBlankAttachment vblank;
    };


    ComponentUpdater updater = ComponentUpdater(this);

    std::atomic<bool> initialised = false;
    std::atomic<bool> boundsChanged = false;
    std::atomic<bool> needsRepaint = false;
    
    float scale = 1.0f;
};
