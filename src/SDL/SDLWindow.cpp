/*
 * SDLWindow.cpp
 *
 *  Created on: 27.08.2017
 *      Author: Christoph Neuhauser
 */


#include "SDLWindow.hpp"
#include "Input/SDLMouse.hpp"
#include "Input/SDLKeyboard.hpp"
#include "Input/SDLGamepad.hpp"
#include <Math/Math.hpp>
#include <Utils/AppSettings.hpp>
#include <Utils/File/FileUtils.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/AppSettings.hpp>
#include <Utils/Timer.hpp>
#include <Utils/Events/EventManager.hpp>
#include <Graphics/Texture/Bitmap.hpp>
#include <cstdlib>
#include <ctime>
#include <boost/algorithm/string/predicate.hpp>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#ifdef __linux__
#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>
#endif


namespace sgl {

SDLWindow::SDLWindow() : sdlWindow(NULL), glContext(NULL)
{
}

void SDLWindow::errorCheck()
{
    while (SDL_GetError()[0] != '\0') {
        Logfile::get()->writeError(std::string() + "SDL error: " + SDL_GetError());
        SDL_ClearError();
    }
}

// Query the numbers of multisample samples possible (given a maximum number of desired samples)
int getMaxSamplesGLImpl(int desiredSamples) {
#ifdef __linux__
    const char *displayName = ":0";
    Display *display;
    if (!(display = XOpenDisplay(displayName))) {
        Logfile::get()->writeError("Couldn't open X11 display");
    }
    int defscreen = DefaultScreen(display);

    int nitems;
    GLXFBConfig *fbcfg = glXChooseFBConfig(display, defscreen, NULL, &nitems);
    if (!fbcfg)
        throw std::string("Couldn't get FB configs\n");

    // https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glXGetFBConfigAttrib.xml
    int maxSamples = 0;
    for (int i = 0; i < nitems; ++i) {
        int samples = 0;
        glXGetFBConfigAttrib(display, fbcfg[i], GLX_SAMPLES, &samples);
        if (samples > maxSamples) {
            maxSamples = samples;
        }
    }

    Logfile::get()->writeInfo("Maximum OpenGL multisamples (GLX): " + toString(maxSamples));

    return min(maxSamples, desiredSamples);
#else
    return desiredSamples;
#endif
}

void SDLWindow::initialize(const WindowSettings &settings)
{
    windowSettings = settings;

    // Set the window attributes
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, windowSettings.depthSize);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    //SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 0);

    // Set core profile
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    if (windowSettings.debugContext) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    }

    if (windowSettings.multisamples != 0) {
        // Context creation fails (at least on GLX) if multisample samples are too high, so query the maximum beforehand
        windowSettings.multisamples = getMaxSamplesGLImpl(windowSettings.multisamples);
    }
    if (windowSettings.multisamples != 0) {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, windowSettings.multisamples);
        glEnable(GL_MULTISAMPLE);
    }

    errorCheck();

    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
    if (windowSettings.fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
    if (windowSettings.resizable) flags |= SDL_WINDOW_RESIZABLE;

    // Create the window
    sdlWindow = SDL_CreateWindow(FileUtils::get()->getTitleName().c_str(),
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            windowSettings.width, windowSettings.height, flags);

    errorCheck();
    glContext = SDL_GL_CreateContext(sdlWindow);
    errorCheck();

    if (windowSettings.multisamples != 0) {
        glEnable(GL_MULTISAMPLE);
    }

    if (windowSettings.vSync) {
        SDL_GL_SetSwapInterval(-1);

        const char *sdlError = SDL_GetError();
        if (boost::contains(sdlError, "Negative swap interval unsupported")) {
            Logfile::get()->writeInfo(std::string() + "VSYNC Info: " + sdlError);
            SDL_ClearError();
            SDL_GL_SetSwapInterval(1);
        }
    } else {
        SDL_GL_SetSwapInterval(0);
    }

    // Did something fail during the initialization?
    errorCheck();

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        Logfile::get()->writeError(std::string() + "Error: SDLWindow::initializeOpenGL: glewInit: " + (char*)glewGetErrorString(glewError));
    }

    glViewport(0, 0, windowSettings.width, windowSettings.height);
}


void SDLWindow::toggleFullscreen(bool nativeFullscreen)
{
    windowSettings.fullscreen = !windowSettings.fullscreen;
    int fullscreenMode = nativeFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN;
    SDL_SetWindowFullscreen(sdlWindow, windowSettings.fullscreen ? fullscreenMode : 0);
}

