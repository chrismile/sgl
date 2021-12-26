/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2017, Christoph Neuhauser
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

#include <iostream>
#include <cstdlib>
#include <boost/algorithm/string/predicate.hpp>
#include <SDL2/SDL.h>

#include <Math/Math.hpp>
#include <Utils/AppSettings.hpp>
#include <Utils/File/FileUtils.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/Events/EventManager.hpp>
#include <Graphics/Texture/Bitmap.hpp>

#include "Input/SDLMouse.hpp"
#include "Input/SDLKeyboard.hpp"
#include "Input/SDLGamepad.hpp"
#include "SDLWindow.hpp"

#ifdef SUPPORT_OPENGL
#include <GL/glew.h>
#ifdef __linux__
#include <dlfcn.h>
#include <X11/Xlib.h>
#include <GL/glx.h>
#endif
#endif

#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/libs/volk/volk.h>
#include <Graphics/Vulkan/Utils/Instance.hpp>
#include <Graphics/Vulkan/Utils/Device.hpp>
#include <Graphics/Vulkan/Utils/Swapchain.hpp>
#endif

namespace sgl {

SDLWindow::SDLWindow() = default;

SDLWindow::~SDLWindow()
{
#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        SDL_GL_DeleteContext(glContext);
    }
#endif
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN) {
        sgl::vk::Instance* instance = sgl::AppSettings::get()->getVulkanInstance();
        vkDestroySurfaceKHR(instance->getVkInstance(), windowSurface, nullptr);
    }
#endif
    SDL_DestroyWindow(sdlWindow);
    Logfile::get()->write("Closing SDL window.");
}

void SDLWindow::errorCheck()
{
    errorCheckSDL();
}

void SDLWindow::errorCheckSDL()
{
    while (SDL_GetError()[0] != '\0') {
        Logfile::get()->writeError(std::string() + "SDL error: " + SDL_GetError());
        SDL_ClearError();
    }
}

#ifdef SUPPORT_OPENGL
// Query the numbers of multisample samples possible (given a maximum number of desired samples)
int getMaxSamplesGLImpl(int desiredSamples) {
#ifdef __linux__
    typedef Display* (*PFN_XOpenDisplay)(_Xconst char*);
    typedef GLXFBConfig* (*PFN_glXChooseFBConfig)(Display* dpy, int screen, const int* attribList, int* nitems);
    typedef int (*PFN_glXGetFBConfigAttrib)(Display* dpy, GLXFBConfig config, int attribute, int* value);

    void* libX11so = dlopen("libX11.so", RTLD_NOW | RTLD_LOCAL);
    if (!libX11so) {
        sgl::Logfile::get()->writeError("Error in getMaxSamplesGLImpl: Could not load libX11.so!");
        return 1;
    }
    void* libGLXso = dlopen("libGLX.so", RTLD_NOW | RTLD_LOCAL);
    if (!libGLXso) {
        sgl::Logfile::get()->writeError("Error in getMaxSamplesGLImpl: Could not load libGLX.so!");
        dlclose(libX11so);
        return 1;
    }

    auto dyn_XOpenDisplay = PFN_XOpenDisplay(dlsym(libX11so, "XOpenDisplay"));
    if (!dyn_XOpenDisplay) {
        sgl::Logfile::get()->writeError("Error in getMaxSamplesGLImpl: Could not load function from libX11.so!");
        dlclose(libGLXso);
        dlclose(libX11so);
        return 1;
    }

    auto dyn_glXChooseFBConfig = PFN_glXChooseFBConfig(dlsym(libGLXso, "glXChooseFBConfig"));
    auto dyn_glXGetFBConfigAttrib = PFN_glXGetFBConfigAttrib(dlsym(libGLXso, "glXGetFBConfigAttrib"));
    if (!dyn_glXChooseFBConfig || !dyn_glXGetFBConfigAttrib) {
        sgl::Logfile::get()->writeError("Error in getMaxSamplesGLImpl: Could not load functions from libGLX.so!");
        dlclose(libGLXso);
        dlclose(libX11so);
        return 1;
    }


    const char* displayName = ":0";
    Display* display;
    if (!(display = dyn_XOpenDisplay(displayName))) {
        Logfile::get()->writeError("Error in getMaxSamplesGLImpl: Couldn't open X11 display!");
        dlclose(libGLXso);
        dlclose(libX11so);
        return 1;
    }
    int defscreen = DefaultScreen(display);

    int nitems;
    GLXFBConfig* fbcfg = dyn_glXChooseFBConfig(display, defscreen, NULL, &nitems);
    if (!fbcfg) {
        Logfile::get()->writeError("Error in getMaxSamplesGLImpl: Couldn't get FB configs!");
        dlclose(libGLXso);
        dlclose(libX11so);
        return 1;
    }

    // https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glXGetFBConfigAttrib.xml
    int maxSamples = 0;
    for (int i = 0; i < nitems; ++i) {
        int samples = 0;
        dyn_glXGetFBConfigAttrib(display, fbcfg[i], GLX_SAMPLES, &samples);
        if (samples > maxSamples) {
            maxSamples = samples;
        }
    }

    Logfile::get()->writeInfo("Maximum OpenGL multisamples (GLX): " + toString(maxSamples));

    dlclose(libGLXso);
    dlclose(libX11so);

    return min(maxSamples, desiredSamples);
#else
    return desiredSamples;
#endif
}
#endif

