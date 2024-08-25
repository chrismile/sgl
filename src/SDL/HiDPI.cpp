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

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <Utils/AppSettings.hpp>
#include <Utils/File/Logfile.hpp>
#include "SDLWindow.hpp"

#include "HiDPI.hpp"

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

namespace sgl
{

bool getScreenScalingWindows(float& scalingFactor) {
    HDC hdcScreen = GetDC(nullptr); // HWND hWnd
    int dpi = GetDeviceCaps(hdcScreen, LOGPIXELSX);
    ReleaseDC(nullptr, hdcScreen);
    scalingFactor = dpi / 96.0f;
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
    auto* window = static_cast<sgl::SDLWindow*>(sgl::AppSettings::get()->getMainWindow());
    window->errorCheck();
    SDL_Window* sdlWindow = window->getSDLWindow();
    SDL_SysWMinfo wminfo;
    SDL_VERSION(&wminfo.version);
    auto succeeded = SDL_GetWindowWMInfo(sdlWindow, &wminfo);
#ifdef __EMSCRIPTEN__
    // For whatever reason, we get "SDL error: That operation is not supported" after SDL_GetWindowWMInfo.
    window->errorCheckIgnoreUnsupportedOperation();
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
                scaleFactorSetManually = getScreenScalingWindows(scaleFactorHiDPI);
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
    if (!scaleFactorSetManually && (wminfo.subsystem == SDL_SYSWM_WAYLAND || wminfo.subsystem == SDL_SYSWM_COCOA)) {
        if (window->getVirtualWidth() != window->getPixelWidth()) {
            scaleFactorHiDPI = float(window->getPixelWidth()) / float(window->getVirtualWidth());
            scaleFactorSetManually = true;
        }
    }
#endif

    if (!scaleFactorSetManually) {
        // If querying the DPI scaling factor from the OS is not supported, approximate a good screen
        // scaling factor by dividing the vertical dpi (vdpi) of screen #0 by 96.
        // Standard DPI is supposedly 72 on macOS, but fonts seem to be too big in this case.
        if ((SDL_GetWindowFlags(sdlWindow) & SDL_WINDOW_ALLOW_HIGHDPI) != 0) {
            float ddpi = 96, hdpi = 96, vdpi = 96;
            if (SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi) == 0) {
                Logfile::get()->writeInfo(
                        std::string() + "getHighDPIScaleFactor: ddpi: " + toString(ddpi)
                        + ", hdpi: " + toString(hdpi) + ", vdpi: " + toString(vdpi));
                scaleFactorHiDPI = hdpi / 96.0f;
            }
        }
    }

    return scaleFactorHiDPI;
}

void overwriteHighDPIScaleFactor(float scaleFactor) {
    scaleFactorRetrieved = true;
    scaleFactorHiDPI = scaleFactor;
}

}