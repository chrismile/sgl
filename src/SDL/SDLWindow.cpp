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
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <Math/Math.hpp>
#include <Utils/StringUtils.hpp>
#include <Utils/AppSettings.hpp>
#include <Utils/File/FileUtils.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/Events/EventManager.hpp>
#include <Utils/File/ResourceBuffer.hpp>
#include <Utils/File/ResourceManager.hpp>
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

#ifdef SUPPORT_WEBGPU
#include <sdl2webgpu.h>

// Xlib.h defines "Bool" on Linux, which conflicts with webgpu.hpp.
#ifdef Bool
#undef Bool
#endif

// X.h defines "Success", "Always", and "None" on Linux, which conflicts with webgpu.hpp.
#ifdef Success
#undef Success
#endif
#ifdef Always
#undef Always
#endif
#ifdef None
#undef None
#endif

#include <Graphics/WebGPU/Utils/Instance.hpp>
#include <Graphics/WebGPU/Utils/Swapchain.hpp>
#endif

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
//#include <UniformTypeIdentifiers/UTCoreTypes.h>
#endif

namespace sgl {

SDLWindow::SDLWindow() = default;

SDLWindow::~SDLWindow() {
    for (auto& entry : cursors) {
        SDL_FreeCursor(entry.second);
    }
    cursors.clear();
#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        SDL_GL_DeleteContext(glContext);
    }
#endif
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN && !windowSettings.useDownloadSwapchain) {
        sgl::vk::Instance* instance = sgl::AppSettings::get()->getVulkanInstance();
        vkDestroySurfaceKHR(instance->getVkInstance(), windowSurface, nullptr);
    }
#endif
#ifdef SUPPORT_WEBGPU
    if (renderSystem == RenderSystem::WEBGPU && webgpuSurface) {
        wgpuSurfaceRelease(webgpuSurface);
        webgpuSurface = nullptr;
    }
#endif
    SDL_DestroyWindow(sdlWindow);
    Logfile::get()->write("Closing SDL window.");
}

void SDLWindow::errorCheck() {
    errorCheckSDL();
}

void SDLWindow::errorCheckSDL() {
    while (SDL_GetError()[0] != '\0') {
        std::string errorString = SDL_GetError();
        bool openMessageBox = true;
        // "Unknown sensor type" can somehow can occur some Windows systems. Ignore it, as it is probably harmless.
        if (sgl::stringContains(errorString, "Unknown sensor type")
                || sgl::stringContains(errorString, "No window has focus")
                // "Couldn't get DPI" happens on an Ubuntu 22.04 VM. We have good fallbacks, so don't open a message box.
                || sgl::stringContains(errorString, "Couldn't get DPI")
                || sgl::stringContains(errorString, "X server refused mouse capture")
                || sgl::stringContains(errorString, "Unknown touch device id -1, cannot reset")) {
            openMessageBox = false;
        }
        Logfile::get()->writeError(std::string() + "SDL error: " + errorString, openMessageBox);
        SDL_ClearError();
    }
}

void SDLWindow::errorCheckSDLCritical() {
    while (SDL_GetError()[0] != '\0') {
        Logfile::get()->throwError(std::string() + "SDL error: " + SDL_GetError());
    }
}

void SDLWindow::errorCheckIgnoreUnsupportedOperation() {
    const char* sdlError = "";
    while (true) {
        sdlError = SDL_GetError();
        if (sdlError[0] == '\0') {
            break;
        }
        if (sgl::stringContains(sdlError, "That operation is not supported")) {
            SDL_ClearError();
        } else {
            Logfile::get()->throwError(std::string() + "SDL error HERE: " + sdlError);
        }
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
        libGLXso = dlopen("libGLX.so.0", RTLD_NOW | RTLD_LOCAL);
        if (!libGLXso) {
            sgl::Logfile::get()->writeError("Error in getMaxSamplesGLImpl: Could not load libGLX.so!");
            dlclose(libX11so);
            return 1;
        }
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

void SDLWindow::initialize(const WindowSettings &settings, RenderSystem renderSystem) {
    this->renderSystem = renderSystem;
    this->windowSettings = settings;

    errorCheck();

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
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);

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
        }
    }
