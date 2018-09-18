//
// Created by christoph on 12.09.18.
//

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
    if (XrmGetResource(database, "Xft.dpi", "String", &type, &value) && value.addr) {
        float dpi = atof(value.addr);
        return dpi/96.0f;
    }
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
        const char *subsystem = "Unknown Window System";
        switch (wminfo.subsystem) {
            case SDL_SYSWM_X11:
#if defined(SDL_VIDEO_DRIVER_X11)
                scaleFactorHiDPI = getScreenScalingX11(wminfo.info.x11.display);
#endif
                break;
            case SDL_SYSWM_WINDOWS: // TODO
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

}