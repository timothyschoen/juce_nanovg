#pragma once
#include "nanovg_compat/nanovg_compat.h"

#include <juce_graphics/juce_graphics.h>


class ComponentLayer;
class NanoVGGraphics;


struct Rect
{
    Rect(): L(0), T(0), R(0), B(0) {}
    Rect(float l, float t, float r, float b): L(l), T(t), R(r), B(b) {}
    Rect(const juce::Rectangle<float>& r): Rect(r.getX(), r.getRight(), r.getRight(), r.getBottom()) {}

    bool operator==(const Rect& other) const { return (L == other.L && T == other.T && R == other.R && B == other.B); }
    bool operator!=(const Rect& other) const { return !(*this == other); }
    bool isEmpty() const noexcept { return (L == 0.f && T == 0.f && R == 0.f && B == 0.f); }

    float L, T, R, B;
    void align()
    {
        L = std::floor(L); T = std::floor(T);
        R = std::ceil(R);  B = std::ceil(B);
    }
    Rect getAligned() const
    {
        Rect r = *this;
        r.align();
        return r;
    }
    /** Create a new IRECT that is the intersection of this IRECT and `rhs`.
     * The resulting IRECT will have the maximum L and T values and minimum R and B values of the inputs.
     * @param rhs another IRECT
     * @return IRECT the new IRECT  */
    inline Rect Intersect(const Rect& other) const
    {
        if (intersects(other))
            return Rect(std::max<float>(L, other.L), std::max<float>(T, other.T), std::min<float>(R, other.R), std::min<float>(B, other.B));

        return Rect();
    }
    /** Returns true if this IRECT shares any common pixels with `rhs`, false otherwise.
     * @param rhs another IRECT
     * @return true this IRECT shares any common space with `rhs`
     * @return false this IRECT and `rhs` are completely separate  */
    inline bool intersects(const Rect& other) const { return (!isEmpty() && !other.isEmpty() && R >= other.L && L < other.R && B >= other.T && T < other.B); }
    float W() const { return R - L; }
    float H() const { return B - T; }
    float CY() const { return 0.5f * (T + B); }
    float CX() const { return 0.5f * (R + L); }
};

/** Used to manage composite/blend operations, independent of draw class/platform */
struct Blend
{
    /** @enum BlendType Porter-Duff blend mode/compositing operators */
    enum BlendType
    {
        SrcOver,
        SrcIn,
        SrcOut,
        SrcAtop,
        DstOver,
        DstIn,
        DstOut,
        DstAtop,
        Add,
        XOR,
        Default = SrcOver
    };
    BlendType method;
    float weight;

    /** Creates a new Blend
     * @param type Blend type (defaults to none)
     * @param weight normalised alpha blending amount */
    Blend(BlendType type = Default, float weight_ = 1.0f)
        : method(type)
        , weight(std::clamp(weight_, 0.0f, 1.0f))
    {}

    static inline constexpr float blendWeight(const Blend* pBlend) { return (pBlend ? pBlend->weight : 1.0f); }
};

class APIBitmap
{
public:
    APIBitmap(NanoVGGraphics& g, int width_, int height_, float scale_, float drawScale_);
    ~APIBitmap();

    APIBitmap (const APIBitmap&) = delete;
    APIBitmap& operator= (const APIBitmap&) = delete;

    int width = 0, height = 0;
    float scale = 1, drawScale = 1;

    NVGframebuffer* getFBO() const { return FBO; }
    // NanoVG texture ID. Lives on GPU
    int getImageId() const { return FBO->image; }
private:
    NanoVGGraphics& graphics;
    NVGframebuffer* FBO = nullptr;
};

class Layer
{
public:
    using Ptr = std::unique_ptr<Layer>;

    Layer(APIBitmap* bmp, const Rect& layerRect, ComponentLayer* comp);

    Rect getBounds() { return rect; }
    void invalidate() { invalid = true; }

    std::unique_ptr<APIBitmap> bitmap;
    ComponentLayer* component;
    Rect componentRect;
    Rect rect;
    bool invalid;
};


class ComponentLayer
{
public:
    ComponentLayer() = default;
    ~ComponentLayer() = default;

    virtual void drawCachable(NanoVGGraphics&) {}
    virtual void drawAnimated(NanoVGGraphics&) {}

    Layer::Ptr layer;
    Rect bounds;
    Blend blend;
    bool useLayer = false;

protected:
    void draw(NanoVGGraphics& g);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ComponentLayer)
};