void SDLWindow::initialize(const WindowSettings &settings, RenderSystem renderSystem)
{
    this->renderSystem = renderSystem;
    this->windowSettings = settings;

#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        // Set the window attributes
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, windowSettings.depthSize);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, windowSettings.stencilSize);
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
    }
#endif

    errorCheck();

    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;
#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        flags = flags | SDL_WINDOW_OPENGL;
    }
#endif
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN) {
        flags = flags | SDL_WINDOW_VULKAN;
    }
#endif
    if (windowSettings.fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
    if (windowSettings.resizable) flags |= SDL_WINDOW_RESIZABLE;

    // Create the window
    sdlWindow = SDL_CreateWindow(FileUtils::get()->getTitleName().c_str(),
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            windowSettings.width, windowSettings.height, flags);

    errorCheck();
#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        glContext = SDL_GL_CreateContext(sdlWindow);
        errorCheck();
        SDL_GL_MakeCurrent(sdlWindow, glContext);
    }
#endif
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN) {
        /*
         * The array "instanceExtensionNames" holds the name of all extensions that get requested. First, user-specified
         * extensions are added. Then, extensions required by SDL are added using "SDL_Vulkan_GetInstanceExtensions".
         */
        std::vector<const char*> instanceExtensionNames = {
        };
        uint32_t extensionCount;
        SDL_Vulkan_GetInstanceExtensions(sdlWindow, &extensionCount, nullptr);
        size_t additionalExtensionCount = instanceExtensionNames.size();
        instanceExtensionNames.resize(extensionCount + instanceExtensionNames.size());
        SDL_Vulkan_GetInstanceExtensions(
                sdlWindow, &extensionCount, instanceExtensionNames.data() + additionalExtensionCount);

        sgl::vk::Instance* instance = sgl::AppSettings::get()->getVulkanInstance();
        instance->createInstance(instanceExtensionNames, windowSettings.debugContext);

        if (!SDL_Vulkan_CreateSurface(sdlWindow, instance->getVkInstance(), &windowSurface)) {
            sgl::Logfile::get()->writeError(
                    std::string() + "Error in SDLWindow::initialize: Failed to create a Vulkan surface.");
            exit(-1);
        }
    }
#endif
    errorCheck();

#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL && windowSettings.multisamples != 0) {
        glEnable(GL_MULTISAMPLE);
    }

    if (renderSystem == RenderSystem::OPENGL && windowSettings.vSync) {
        if (windowSettings.vSync) {
            SDL_GL_SetSwapInterval(-1);

            const char* sdlError = SDL_GetError();
            if (boost::contains(sdlError, "Negative swap interval unsupported")) {
                Logfile::get()->writeInfo(std::string() + "VSYNC Info: " + sdlError);
                SDL_ClearError();
                SDL_GL_SetSwapInterval(1);
            }
        } else {
            SDL_GL_SetSwapInterval(0);
        }
    }
