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
#include <X11/Xatom.h>
#include <X11/Xresource.h>

namespace sgl
{

// Inspired by https://github.com/glfw/glfw/issues/1019
float getScreenScalingX11(Display *display)
{
    char *resourceString = XResourceManagerString(display);
    XrmInitialize();
    XrmDatabase database = XrmGetStringDatabase(resourceString);

    XrmValue value;
    char *type = NULL;
    float scalingFactor = 1.0f;
    if (XrmGetResource(database, "Xft.dpi", "String", &type, &value) && value.addr) {
        scalingFactor = atof(value.addr) / 96.0f;
    }
    XrmDestroyDatabase(database);
    return scalingFactor;
}

}

#elif defined(WIN32) // TODO: _WIN32?
#include <windows.h>

namespace sgl
{

float getScreenScalingWindows()
{
    HDC hdcScreen = GetDC(NULL); // HWND hWnd
    int dpi = GetDeviceCaps(hdcScreen, LOGPIXELSX);
    ReleaseDC(NULL, hdcScreen);
    return dpi/96.0f;
}

}

#endif

namespace sgl
{

static bool scaleFactorRetrieved = false;
static float scaleFactorHiDPI = 1.0f;

float getHighDPIScaleFactor() {
    if (scaleFactorRetrieved) {
        return scaleFactorHiDPI;
    }
    scaleFactorRetrieved = true;

    SDL_Window *window = static_cast<sgl::SDLWindow *>(sgl::AppSettings::get()->getMainWindow())->getSDLWindow();
    SDL_SysWMinfo wminfo;
    SDL_VERSION(&wminfo.version);
    if (SDL_GetWindowWMInfo(window, &wminfo)) {
        switch (wminfo.subsystem) {
            case SDL_SYSWM_X11:
#if defined(SDL_VIDEO_DRIVER_X11)
                scaleFactorHiDPI = getScreenScalingX11(wminfo.info.x11.display);
#endif
                break;
            case SDL_SYSWM_WINDOWS:
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
                scaleFactorHiDPI = getScreenScalingWindows();
#endif
            case SDL_SYSWM_WAYLAND:
            case SDL_SYSWM_ANDROID:
            case SDL_SYSWM_COCOA:
            default:
                float ddpi = 96, hdpi = 96, vdpi = 96;
                if (SDL_GetDisplayDPI(0, &ddpi, &hdpi, &vdpi) == 0) {
                    // If querying the DPI scaling factor from the OS is not supported, approximate a good screen
                    // scaling factor by dividing the vertical dpi (vdpi) of screen #0 by 96.
                    //std::cout << "ddpi: " << ddpi << ", hdpi: " << hdpi << ", vdpi: " << vdpi << std::endl;
                    Logfile::get()->writeInfo(std::string() + "getHighDPIScaleFactor: ddpi: " + toString(ddpi)
                            + ", hdpi: " + toString(hdpi) + ", vdpi: " + toString(vdpi));
                    return hdpi / 96.0f;
                }
                break;
        }
    } else {
        Logfile::get()->writeError(std::string() + "Couldn't get window information: " + SDL_GetError());
    }
    return scaleFactorHiDPI;
}

void overwriteHighDPIScaleFactor(float scaleFactor) {
    scaleFactorRetrieved = true;
    scaleFactorHiDPI = scaleFactor;
}

}