#pragma once
#include "../NanoVGComponent.h"

//==============================================================================

class NVGDemoComponent : public NanoVGComponent
{
public:
    NVGDemoComponent();
    ~NVGDemoComponent() override;

    void mouseMove (const juce::MouseEvent& e) override;
    void contextCreated(NVGcontext*) override;
    void paint (juce::Graphics&) override;

private:
    int loadDemoData(NVGcontext* vg);
    void freeDemoData(NVGcontext* vg);
    void renderDemo(NVGcontext* vg, float mx, float my, float width, float height, float t, int blowup);

    static char* cpToUTF8(int cp, char* str);
    static int isBlack(NVGcolor col);

    //==============================================================================

    void drawEyes(NVGcontext* vg, float x, float y, float w, float h, float mx, float my, float t);
    void drawParagraph(NVGcontext* vg, float x, float y, float width, float height, float mx, float my);
    void drawGraph(NVGcontext* vg, float x, float y, float w, float h, float t);
    void drawColorwheel(NVGcontext* vg, float x, float y, float w, float h, float t);
    void drawLines(NVGcontext* vg, float x, float y, float w, float h, float t);
    void drawWidths(NVGcontext* vg, float x, float y, float width);
    void drawCaps(NVGcontext* vg, float x, float y, float width);
    void drawScissor(NVGcontext* vg, float x, float y, float t);
    void drawWindow(NVGcontext* vg, const char* title, float x, float y, float w, float h);
    void drawSearchBox(NVGcontext* vg, const char* text, float x, float y, float w, float h);
    void drawDropDown(NVGcontext* vg, const char* text, float x, float y, float w, float h);
    void drawLabel(NVGcontext* vg, const char* text, float x, float y, float w, float h);
    void drawEditBoxBase(NVGcontext* vg, float x, float y, float w, float h);
    void drawEditBox(NVGcontext* vg, const char* text, float x, float y, float w, float h);
    void drawEditBoxNum(NVGcontext* vg, const char* text, const char* units, float x, float y, float w, float h);
    void drawCheckBox(NVGcontext* vg, const char* text, float x, float y, float w, float h);
    void drawButton(NVGcontext* vg, int preicon, const char* text, float x, float y, float w, float h, NVGcolor col);
    void drawSlider(NVGcontext* vg, float pos, float x, float y, float w, float h);
    void drawThumbnails(NVGcontext* vg, float x, float y, float w, float h, const int* images, int nimages, float t);
    void drawSpinner(NVGcontext* vg, float cx, float cy, float r, float t);

    //==============================================================================

    enum GraphrenderStyle
    {
        GRAPH_RENDER_FPS,
        GRAPH_RENDER_MS,
        GRAPH_RENDER_PERCENT,
    };

    enum { GRAPH_HISTORY_COUNT = 100 };

    struct PerfGraph
    {
        GraphrenderStyle style;
        char name[32];
        float values[GRAPH_HISTORY_COUNT];
        int head;
    };

    void initGraph(PerfGraph* fps, GraphrenderStyle style, const char* name);
    void updateGraph(PerfGraph* fps, float frameTime);
    void renderGraph(NVGcontext* vg, float x, float y, PerfGraph* fps);
    float getGraphAverage(PerfGraph* fps);

    //==============================================================================

    struct DemoData {
        int fontNormal, fontBold, fontIcons, fontEmoji;
        int images[12];
    };

    class glfwLikeTimer
    {
    public:
        glfwLikeTimer()
        {
            setTime(0);
        }
        void setTime(double offset)
        {
            startTime = getNow() + offset;
        }

        double getTime() { return getNow() - startTime; }
    private:
        double getNow()
        {
            auto ticks = juce::Time::getHighResolutionTicks();
            return juce::Time::highResolutionTicksToSeconds(ticks);
        }
        double startTime = 0;
    };

    float mouseX, mouseY = 0.0f;
    glfwLikeTimer timer;
    double prevTime = 0;
    int blowup = 0;
    DemoData data;

    PerfGraph performanceGraph;

    bool demoDataInitialised = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NVGDemoComponent)
};
