/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2018, Christoph Neuhauser
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef SUPPORT_SDL3
#define SDL_ENABLE_OLD_NAMES
#ifdef _WIN32
#define NOMINMAX
#endif
#include <SDL3/SDL.h>
#endif
#ifdef SUPPORT_SDL2
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#endif

#include <Utils/AppSettings.hpp>
#include <Utils/File/Logfile.hpp>
#include <Graphics/Window.hpp>

#ifndef DISABLE_IMGUI
#include <ImGui/ImGuiWrapper.hpp>
#endif

#ifdef SUPPORT_SDL
#include <SDL/SDLWindow.hpp>
#endif

#ifdef SUPPORT_GLFW
#include <GLFW/glfw3.h>

#ifndef __EMSCRIPTEN__
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
#endif // __EMSCRIPTEN__
#include <GLFW/glfw3native.h>
#endif // SUPPORT_GLFW

#include "HiDPI.hpp"

#include "GLFW/GlfwWindow.hpp"

#if defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <dlfcn.h>

namespace sgl
{

// Inspired by https://github.com/glfw/glfw/issues/1019
bool getScreenScalingX11(Display* display, float& scalingFactor) {
    typedef char* (*PFN_XResourceManagerString)(Display*);
    typedef void (*PFN_XrmInitialize)();
    typedef XrmDatabase (*PFN_XrmGetStringDatabase)(_Xconst char*);
    typedef Bool (*PFN_XrmGetResource)(XrmDatabase, _Xconst char*, _Xconst char*, char**, XrmValue*);
    typedef void (*PFN_XrmDestroyDatabase)(XrmDatabase);

    void* libX11so = dlopen("libX11.so", RTLD_NOW | RTLD_LOCAL);
    if (!libX11so) {
        sgl::Logfile::get()->writeError("Error in getScreenScalingX11: Could not load libX11.so!");
        return false;
    }

    auto dyn_XResourceManagerString = PFN_XResourceManagerString(dlsym(libX11so, "XResourceManagerString"));
    auto dyn_XrmInitialize = PFN_XrmInitialize(dlsym(libX11so, "XrmInitialize"));
    auto dyn_XrmGetStringDatabase = PFN_XrmGetStringDatabase(dlsym(libX11so, "XrmGetStringDatabase"));
    auto dyn_XrmGetResource = PFN_XrmGetResource(dlsym(libX11so, "XrmGetResource"));
    auto dyn_XrmDestroyDatabase = PFN_XrmDestroyDatabase(dlsym(libX11so, "XrmDestroyDatabase"));

    if (!dyn_XResourceManagerString || !dyn_XrmInitialize || !dyn_XrmGetStringDatabase || !dyn_XrmGetResource
            || !dyn_XrmDestroyDatabase) {
        sgl::Logfile::get()->writeError("Error in getScreenScalingX11: Could not load all required functions!");
        dlclose(libX11so);
        return false;
    }

    char* resourceString = dyn_XResourceManagerString(display);
    if (!resourceString) {
        sgl::Logfile::get()->writeError(
                "Error in getScreenScalingX11: XResourceManagerString returned that no property exists!");
        dlclose(libX11so);
        return false;
    }
    dyn_XrmInitialize();
    XrmDatabase database = dyn_XrmGetStringDatabase(resourceString);

    XrmValue value;
    char* type = nullptr;
    scalingFactor = 1.0f;
    if (dyn_XrmGetResource(database, "Xft.dpi", "String", &type, &value) && value.addr) {
        scalingFactor = atof(value.addr) / 96.0f;
    }
    dyn_XrmDestroyDatabase(database);
    dlclose(libX11so);

    return true;
}

}
#elif defined(_WIN32)
#include <windows.h>
#include <winuser.h>
#include <VersionHelpers.h>

namespace sgl
{

typedef UINT (__stdcall *PFN_GetDpiForWindow)(HWND hwnd);
static PFN_GetDpiForWindow getDpiForWindowPtr = {};

void setWindowsLibraryHandles(HMODULE user32Module) {
    getDpiForWindowPtr = reinterpret_cast<PFN_GetDpiForWindow>(GetProcAddress(user32Module, "GetDpiForWindow"));
}

bool getScreenScalingWindows(HWND windowHandle, float& scalingFactor) {
    static bool minWin81 = IsWindows8Point1OrGreater();//IsWindowsVersionOrGreater(HIBYTE(0x0603), LOBYTE(0x0603), 0); // IsWindows8Point1OrGreater
    if (minWin81) {
        unsigned windowDpi = getDpiForWindowPtr(windowHandle);
        if (windowDpi == 0) {
            return false;
        }
        scalingFactor = static_cast<float>(windowDpi) / 96.0f;
    } else {
        HDC hdcScreen = GetDC(nullptr); // HWND hWnd
        int dpi = GetDeviceCaps(hdcScreen, LOGPIXELSX);
        ReleaseDC(nullptr, hdcScreen);
        scalingFactor = static_cast<float>(dpi) / 96.0f;
    }
    return true;
}

}
#endif