#endif

    errorCheckSDLCritical();

    Uint32 flags = SDL_WINDOW_SHOWN;
#ifndef __APPLE__
    flags |= SDL_WINDOW_ALLOW_HIGHDPI;
#else
    /*
     * Check if the application is run from an app bundle (only then NSHighResolutionCapable is set):
     *  https://stackoverflow.com/questions/58036928/check-if-c-program-is-running-as-an-app-bundle-or-command-line-on-mac
     */
    CFBundleRef bundle = CFBundleGetMainBundle();
    CFURLRef bundleUrl = CFBundleCopyBundleURL(bundle);
    CFStringRef uti;
    if (CFURLCopyResourcePropertyForKey(bundleUrl, kCFURLTypeIdentifierKey, &uti, NULL) && uti
            && UTTypeConformsTo(uti, kUTTypeApplicationBundle)) {
        flags |= SDL_WINDOW_ALLOW_HIGHDPI;
    }
#endif
#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        flags |= SDL_WINDOW_OPENGL;
    }
#endif
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN && !windowSettings.useDownloadSwapchain) {
        flags |= SDL_WINDOW_VULKAN;
    }
#endif
    if (windowSettings.fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
    if (windowSettings.resizable) flags |= SDL_WINDOW_RESIZABLE;

    // Create the window
    sdlWindow = SDL_CreateWindow(FileUtils::get()->getAppName().c_str(),
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            windowSettings.width, windowSettings.height, flags);

    errorCheckSDLCritical();
#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        glContext = SDL_GL_CreateContext(sdlWindow);
        errorCheckSDLCritical();
        SDL_GL_MakeCurrent(sdlWindow, glContext);
    }
#endif
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN && !windowSettings.useDownloadSwapchain) {
        /*
         * The array "instanceExtensionNames" holds the name of all extensions that get requested. First, user-specified
         * extensions are added. Then, extensions required by SDL are added using "SDL_Vulkan_GetInstanceExtensions".
         */
        std::vector<const char*> instanceExtensionNames =
                sgl::AppSettings::get()->getRequiredVulkanInstanceExtensions();
        uint32_t extensionCount;
        SDL_Vulkan_GetInstanceExtensions(sdlWindow, &extensionCount, nullptr);
        size_t additionalExtensionCount = instanceExtensionNames.size();
        instanceExtensionNames.resize(extensionCount + instanceExtensionNames.size());
        SDL_Vulkan_GetInstanceExtensions(
                sdlWindow, &extensionCount, instanceExtensionNames.data() + additionalExtensionCount);

        sgl::vk::Instance* instance = sgl::AppSettings::get()->getVulkanInstance();
        instance->createInstance(instanceExtensionNames, windowSettings.debugContext);

        if (!SDL_Vulkan_CreateSurface(sdlWindow, instance->getVkInstance(), &windowSurface)) {
            sgl::Logfile::get()->throwError(
                    std::string() + "Error in SDLWindow::initialize: Failed to create a Vulkan surface.");
        }
    }
    if (renderSystem == RenderSystem::VULKAN && windowSettings.useDownloadSwapchain) {
        sgl::Logfile::get()->write("Using Vulkan download swapchain (i.e., manual copy to window).", sgl::BLUE);
        sgl::vk::Instance* instance = sgl::AppSettings::get()->getVulkanInstance();
        instance->createInstance({}, windowSettings.debugContext);
    }
