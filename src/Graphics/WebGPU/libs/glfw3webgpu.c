/**
 * This is an extension of GLFW for WebGPU, abstracting away the details of
 * OS-specific operations.
 * 
 * This file is part of the "Learn WebGPU for C++" book.
 *   https://eliemichel.github.io/LearnWebGPU
 * 
 * Most of this code comes from the wgpu-native triangle example:
 *   https://github.com/gfx-rs/wgpu-native/blob/master/examples/triangle/main.c
 * 
 * MIT License
 * Copyright (c) 2022-2025 Elie Michel and the wgpu-native authors
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "glfw3webgpu.h"

#include <webgpu/webgpu.h>

#include <GLFW/glfw3.h>

#ifdef __EMSCRIPTEN__
#define GLFW_EXPOSE_NATIVE_EMSCRIPTEN
#ifndef GLFW_PLATFORM_EMSCRIPTEN // not defined in older versions of emscripten
#define GLFW_PLATFORM_EMSCRIPTEN 0
#endif
#else // __EMSCRIPTEN__
#ifdef GLFW_SUPPORTS_X11
#define GLFW_EXPOSE_NATIVE_X11
#endif
#ifdef GLFW_SUPPORTS_WAYLAND
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif
#ifdef GLFW_SUPPORTS_COCOA
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#ifdef GLFW_SUPPORTS_WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#if (GLFW_VERSION_MAJOR <= 3 && GLFW_VERSION_MINOR < 4) && !defined(__linux__) && !defined(_WIN32) && !defined(GLFW_SUPPORTS_COCOA) && !defined(__APPLE__)
#include <Utils/File/Logfile.hpp>
#endif
#endif // __EMSCRIPTEN__

#ifdef GLFW_EXPOSE_NATIVE_COCOA
#include <Foundation/Foundation.h>
#include <QuartzCore/CAMetalLayer.h>
#endif

#ifndef __EMSCRIPTEN__
#include <GLFW/glfw3native.h>
#endif

WGPUSurface glfwGetWGPUSurface(WGPUInstance instance, GLFWwindow* window) {
#if (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 4) || GLFW_VERSION_MAJOR > 3

#ifndef __EMSCRIPTEN__
    switch (glfwGetPlatform()) {
#else
    // glfwGetPlatform is not available in older versions of emscripten
    switch (GLFW_PLATFORM_EMSCRIPTEN) {
#endif

#ifdef GLFW_EXPOSE_NATIVE_X11
    case GLFW_PLATFORM_X11: {
        Display* x11_display = glfwGetX11Display();
        Window x11_window = glfwGetX11Window(window);

        WGPUSurfaceSourceXlibWindow fromXlibWindow;
        fromXlibWindow.chain.sType = WGPUSType_SurfaceSourceXlibWindow;
        fromXlibWindow.chain.next = NULL;
        fromXlibWindow.display = x11_display;
        fromXlibWindow.window = x11_window;

        WGPUSurfaceDescriptor surfaceDescriptor;
        surfaceDescriptor.nextInChain = &fromXlibWindow.chain;
        surfaceDescriptor.label = (WGPUStringView){ NULL, WGPU_STRLEN };

        return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
    }
#endif // GLFW_EXPOSE_NATIVE_X11

#ifdef GLFW_EXPOSE_NATIVE_WAYLAND
    case GLFW_PLATFORM_WAYLAND: {
        struct wl_display* wayland_display = glfwGetWaylandDisplay();
        struct wl_surface* wayland_surface = glfwGetWaylandWindow(window);

        WGPUSurfaceSourceWaylandSurface fromWaylandSurface;
        fromWaylandSurface.chain.sType = WGPUSType_SurfaceSourceWaylandSurface;
        fromWaylandSurface.chain.next = NULL;
        fromWaylandSurface.display = wayland_display;
        fromWaylandSurface.surface = wayland_surface;

        WGPUSurfaceDescriptor surfaceDescriptor;
        surfaceDescriptor.nextInChain = &fromWaylandSurface.chain;
        surfaceDescriptor.label = (WGPUStringView){ NULL, WGPU_STRLEN };

        return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
    }
#endif // GLFW_EXPOSE_NATIVE_WAYLAND

#ifdef GLFW_EXPOSE_NATIVE_COCOA
    case GLFW_PLATFORM_COCOA: {
        id metal_layer = [CAMetalLayer layer];
        NSWindow* ns_window = glfwGetCocoaWindow(window);
        [ns_window.contentView setWantsLayer : YES] ;
        [ns_window.contentView setLayer : metal_layer] ;

        WGPUSurfaceSourceMetalLayer fromMetalLayer;
        fromMetalLayer.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
        fromMetalLayer.chain.next = NULL;
        fromMetalLayer.layer = metal_layer;

        WGPUSurfaceDescriptor surfaceDescriptor;
        surfaceDescriptor.nextInChain = &fromMetalLayer.chain;
        surfaceDescriptor.label = (WGPUStringView){ NULL, WGPU_STRLEN };

        return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
    }
#endif // GLFW_EXPOSE_NATIVE_COCOA

#ifdef GLFW_EXPOSE_NATIVE_WIN32
    case GLFW_PLATFORM_WIN32: {
        HWND hwnd = glfwGetWin32Window(window);
        HINSTANCE hinstance = GetModuleHandle(NULL);

        WGPUSurfaceSourceWindowsHWND fromWindowsHWND;
        fromWindowsHWND.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
        fromWindowsHWND.chain.next = NULL;
        fromWindowsHWND.hinstance = hinstance;
        fromWindowsHWND.hwnd = hwnd;

        WGPUSurfaceDescriptor surfaceDescriptor;
        surfaceDescriptor.nextInChain = &fromWindowsHWND.chain;
        surfaceDescriptor.label = (WGPUStringView){ NULL, WGPU_STRLEN };

        return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
    }
#endif // GLFW_EXPOSE_NATIVE_X11

#ifdef GLFW_EXPOSE_NATIVE_EMSCRIPTEN
    case GLFW_PLATFORM_EMSCRIPTEN: {
        WGPUSurfaceDescriptor surfaceDescriptor;
#ifdef WEBGPU_BACKEND_EMDAWNWEBGPU
        WGPUEmscriptenSurfaceSourceCanvasHTMLSelector fromCanvasHTMLSelector;
        fromCanvasHTMLSelector.chain.sType = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector;
        fromCanvasHTMLSelector.selector.data = "canvas";
        fromCanvasHTMLSelector.selector.length = strlen(fromCanvasHTMLSelector.selector.data);
        surfaceDescriptor.label.data = NULL;
        surfaceDescriptor.label.length = 0;
#else
        WGPUSurfaceDescriptorFromCanvasHTMLSelector fromCanvasHTMLSelector;
        fromCanvasHTMLSelector.chain.sType = WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector;
        fromCanvasHTMLSelector.selector = "canvas";
        surfaceDescriptor.label = NULL;
#endif
        fromCanvasHTMLSelector.chain.next = NULL;
        surfaceDescriptor.nextInChain = &fromCanvasHTMLSelector.chain;

        return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
    }
#endif // GLFW_EXPOSE_NATIVE_X11

    default:
        // Unsupported platform
        return NULL;
    }

#else // GLFW_VERSION_MAJOR <= 3 && GLFW_VERSION_MINOR < 4

#if defined(__linux__)
    Display* x11_display = glfwGetX11Display();
    Window x11_window = glfwGetX11Window(window);

    WGPUSurfaceSourceXlibWindow fromXlibWindow;
    fromXlibWindow.chain.sType = WGPUSType_SurfaceSourceXlibWindow;
    fromXlibWindow.chain.next = NULL;
    fromXlibWindow.display = x11_display;
    fromXlibWindow.window = x11_window;

    WGPUSurfaceDescriptor surfaceDescriptor;
    surfaceDescriptor.nextInChain = &fromXlibWindow.chain;
    surfaceDescriptor.label = (WGPUStringView){ NULL, WGPU_STRLEN };

    return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
#elif defined(_WIN32)
    HWND hwnd = glfwGetWin32Window(window);
    HINSTANCE hinstance = GetModuleHandle(NULL);

    WGPUSurfaceSourceWindowsHWND fromWindowsHWND;
    fromWindowsHWND.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
    fromWindowsHWND.chain.next = NULL;
    fromWindowsHWND.hinstance = hinstance;
    fromWindowsHWND.hwnd = hwnd;

    WGPUSurfaceDescriptor surfaceDescriptor;
    surfaceDescriptor.nextInChain = &fromWindowsHWND.chain;
    surfaceDescriptor.label = (WGPUStringView){ NULL, WGPU_STRLEN };

    return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
#elif defined(__APPLE__)
    id metal_layer = [CAMetalLayer layer];
    NSWindow* ns_window = glfwGetCocoaWindow(window);
    [ns_window.contentView setWantsLayer : YES] ;
    [ns_window.contentView setLayer : metal_layer] ;

    WGPUSurfaceSourceMetalLayer fromMetalLayer;
    fromMetalLayer.chain.sType = WGPUSType_SurfaceSourceMetalLayer;
    fromMetalLayer.chain.next = NULL;
    fromMetalLayer.layer = metal_layer;

    WGPUSurfaceDescriptor surfaceDescriptor;
    surfaceDescriptor.nextInChain = &fromMetalLayer.chain;
    surfaceDescriptor.label = (WGPUStringView){ NULL, WGPU_STRLEN };

    return wgpuInstanceCreateSurface(instance, &surfaceDescriptor);
#else
    sgl::Logfile::get()->throwError("Error in glfwGetWGPUSurface: GLFW < 3.4, but unsupported backend.");
#endif

#endif
}