#endif

    // Did something fail during the initialization?
    errorCheck();

#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        // Initialize GLEW
        glewExperimental = GL_TRUE;
        GLenum glewError = glewInit();
        if (glewError != GLEW_OK) {
            Logfile::get()->writeError(std::string() + "Error: SDLWindow::initializeOpenGL: glewInit: " + (char*)glewGetErrorString(glewError));
        }

        glViewport(0, 0, windowSettings.width, windowSettings.height);
    }
#endif
}


void SDLWindow::toggleFullscreen(bool nativeFullscreen)
{
    windowSettings.fullscreen = !windowSettings.fullscreen;
    int fullscreenMode = nativeFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN;
    SDL_SetWindowFullscreen(sdlWindow, windowSettings.fullscreen ? fullscreenMode : 0);
}

void SDLWindow::setWindowSize(int width, int height)
{
    windowSettings.width = width;
    windowSettings.height = height;
    SDL_SetWindowSize(sdlWindow, width, height);
    if (renderSystem != RenderSystem::VULKAN) {
        EventManager::get()->queueEvent(EventPtr(new Event(RESOLUTION_CHANGED_EVENT)));
    }
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN) {
        //sgl::vk::Device* device = sgl::AppSettings::get()->getPrimaryDevice();
        //device->waitIdle();
    }
#endif
}

glm::ivec2 SDLWindow::getWindowPosition()
{
    int x, y;
    SDL_GetWindowPosition(sdlWindow, &x, &y);
    return glm::ivec2(x, y);
}

void SDLWindow::setWindowPosition(int x, int y)
{
    SDL_SetWindowPosition(sdlWindow, x, y);
}

void SDLWindow::update()
{
}

void SDLWindow::setEventHandler(std::function<void(const SDL_Event&)> eventHandler)
{
    this->eventHandler = eventHandler;
    eventHandlerSet = true;
}

bool SDLWindow::processEvents()
{
    SDL_PumpEvents();

    bool running = true;
    SDLMouse* sdlMouse = (SDLMouse*)Mouse;
    sdlMouse->setScrollWheelValue(0);
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case (SDL_QUIT):
            running = false;
            break;

        case (SDL_KEYDOWN):
            switch (event.key.keysym.sym) {
            case (SDLK_v):
                if (SDL_GetModState() & KMOD_CTRL) {
                    char* clipboardText = SDL_GetClipboardText();
                    Keyboard->addToKeyBuffer(clipboardText);
                    free(clipboardText);
                }
                break;
            }
            break;

        case SDL_TEXTINPUT:
            if ((SDL_GetModState() & KMOD_CTRL) == 0) {
                Keyboard->addToKeyBuffer(event.text.text);
            }
            break;

        case SDL_WINDOWEVENT:
            if (event.window.windowID == SDL_GetWindowID(sdlWindow)) {
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_RESIZED:
                        windowSettings.width = event.window.data1;
                        windowSettings.height = event.window.data2;
                        if (renderSystem != RenderSystem::VULKAN) {
                            EventManager::get()->queueEvent(EventPtr(new Event(RESOLUTION_CHANGED_EVENT)));
                        }
#ifdef SUPPORT_VULKAN
                        else {
                            vk::Swapchain* swapchain = AppSettings::get()->getSwapchain();
                            if (swapchain && !swapchain->getIsWaitingForResizeEnd()) {
                                swapchain->recreateSwapchain();
                            }
                        }
#endif
                        break;
                    case SDL_WINDOWEVENT_CLOSE:
                        if (event.window.windowID == SDL_GetWindowID(sdlWindow)) {
                            running = false;
                        }
                        break;
                }
            }
            break;

        case SDL_MOUSEWHEEL:
            sdlMouse->setScrollWheelValue(event.wheel.y);
            break;
        }
        if (eventHandlerSet) {
            eventHandler(event);
        }
    }

    if (isFirstFrame) {
        if (windowSettings.savePosition && windowSettings.windowPosition.x != std::numeric_limits<int>::min()) {
            setWindowPosition(windowSettings.windowPosition.x, windowSettings.windowPosition.y);
        }
        isFirstFrame = false;
    }

    return running;
}