#endif
#ifdef SUPPORT_WEBGPU
    if (renderSystem == RenderSystem::WEBGPU) {
        sgl::webgpu::Instance* instance = sgl::AppSettings::get()->getWebGPUInstance();
        instance->createInstance();
        if (!instance) {
            sgl::Logfile::get()->throwError(
                    std::string() + "Error in SDLWindow::initialize: Failed to create a WebGPU instance.");
        }
        errorCheckSDLCritical();
        webgpuSurface = SDL_GetWGPUSurface(instance->getWGPUInstance(), sdlWindow);
        if (!webgpuSurface) {
            sgl::Logfile::get()->throwError(
                    std::string() + "Error in SDLWindow::initialize: Failed to create a WebGPU surface.");
        }
#ifdef __EMSCRIPTEN__
        // For whatever reason, we get "SDL error: That operation is not supported" after SDL_GetWindowWMInfo.
        errorCheckIgnoreUnsupportedOperation();
#endif
    }
#endif
    errorCheckSDLCritical();

#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL && windowSettings.multisamples != 0) {
        glEnable(GL_MULTISAMPLE);
    }

    if (renderSystem == RenderSystem::OPENGL) {
        if (windowSettings.vSync) {
            SDL_GL_SetSwapInterval(-1);

            const char* sdlError = SDL_GetError();
            if (sgl::stringContains(sdlError, "Negative swap interval unsupported")) {
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


    SDL_SysWMinfo wminfo;
    SDL_VERSION(&wminfo.version);
    if (SDL_GetWindowWMInfo(sdlWindow, &wminfo)) {
        usesX11Backend = (wminfo.subsystem == SDL_SYSWM_X11);
        usesWaylandBackend = (wminfo.subsystem == SDL_SYSWM_WAYLAND);
#ifdef __linux__
        const char* waylandDisplayVar = getenv("WAYLAND_DISPLAY");
        if (usesX11Backend && waylandDisplayVar) {
            usesXWaylandBackend = true;
        }
#endif
    }
#ifdef __EMSCRIPTEN__
    // For whatever reason, we get "SDL error: That operation is not supported" after SDL_GetWindowWMInfo.
    errorCheckIgnoreUnsupportedOperation();
#endif

    windowSettings.pixelWidth = windowSettings.width;
    windowSettings.pixelHeight = windowSettings.height;
#if defined(__APPLE__) || defined(__linux__)
#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        SDL_GL_GetDrawableSize(sdlWindow, &windowSettings.pixelWidth, &windowSettings.pixelHeight);
    }
#endif
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN && !windowSettings.useDownloadSwapchain) {
        SDL_Vulkan_GetDrawableSize(sdlWindow, &windowSettings.pixelWidth, &windowSettings.pixelHeight);
    }
#endif
#endif

#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        // Initialize GLEW
        glewExperimental = GL_TRUE;
        GLenum glewError = glewInit();
#ifndef __EMSCRIPTEN__
        // NO_GLX_DISPLAY can trigger when using Wayland, but GLEW still seems to work even if not built with EGL support...
        if (glewError == GLEW_ERROR_NO_GLX_DISPLAY) {
            Logfile::get()->writeWarning("Warning: GLEW is not built with EGL support.");
        } else
#endif
        if (glewError != GLEW_OK) {
            Logfile::get()->writeError(std::string() + "Error: SDLWindow::initializeOpenGL: glewInit: " + (char*)glewGetErrorString(glewError));
        }

        glViewport(0, 0, windowSettings.pixelWidth, windowSettings.pixelHeight);
    }
#endif
}


void SDLWindow::toggleFullscreen(bool nativeFullscreen) {
    windowSettings.fullscreen = !windowSettings.fullscreen;
    int fullscreenMode = nativeFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN;
    SDL_SetWindowFullscreen(sdlWindow, windowSettings.fullscreen ? fullscreenMode : 0);
}

