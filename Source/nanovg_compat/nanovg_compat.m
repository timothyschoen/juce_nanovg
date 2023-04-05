#define NANOVG_METAL_IMPLEMENTATION

#import "nanovg_compat.h"
#import "nanovg_mtl.m"

#import <AppKit/AppKit.h>

void mnvgSetViewBounds(void* view, int width, int height) {
    [(CAMetalLayer*)[(__bridge NSView*)view layer] setDrawableSize:CGSizeMake(width, height)];
}

NVGcontext* mnvgCreateContext(void* view, int flags, int width, int height) {

    ((__bridge NSView*) view).layer = [CAMetalLayer new];
    
    [(CAMetalLayer*)[((__bridge NSView*) view) layer] setPixelFormat:MTLPixelFormatBGRA8Unorm];
    ((CAMetalLayer*) ((__bridge NSView*) view).layer).device = MTLCreateSystemDefaultDevice();
    
    [(CAMetalLayer*)[(__bridge NSView*)view layer] setDrawableSize:CGSizeMake(width, height)];
    
    return nvgCreateMTL((__bridge void*)((__bridge NSView*) view).layer, flags);
}