void SDLWindow::setWindowSize(int width, int height)
{
    SDL_SetWindowSize(sdlWindow, width, height);
    windowSettings.width = width;
    windowSettings.height = height;
    EventManager::get()->queueEvent(EventPtr(new Event(RESOLUTION_CHANGED_EVENT)));
}

void SDLWindow::close()
{
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(sdlWindow);
    AppSettings::get()->release();
    Logfile::get()->write("SDLWindow::quit()");
}

void SDLWindow::update()
{
}

bool SDLWindow::processEvents(std::function<void(const SDL_Event&)> eventHandler)
{
    SDL_PumpEvents();

    bool running = true;
    SDLMouse *sdlMouse = (SDLMouse*)Mouse;
    sdlMouse->setScrollWheelValue(0);
    SDL_Event event;
    while (SDL_PollEvent (&event)) {
        switch (event.type) {
        case (SDL_QUIT):
            running = false;
            break;

        case (SDL_KEYDOWN):
            switch (event.key.keysym.sym) {
            case (SDLK_ESCAPE):
                running = false;
                break;
            case (SDLK_v):
                if (SDL_GetModState() & KMOD_CTRL) {
                    char *clipboardText = SDL_GetClipboardText();
                    Keyboard->addToKeyBuffer(clipboardText);
                    free(clipboardText);
                }
                break;
            default:
                eventHandler(event);
                break;
            }
            break;

        case SDL_TEXTINPUT:
            if ((SDL_GetModState() & KMOD_CTRL) == 0) {
                Keyboard->addToKeyBuffer(event.text.text);
            }
            eventHandler(event);
            break;

        case SDL_WINDOWEVENT:
            switch (event.window.event) {
            case SDL_WINDOWEVENT_RESIZED:
                windowSettings.width = event.window.data1;
                windowSettings.height = event.window.data2;
                EventManager::get()->queueEvent(EventPtr(new Event(RESOLUTION_CHANGED_EVENT)));
                break;
            }
            eventHandler(event);
            break;

        case SDL_MOUSEWHEEL:
            sdlMouse->setScrollWheelValue(event.wheel.y);
            eventHandler(event);
            break;

        default:
            eventHandler(event);
        }
    }
    return running;
}

void SDLWindow::clear(const Color &color)
{
    glClearColor(color.getFloatR(), color.getFloatG(), color.getFloatB(), 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

void SDLWindow::flip()
{
    SDL_GL_SwapWindow(sdlWindow);
}

void SDLWindow::serializeSettings(SettingsFile &settings)
{
    settings.addKeyValue("window-width", windowSettings.width);
    settings.addKeyValue("window-height", windowSettings.height);
    settings.addKeyValue("window-fullscreen", windowSettings.fullscreen);
    settings.addKeyValue("window-resizable", windowSettings.resizable);
    settings.addKeyValue("window-multisamples", windowSettings.multisamples);
    settings.addKeyValue("window-depthSize", windowSettings.depthSize);
    settings.addKeyValue("window-vSync", windowSettings.vSync);

}

WindowSettings SDLWindow::deserializeSettings(const SettingsFile &settings)
{
    WindowSettings windowSettings;
    settings.getValueOpt("window-width", windowSettings.width);
    settings.getValueOpt("window-height", windowSettings.height);
    settings.getValueOpt("window-fullscreen", windowSettings.fullscreen);
    settings.getValueOpt("window-resizable", windowSettings.resizable);
    settings.getValueOpt("window-multisamples", windowSettings.multisamples);
    settings.getValueOpt("window-depthSize", windowSettings.depthSize);
    settings.getValueOpt("window-vSync", windowSettings.vSync);
    settings.getValueOpt("window-debugContext", windowSettings.debugContext);
    return windowSettings;
}

void SDLWindow::saveScreenshot(const char *filename)
{
    BitmapPtr bitmap(new Bitmap(windowSettings.width, windowSettings.height, 32));
    glReadPixels(0, 0, windowSettings.width, windowSettings.height, GL_RGBA, GL_UNSIGNED_BYTE, bitmap->getPixels());
    bitmap->savePNG(filename, true);
    Logfile::get()->write(std::string() + "INFO: SDLWindow::saveScreenshot: Screenshot saved to \"" + filename + "\".", BLUE);
}

}