void SDLWindow::setWindowVirtualSize(int width, int height) {
    windowSettings.width = width;
    windowSettings.height = height;
    windowSettings.pixelWidth = width;
    windowSettings.pixelHeight = height;
#if defined(__APPLE__) || defined(__linux__)
#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        int oldWidth = 0, oldHeight = 0;
        int oldPixelWidth = 0, oldPixelHeight = 0;
        SDL_GetWindowSize(sdlWindow, &oldWidth, &oldHeight);
        SDL_GL_GetDrawableSize(sdlWindow, &oldPixelWidth, &oldPixelHeight);
        windowSettings.pixelWidth = width * oldPixelWidth / oldWidth;
        windowSettings.pixelWidth = height * oldPixelHeight / oldHeight;
    }
#endif
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN && !windowSettings.useDownloadSwapchain) {
        int oldWidth = 0, oldHeight = 0;
        int oldPixelWidth = 0, oldPixelHeight = 0;
        SDL_GetWindowSize(sdlWindow, &oldWidth, &oldHeight);
        SDL_Vulkan_GetDrawableSize(sdlWindow, &oldPixelWidth, &oldPixelHeight);
        windowSettings.pixelWidth = width * oldPixelWidth / oldWidth;
        windowSettings.pixelWidth = height * oldPixelHeight / oldHeight;
    }
#endif
#endif
    SDL_SetWindowSize(sdlWindow, windowSettings.width, windowSettings.height);
    if (renderSystem != RenderSystem::VULKAN) {
        EventManager::get()->queueEvent(EventPtr(new Event(RESOLUTION_CHANGED_EVENT)));
    }
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN && !windowSettings.useDownloadSwapchain) {
        //sgl::vk::Device* device = sgl::AppSettings::get()->getPrimaryDevice();
        //device->waitIdle();
    }
#endif
}

void SDLWindow::setWindowPixelSize(int width, int height) {
    windowSettings.width = width;
    windowSettings.height = height;
    windowSettings.pixelWidth = width;
    windowSettings.pixelHeight = height;
#if defined(__APPLE__) || defined(__linux__)
#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        int oldWidth = 0, oldHeight = 0;
        int oldPixelWidth = 0, oldPixelHeight = 0;
        SDL_GetWindowSize(sdlWindow, &oldWidth, &oldHeight);
        SDL_GL_GetDrawableSize(sdlWindow, &oldPixelWidth, &oldPixelHeight);
        windowSettings.width = width * oldWidth / oldPixelWidth;
        windowSettings.height = height * oldHeight / oldPixelHeight;
    }
#endif
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN && !windowSettings.useDownloadSwapchain) {
        int oldWidth = 0, oldHeight = 0;
        int oldPixelWidth = 0, oldPixelHeight = 0;
        SDL_GetWindowSize(sdlWindow, &oldWidth, &oldHeight);
        SDL_Vulkan_GetDrawableSize(sdlWindow, &oldPixelWidth, &oldPixelHeight);
        windowSettings.width = width * oldWidth / oldPixelWidth;
        windowSettings.height = height * oldHeight / oldPixelHeight;
    }
#endif
#endif
    SDL_SetWindowSize(sdlWindow, windowSettings.width, windowSettings.height);
    if (renderSystem != RenderSystem::VULKAN) {
        EventManager::get()->queueEvent(EventPtr(new Event(RESOLUTION_CHANGED_EVENT)));
    }
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN && !windowSettings.useDownloadSwapchain) {
        //sgl::vk::Device* device = sgl::AppSettings::get()->getPrimaryDevice();
        //device->waitIdle();
    }
#endif
}

glm::ivec2 SDLWindow::getWindowPosition() {
    int x, y;
    SDL_GetWindowPosition(sdlWindow, &x, &y);
    return glm::ivec2(x, y);
}

void SDLWindow::setWindowPosition(int x, int y) {
    SDL_SetWindowPosition(sdlWindow, x, y);
}

void SDLWindow::update() {
}

void SDLWindow::setEventHandler(std::function<void(const SDL_Event&)> eventHandler) {
    this->eventHandler = eventHandler;
    eventHandlerSet = true;
}

