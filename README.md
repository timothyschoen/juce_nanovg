# juce_nanovg

[NanoVG](https://github.com/memononen/nanovg) graphics module for [JUCE](https://github.com/juce-framework/JUCE), based on https://github.com/Archie3d/juce_bgfx

Contains a LowLevelGraphicsContext and attachable component for rendering JUCE projects with NanoVG. See the example to find out how to implement this yourself.

A work-in-progress, currently only works on MacOS with Metal and Windows with DirectX 11. I expect to add better Linux support at some point, but if you have experience with NanoVG, OpenGL or Vulkan, let me know!

## iGraphics

If you are familiar with the iGraphics class from [iPlug2](https://github.com/iPlug2/iPlug2), you may be interested in the stripped back implementation found [here](Source/NanoVGGraphics.h).

## Framebuffers

A framebuffer is an image/texture living in your GPUs RAM.

If you prefer to draw shapes solely using the NanoVG API, you will quickly find your GPU usage shoots up from all the draw calls.

In that case, it may be helpful to cache the static parts of your GUI by painting to an image or _framebuffer_. This way you can instead paint the framebuffer on every frame.

To use framebuffers, call `nvgCreateFramebuffer` and store the returned pointer on your class (remember to call `nvgDeleteFramebuffer` in your class destructor!). You will need to delete/create this buffer every time your GUI resizes, so be careful how many of these you use. Next call `nvgBindFramebuffer(fb)` in order to draw to it with all of your NanoVG draw methods. When you're done drawing, remember to switch back to your main buffer using `nvgBindFramebuffer(mainBuffer)`, or if you don't use a main buffer, use `nvgBindFramebuffer(nullptr)` to set the back buffer as the rendering target.

To draw the framebuffer, use this:

```c++
NVGpaint paint= nvgImagePattern(nvg, x, y, w, h, 0.0f, fb->image, 1.0f);
nvgBeginPath(nvg); // clears existing path cache
nvgRect(nvg, x, y, w, h); // fills the path with a rectangle
nvgFillPaint(nvg, paint); // sets the paint settings to the current state
nvgFill(nvg); // draw call happens here, using the paint settings
```

An implementation of caching with framebuffers is found [here](Source/example/CacheTest.h)

## Screenshots

<img width="712" alt="Screenshot 2022-03-25 at 00 28 24" src="https://user-images.githubusercontent.com/44585538/160026228-2c59e3ec-ce98-4492-af4a-cd9611f912c5.png">

<img width="502" alt="Screenshot 2022-03-25 at 00 28 25" src="https://user-images.githubusercontent.com/44585538/160179153-b2fa2d56-2453-4614-98d6-702d730635f3.png">