void SDLWindow::clear(const Color &color)
{
#ifdef SUPPORT_OPENGL
    glClearColor(color.getFloatR(), color.getFloatG(), color.getFloatB(), 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}

void SDLWindow::flip()
{
    if (renderSystem == RenderSystem::OPENGL) {
        SDL_GL_SwapWindow(sdlWindow);
    } else {
        throw std::runtime_error("SDLWindow::flip: Unsupported operation when using Vulkan.");
    }
}

void SDLWindow::serializeSettings(SettingsFile &settings)
{
    settings.addKeyValue("window-width", windowSettings.width);
    settings.addKeyValue("window-height", windowSettings.height);
    settings.addKeyValue("window-fullscreen", windowSettings.fullscreen);
    settings.addKeyValue("window-resizable", windowSettings.resizable);
    settings.addKeyValue("window-multisamples", windowSettings.multisamples);
    settings.addKeyValue("window-depthSize", windowSettings.depthSize);
    settings.addKeyValue("window-stencilSize", windowSettings.stencilSize);
    settings.addKeyValue("window-vSync", windowSettings.vSync);
    settings.addKeyValue("window-savePosition", windowSettings.savePosition);
    if (windowSettings.savePosition) {
        windowSettings.windowPosition = getWindowPosition();
        settings.addKeyValue("window-windowPosition", windowSettings.windowPosition);
    }
}

WindowSettings SDLWindow::deserializeSettings(const SettingsFile &settings)
{
    WindowSettings windowSettings;
    if (!settings.hasKey("window-width") || !settings.hasKey("window-height")) {
        int desktopWidth = 1920;
        int desktopHeight = 1080;
        int refreshRate = 60;
        sgl::AppSettings::get()->getDesktopDisplayMode(desktopWidth, desktopHeight, refreshRate);
        if (desktopWidth < 2560 || desktopHeight < 1440) {
            windowSettings.width = 1280;
            windowSettings.height = 720;
        } else {
            windowSettings.width = 1920;
            windowSettings.height = 1080;
        }
    }
    settings.getValueOpt("window-width", windowSettings.width);
    settings.getValueOpt("window-height", windowSettings.height);
    settings.getValueOpt("window-fullscreen", windowSettings.fullscreen);
    settings.getValueOpt("window-resizable", windowSettings.resizable);
    settings.getValueOpt("window-multisamples", windowSettings.multisamples);
    settings.getValueOpt("window-depthSize", windowSettings.depthSize);
    settings.getValueOpt("window-stencilSize", windowSettings.stencilSize);
    settings.getValueOpt("window-vSync", windowSettings.vSync);
    settings.getValueOpt("window-debugContext", windowSettings.debugContext);
    settings.getValueOpt("window-savePosition", windowSettings.savePosition);
    settings.getValueOpt("window-windowPosition", windowSettings.windowPosition);
    return windowSettings;
}

void SDLWindow::saveScreenshot(const char* filename)
{
    if (renderSystem == RenderSystem::OPENGL) {
#ifdef SUPPORT_OPENGL
        BitmapPtr bitmap(new Bitmap(windowSettings.width, windowSettings.height, 32));
        glReadPixels(
                0, 0, windowSettings.width, windowSettings.height,
                GL_RGBA, GL_UNSIGNED_BYTE, bitmap->getPixels());
        bitmap->savePNG(filename, true);
        Logfile::get()->write(
                std::string() + "INFO: SDLWindow::saveScreenshot: Screenshot saved to \""
                + filename + "\".", BLUE);
#endif
    } else {
        throw std::runtime_error("SDLWindow::saveScreenshot: Unsupported operation when using Vulkan.");
    }
}

}