namespace sgl {

static bool scaleFactorRetrieved = false;
static float scaleFactorHiDPI = 1.0f;

float getHighDPIScaleFactor() {
    if (scaleFactorRetrieved) {
        return scaleFactorHiDPI;
    }
    scaleFactorRetrieved = true;

    bool scaleFactorSetManually = false;

#if defined(SUPPORT_SDL) || defined(SUPPORT_GLFW)
    bool allowHighDPI = false;
#endif
#ifdef __linux__
    bool isWayland = false;
#endif
#ifdef __APPLE__
    bool isCocoa = false;
#endif
    auto* window = sgl::AppSettings::get()->getMainWindow();
#ifdef SUPPORT_SDL2
    if (window->getBackend() == WindowBackend::SDL2_IMPL) {
        window->errorCheck();
        SDL_Window* sdlWindow = static_cast<sgl::SDLWindow*>(window)->getSDLWindow();
        SDL_SysWMinfo wminfo;
        SDL_VERSION(&wminfo.version);
        auto succeeded = SDL_GetWindowWMInfo(sdlWindow, &wminfo);
#ifdef __EMSCRIPTEN__
        // For whatever reason, we get "SDL error: That operation is not supported" after SDL_GetWindowWMInfo.
        static_cast<sgl::SDLWindow*>(window)->errorCheckIgnoreUnsupportedOperation();
#endif
        if (succeeded) {
            switch (wminfo.subsystem) {
                case SDL_SYSWM_X11:
#if defined(SDL_VIDEO_DRIVER_X11)
                    scaleFactorSetManually = getScreenScalingX11(wminfo.info.x11.display, scaleFactorHiDPI);
#endif
                    break;
                case SDL_SYSWM_WINDOWS:
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
                    scaleFactorSetManually = getScreenScalingWindows(wminfo.info.win.window, scaleFactorHiDPI);
#endif
                    break;
                case SDL_SYSWM_WAYLAND:
                case SDL_SYSWM_COCOA:
                case SDL_SYSWM_ANDROID:
                default:
                    // No special functions so far; use fallback of screen DPI below.
                    break;
            }
        } else {
            Logfile::get()->writeError(std::string() + "Couldn't get window information: " + SDL_GetError());
        }

#ifdef __linux__
        isWayland = wminfo.subsystem == SDL_SYSWM_WAYLAND;
#endif
#ifdef __APPLE__
        isCocoa = wminfo.subsystem == SDL_SYSWM_COCOA;
#endif
        allowHighDPI = (SDL_GetWindowFlags(sdlWindow) & SDL_WINDOW_ALLOW_HIGHDPI) != 0;
    }
#endif

#ifdef SUPPORT_SDL3
    if (window->getBackend() == WindowBackend::SDL3_IMPL) {
        window->errorCheck();
        SDL_Window* sdlWindow = static_cast<sgl::SDLWindow*>(window)->getSDLWindow();
#ifdef SDL_PLATFORM_LINUX
        if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
            auto* xdisplay = static_cast<Display*>(SDL_GetPointerProperty(
                    SDL_GetWindowProperties(sdlWindow), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr));
            scaleFactorSetManually = getScreenScalingX11(xdisplay, scaleFactorHiDPI);
        }
#endif
#ifdef SDL_PLATFORM_WIN32
        auto windowHandle = static_cast<HWND>(SDL_GetPointerProperty(
               SDL_GetWindowProperties(sdlWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
        scaleFactorSetManually = getScreenScalingWindows(windowHandle, scaleFactorHiDPI);
#endif
#ifdef __linux__
        isWayland = SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0;
#endif
#ifdef __APPLE__
        isCocoa = true;
#endif
        allowHighDPI = (SDL_GetWindowFlags(sdlWindow) & SDL_WINDOW_HIGH_PIXEL_DENSITY) != 0;
    }
#endif

#ifdef SUPPORT_GLFW
    if (window->getBackend() == WindowBackend::GLFW_IMPL) {
#if (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 4) || GLFW_VERSION_MAJOR > 3

#if defined(GLFW_EXPOSE_NATIVE_X11) || defined(GLFW_EXPOSE_NATIVE_WIN32) || defined(GLFW_EXPOSE_NATIVE_WAYLAND) || defined(GLFW_EXPOSE_NATIVE_COCOA)
        auto glfwPlatform = glfwGetPlatform();
#endif
#ifdef GLFW_EXPOSE_NATIVE_X11
        if (glfwPlatform == GLFW_PLATFORM_X11) {
            Display* x11_display = glfwGetX11Display();
            scaleFactorSetManually = getScreenScalingX11(x11_display, scaleFactorHiDPI);
        }
#endif
#ifdef GLFW_EXPOSE_NATIVE_WIN32
        if (glfwPlatform == GLFW_PLATFORM_WIN32) {
            HWND windowHandle = glfwGetWin32Window(static_cast<sgl::GlfwWindow*>(window)->getGlfwWindow());
            scaleFactorSetManually = getScreenScalingWindows(windowHandle, scaleFactorHiDPI);
        }
#endif
#ifdef GLFW_EXPOSE_NATIVE_WAYLAND
        isWayland = glfwPlatform == GLFW_PLATFORM_WAYLAND;
#endif
#ifdef GLFW_EXPOSE_NATIVE_COCOA
        isCocoa = glfwPlatform == GLFW_PLATFORM_COCOA;
        allowHighDPI = true;
#endif

#else // GLFW_VERSION_MAJOR <= 3 && GLFW_VERSION_MINOR < 4

#if defined(__linux__)
        /*
         * For GLFW pre-3.4, we assume that X11 is used as the window management system and not Wayland.
         */
        Display* x11_display = glfwGetX11Display();
        scaleFactorSetManually = getScreenScalingX11(x11_display, scaleFactorHiDPI);
#elif defined(_WIN32)
        HWND windowHandle = glfwGetWin32Window(static_cast<sgl::GlfwWindow*>(window)->getGlfwWindow());
        scaleFactorSetManually = getScreenScalingWindows(windowHandle, scaleFactorHiDPI);
#elif defined(__APPLE__)
        isCocoa = true;
        allowHighDPI = true;
#endif

#endif // (GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 4) || GLFW_VERSION_MAJOR > 3
    }
#endif // SUPPORT_GLFW

#ifdef __linux__
    if (!scaleFactorSetManually) {
        const char* gdkScaleVar = getenv("GDK_SCALE");
        if (gdkScaleVar) {
            try {
                scaleFactorHiDPI = std::stof(gdkScaleVar);
                scaleFactorSetManually = true;
            } catch(std::invalid_argument& e) {}
        }
    }
    if (!scaleFactorSetManually) {
        const char* qtScaleFactorVar = getenv("QT_SCALE_FACTOR");
        if (qtScaleFactorVar) {
            try {
                scaleFactorHiDPI = std::stof(qtScaleFactorVar);
                scaleFactorSetManually = true;
            } catch(std::invalid_argument& e) {}
        }
    }
    bool isWaylandOrCocoa = isWayland;
#endif
#ifdef __APPLE__
    bool isWaylandOrCocoa = isCocoa;
#endif
#if defined(__linux__) || defined(__APPLE__)
    if (!scaleFactorSetManually && isWaylandOrCocoa) {
        if (window->getVirtualWidth() != window->getPixelWidth()) {
            scaleFactorHiDPI = float(window->getPixelWidth()) / float(window->getVirtualWidth());
            scaleFactorSetManually = true;
        }
    }
#endif

#ifdef SUPPORT_SDL2
    if (window->getBackend() == WindowBackend::SDL2_IMPL && !scaleFactorSetManually) {
        // If querying the DPI scaling factor from the OS is not supported, approximate a good screen
        // scaling factor by dividing the vertical dpi (vdpi) of screen #0 by 96.
        // Standard DPI is supposedly 72 on macOS, but fonts seem to be too big in this case.
        if (allowHighDPI) {
            float ddpi = 96, hdpi = 96, vdpi = 96;
            if (SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi) == 0) {
                Logfile::get()->writeInfo(
                        std::string() + "getHighDPIScaleFactor: ddpi: " + toString(ddpi)
                        + ", hdpi: " + toString(hdpi) + ", vdpi: " + toString(vdpi));
                scaleFactorHiDPI = hdpi / 96.0f;
            }
        }
    }
#endif

#ifdef SUPPORT_SDL3
    if (window->getBackend() == WindowBackend::SDL3_IMPL && !scaleFactorSetManually && allowHighDPI) {
        scaleFactorHiDPI = SDL_GetWindowDisplayScale(static_cast<SDLWindow*>(window)->getSDLWindow());
    }
#endif

#ifdef SUPPORT_GLFW
    if (window->getBackend() == WindowBackend::GLFW_IMPL && !scaleFactorSetManually) {
        float xScale = 1.0f, yScale = 1.0f;
        glfwGetMonitorContentScale(glfwGetPrimaryMonitor(), &xScale, &yScale);
        scaleFactorHiDPI = std::min(xScale, yScale);
    }
#endif

    return scaleFactorHiDPI;
}

void overwriteHighDPIScaleFactor(float scaleFactor) {
    scaleFactorRetrieved = true;
    scaleFactorHiDPI = scaleFactor;
}

void updateHighDPIScaleFactor() {
    float scaleFactorOld = scaleFactorHiDPI;
    scaleFactorRetrieved = false;
    float scaleFactor = getHighDPIScaleFactor();
    if (std::abs(scaleFactorOld - scaleFactor) < 0.01f) {
        scaleFactorHiDPI = scaleFactorOld;
    }
#ifndef DISABLE_IMGUI
    else {
        sgl::ImGuiWrapper::get()->updateMainWindowScaleFactor(scaleFactor);
    }
#endif
}

}