bool SDLWindow::processEvents() {
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
                        windowSettings.pixelWidth = windowSettings.width;
                        windowSettings.pixelHeight = windowSettings.height;
#if defined(__APPLE__) || defined(__linux__)
#ifdef SUPPORT_OPENGL
                        if (renderSystem == RenderSystem::OPENGL) {
                            SDL_GL_GetDrawableSize(sdlWindow, &windowSettings.pixelWidth, &windowSettings.pixelHeight);
                        }
#endif
#ifdef SUPPORT_VULKAN
                        if (renderSystem == RenderSystem::VULKAN && !windowSettings.useDownloadSwapchain) {
                            SDL_Vulkan_GetDrawableSize(sdlWindow, &windowSettings.pixelWidth, &windowSettings.pixelHeight);
                        }
#endif
#endif
#ifdef SUPPORT_WEBGPU
                        if (renderSystem == RenderSystem::WEBGPU) {
                            webgpu::Swapchain* swapchain = AppSettings::get()->getWebGPUSwapchain();
                            if (swapchain) {
                                swapchain->recreateSwapchain();
                            }
                        }
#endif
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

void SDLWindow::clear(const Color &color) {
#ifdef SUPPORT_OPENGL
    glClearColor(color.getFloatR(), color.getFloatG(), color.getFloatB(), 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}

void SDLWindow::flip() {
    if (renderSystem == RenderSystem::OPENGL) {
        SDL_GL_SwapWindow(sdlWindow);
    } else {
        throw std::runtime_error("SDLWindow::flip: Unsupported operation when using Vulkan.");
    }
}

void SDLWindow::serializeSettings(SettingsFile &settings) {
    settings.addKeyValue("window-width", windowSettings.width);
    settings.addKeyValue("window-height", windowSettings.height);
    settings.addKeyValue("window-fullscreen", windowSettings.fullscreen);
    settings.addKeyValue("window-resizable", windowSettings.resizable);
    settings.addKeyValue("window-multisamples", windowSettings.multisamples);
    settings.addKeyValue("window-depthSize", windowSettings.depthSize);
    settings.addKeyValue("window-stencilSize", windowSettings.stencilSize);
    settings.addKeyValue("window-vSync", windowSettings.vSync);
#ifndef __EMSCRIPTEN__
    settings.addKeyValue("window-savePosition", windowSettings.savePosition);
    if (windowSettings.savePosition) {
        windowSettings.windowPosition = getWindowPosition();
        settings.addKeyValue("window-windowPosition", windowSettings.windowPosition);
    }
#endif
    settings.addKeyValue("window-useDownloadSwapchain", windowSettings.useDownloadSwapchain);
}

WindowSettings SDLWindow::deserializeSettings(const SettingsFile &settings) {
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
#ifndef __EMSCRIPTEN__
    settings.getValueOpt("window-savePosition", windowSettings.savePosition);
    settings.getValueOpt("window-windowPosition", windowSettings.windowPosition);
#endif
    settings.getValueOpt("window-useDownloadSwapchain", windowSettings.useDownloadSwapchain);
    return windowSettings;
}

void SDLWindow::saveScreenshot(const char* filename) {
    if (renderSystem == RenderSystem::OPENGL) {
#ifdef SUPPORT_OPENGL
        BitmapPtr bitmap(new Bitmap(windowSettings.pixelWidth, windowSettings.pixelHeight, 32));
        glReadPixels(
                0, 0, windowSettings.pixelWidth, windowSettings.pixelHeight,
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

void SDLWindow::setWindowIconFromFile(const std::string& imageFilename) {
    /*ResourceBufferPtr resource = ResourceManager::get()->getFileSync(imageFilename.c_str());

    SDL_RWops* rwops = SDL_RWFromMem(resource->getBuffer(), int(resource->getBufferSize()));
    if (!rwops) {
        Logfile::get()->writeError(
                std::string() + "Error in SDLWindow::setWindowIconFromFile: SDL_RWFromMem failed for file \""
                + imageFilename + "\". SDL Error: " + "\"" + SDL_GetError() + "\"");
        return;
    }

    SDL_Surface* surface = IMG_Load_RW(rwops, 0);
    rwops->close(rwops);

    // Was loading the file with SDL_image successful?
    if (!surface) {
        Logfile::get()->writeError(
                std::string() + "Error in SDLWindow::setWindowIconFromFile: IMG_Load_RW failed for file: \""
                + imageFilename + "\". SDL Error: " + "\"" + SDL_GetError() + "\"");
        return;
    }*/

    sgl::BitmapPtr bitmap(new sgl::Bitmap);
    bitmap->fromFile(imageFilename.c_str());
    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
            (void*)bitmap->getPixelsConst(), bitmap->getW(), bitmap->getH(), bitmap->getBPP(),
            bitmap->getW() * (bitmap->getBPP() / 8),
            0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);

    SDL_SetWindowIcon(sdlWindow, surface);
    SDL_FreeSurface(surface);
}

void SDLWindow::setCursorType(CursorType cursorType) {
    if (currentCursorType == cursorType) {
        return;
    }
    currentCursorType = cursorType;
    if (cursorType == CursorType::DEFAULT) {
        SDL_SetCursor(SDL_GetDefaultCursor());
        return;
    }

    auto it = cursors.find(cursorType);
    if (it != cursors.end()) {
        SDL_SetCursor(it->second);
    } else {
        SDL_SystemCursor sdlCursorType = SDL_SYSTEM_CURSOR_ARROW;
        if (cursorType == CursorType::ARROW) {
            sdlCursorType = SDL_SYSTEM_CURSOR_ARROW;
        } else if (cursorType == CursorType::IBEAM) {
            sdlCursorType = SDL_SYSTEM_CURSOR_IBEAM;
        } else if (cursorType == CursorType::WAIT) {
            sdlCursorType = SDL_SYSTEM_CURSOR_WAIT;
        } else if (cursorType == CursorType::CROSSHAIR) {
            sdlCursorType = SDL_SYSTEM_CURSOR_CROSSHAIR;
        } else if (cursorType == CursorType::WAITARROW) {
            sdlCursorType = SDL_SYSTEM_CURSOR_WAITARROW;
        } else if (cursorType == CursorType::SIZENWSE) {
            sdlCursorType = SDL_SYSTEM_CURSOR_SIZENWSE;
        } else if (cursorType == CursorType::SIZENESW) {
            sdlCursorType = SDL_SYSTEM_CURSOR_SIZENESW;
        } else if (cursorType == CursorType::SIZEWE) {
            sdlCursorType = SDL_SYSTEM_CURSOR_SIZEWE;
        } else if (cursorType == CursorType::SIZENS) {
            sdlCursorType = SDL_SYSTEM_CURSOR_SIZENS;
        } else if (cursorType == CursorType::SIZEALL) {
            sdlCursorType = SDL_SYSTEM_CURSOR_SIZEALL;
        } else if (cursorType == CursorType::NO) {
            sdlCursorType = SDL_SYSTEM_CURSOR_NO;
        } else if (cursorType == CursorType::HAND) {
            sdlCursorType = SDL_SYSTEM_CURSOR_HAND;
        }
        SDL_Cursor* cursor = SDL_CreateSystemCursor(sdlCursorType);
        SDL_SetCursor(cursor);
        cursors[cursorType] = cursor;
    }
}


void SDLWindow::setShowCursor(bool _show) {
    if (showCursor == _show) {
        return;
    }
    showCursor = _show;
    SDL_ShowCursor(showCursor ? SDL_TRUE : SDL_FALSE);
}

#ifdef SUPPORT_OPENGL
void* SDLWindow::getOpenGLFunctionPointer(const char* functionName) {
    return SDL_GL_GetProcAddress(functionName);
}
#endif

}
