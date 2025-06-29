/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2024, Christoph Neuhauser
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

#include <Math/Math.hpp>
#include <Utils/StringUtils.hpp>
#include <Utils/AppSettings.hpp>
#include <Utils/File/FileUtils.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/Events/EventManager.hpp>
#include <Utils/Json/ConversionHelpers.hpp>
#include <Graphics/Utils/HiDPI.hpp>
#include <Graphics/Texture/Bitmap.hpp>
#include "Input/GlfwKeyboard.hpp"
#include "Input/GlfwMouse.hpp"
#include "Input/GlfwGamepad.hpp"

#ifdef SUPPORT_OPENGL
#include <GL/glew.h>
#endif

#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/libs/volk/volk.h>
#include <Graphics/Vulkan/Utils/Instance.hpp>
#include <Graphics/Vulkan/Utils/Device.hpp>
#include <Graphics/Vulkan/Utils/Swapchain.hpp>
#endif

#include <GLFW/glfw3.h>
#ifdef SUPPORT_WEBGPU
#include <glfw3webgpu.h>

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
#include <utility>
#endif

#include "GlfwWindow.hpp"

namespace sgl {

GlfwWindow::GlfwWindow() = default;

GlfwWindow::~GlfwWindow() {
    // glfwDestroyCursor should be OK for system cursors (ImGui also does it).
    for (auto& entry : cursors) {
        glfwDestroyCursor(entry.second);
    }
    cursors.clear();
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
    glfwDestroyWindow(glfwWindow);
    Logfile::get()->write("Closing GLFW window.");
}

void GlfwWindow::errorCheck() {
    errorCheckGlfw();
}

void GlfwWindow::errorCheckGlfw() {
    const char* description = nullptr;
    int errorCode = glfwGetError(&description);
    if (errorCode != GLFW_NO_ERROR && description) {
        sgl::Logfile::get()->writeError(
                "Error in AppSettings::createWindow: glfwInit failed (code 0x" + sgl::toHexString(errorCode) + "): "
                + description);
    } else if (errorCode != GLFW_NO_ERROR) {
        sgl::Logfile::get()->writeError(
                "Error in AppSettings::createWindow: glfwInit failed (code 0x" + sgl::toHexString(errorCode) + ")");
    }
}

void GlfwWindow::initialize(const WindowSettings &settings, RenderSystem renderSystem) {
    this->renderSystem = renderSystem;
    this->windowSettings = settings;

    errorCheck();

    int redBits = 8, greenBits = 8, blueBits = 8, alphaBits = 8, refreshRate = 0;
    GLFWmonitor* fullscreenMonitor = nullptr;
    if (windowSettings.isFullscreen) {
        // TODO: Remember monitor over application runs?
        fullscreenMonitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(fullscreenMonitor);
        redBits = mode->redBits;
        greenBits = mode->greenBits;
        blueBits = mode->blueBits;
        refreshRate = mode->refreshRate;
        windowSettings.width = mode->width;
        windowSettings.height = mode->height;
    }

    if (refreshRate > 0) {
        glfwWindowHint(GLFW_REFRESH_RATE, refreshRate);
    }

    if (windowSettings.isMaximized) {
        glfwWindowHint(GLFW_MAXIMIZED, windowSettings.isMaximized);
    }

#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        // Set the window attributes
        glfwWindowHint(GLFW_RED_BITS, redBits);
        glfwWindowHint(GLFW_GREEN_BITS, greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, blueBits);
        glfwWindowHint(GLFW_ALPHA_BITS, alphaBits);
        glfwWindowHint(GLFW_DEPTH_BITS, windowSettings.depthSize);
        glfwWindowHint(GLFW_STENCIL_BITS, windowSettings.stencilSize);
        //glfwWindowHint(GLFW_STEREO, TODO);
        glfwWindowHint(GLFW_DOUBLEBUFFER, 1);
        //glfwWindowHint(GLFW_SRGB_CAPABLE, 0);

        // Set core profile
        if (renderSystem == RenderSystem::OPENGL) {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        } else if (renderSystem == RenderSystem::OPENGLES) {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // TODO
        //glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
        //glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
        //glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_OSMESA_CONTEXT_API);

        if (windowSettings.debugContext) {
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
        }

        if (windowSettings.multisamples != 0) {
            // Context creation fails (at least on GLX) if multisample samples are too high, so query the maximum beforehand
            windowSettings.multisamples = getMaxSamplesGLImpl(windowSettings.multisamples);
        }
        if (windowSettings.multisamples != 0) {
            glfwWindowHint(GLFW_SAMPLES, windowSettings.multisamples);
        }
    }
#endif

    if (renderSystem != RenderSystem::OPENGL) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }


#ifdef __APPLE__
    /*
     * Check if the application is run from an app bundle (only then NSHighResolutionCapable is set):
     *  https://stackoverflow.com/questions/58036928/check-if-c-program-is-running-as-an-app-bundle-or-command-line-on-mac
     */
    CFBundleRef bundle = CFBundleGetMainBundle();
    CFURLRef bundleUrl = CFBundleCopyBundleURL(bundle);
    CFStringRef uti;
    bool useHiDpi = false;
    if (CFURLCopyResourcePropertyForKey(bundleUrl, kCFURLTypeIdentifierKey, &uti, NULL) && uti
            && UTTypeConformsTo(uti, kUTTypeApplicationBundle)) {
        useHiDpi = true;
    }
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, int(useHiDpi));
#endif
    if (windowSettings.isResizable) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_RESIZABLE);
    }

#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN && !windowSettings.useDownloadSwapchain) {
        if (!glfwVulkanSupported()) {
            sgl::Logfile::get()->writeError("Error in GlfwWindow::initialize: glfwVulkanSupported returned false.");
            return;
        }
    }
#endif

    // Create the window
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindow = glfwCreateWindow(
            windowSettings.width, windowSettings.height, FileUtils::get()->getAppName().c_str(),
            fullscreenMonitor, nullptr);

    if (!glfwWindow) {
        sgl::Logfile::get()->writeError("Error in GlfwWindow::initialize: glfwCreateWindow failed.");
        return;
    }

    glfwSetWindowUserPointer(glfwWindow, (void*)this);
    // TODO: Position

#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        glfwMakeContextCurrent(glfwWindow);
    }
#endif
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN && !windowSettings.useDownloadSwapchain) {
        /*
         * The array "instanceExtensionNames" holds the name of all extensions that get requested. First, user-specified
         * extensions are added. Then, extensions required by GLFW are added using "glfwGetRequiredInstanceExtensions".
         */
        std::vector<const char*> instanceExtensionNames =
                sgl::AppSettings::get()->getRequiredVulkanInstanceExtensions();
        uint32_t extensionCount;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
        for (uint32_t i = 0; i < extensionCount; i++) {
            instanceExtensionNames.push_back(glfwExtensions[i]);
        }

        sgl::vk::Instance* instance = sgl::AppSettings::get()->getVulkanInstance();
        instance->createInstance(instanceExtensionNames, windowSettings.debugContext);

        VkResult result = glfwCreateWindowSurface(
                instance->getVkInstance(), glfwWindow, nullptr, &windowSurface);
        if (result != VK_SUCCESS) {
            sgl::Logfile::get()->throwError(
                    std::string() + "Error in GlfwWindow::initialize: Failed to create a Vulkan surface.");
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
                    std::string() + "Error in GlfwWindow::initialize: Failed to create a WebGPU instance.");
        }
        webgpuSurface = glfwGetWGPUSurface(instance->getWGPUInstance(), glfwWindow);
        if (!webgpuSurface) {
            sgl::Logfile::get()->throwError(
                    std::string() + "Error in GlfwWindow::initialize: Failed to create a WebGPU surface.");
        }
    }
#endif

#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL && windowSettings.multisamples != 0) {
        glEnable(GL_MULTISAMPLE);
    }

    if (renderSystem == RenderSystem::OPENGL) {
        if (windowSettings.vSync) {
            glfwSwapInterval(-1);

            // TODO: How to detect that swap interval is not supported? Probably change in error callback is necessary.
            /*const char* sdlError = SDL_GetError();
            if (sgl::stringContains(sdlError, "Negative swap interval unsupported")) {
                Logfile::get()->writeInfo(std::string() + "VSYNC Info: " + sdlError);
                SDL_ClearError();
                SDL_GL_SetSwapInterval(1);
            }*/
        } else {
            glfwSwapInterval(0);
        }
    }
#endif

    // Did something fail during the initialization?
    errorCheck();

#if GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 4

    auto glfwPlatform = glfwGetPlatform();
    usesX11Backend = (glfwPlatform == GLFW_PLATFORM_X11);
    usesWaylandBackend = (glfwPlatform == GLFW_PLATFORM_WAYLAND);

#else // GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 4

    // We will assume a system with an old GLFW version most likely uses X11 instead of Wayland.
#ifdef __linux__
    usesX11Backend = true;
    usesWaylandBackend = false;
#endif

#endif // GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 4

#ifdef __linux__
    const char* waylandDisplayVar = getenv("WAYLAND_DISPLAY");
    if (usesX11Backend && waylandDisplayVar) {
        usesXWaylandBackend = true;
    }
#endif

    windowSettings.pixelWidth = windowSettings.width;
    windowSettings.pixelHeight = windowSettings.height;
    glfwGetWindowSize(glfwWindow, &windowSettings.width, &windowSettings.height);
    glfwGetFramebufferSize(glfwWindow, &windowSettings.pixelWidth, &windowSettings.pixelHeight);
    widthOld = windowSettings.width;
    heightOld = windowSettings.height;


    glfwSetKeyCallback(glfwWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        void* userPtr = glfwGetWindowUserPointer(window);
        if (userPtr) {
            reinterpret_cast<GlfwWindow*>(userPtr)->onKey(key, scancode, action, mods);
        }
    });
    glfwSetInputMode(glfwWindow, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetInputMode(glfwWindow, GLFW_LOCK_KEY_MODS, GLFW_TRUE);

    glfwSetCharCallback(glfwWindow, [](GLFWwindow* window, unsigned int codepoint) {
        void* userPtr = glfwGetWindowUserPointer(window);
        if (userPtr) {
            reinterpret_cast<GlfwWindow*>(userPtr)->onChar(codepoint);
        }
    });

    glfwSetCharModsCallback(glfwWindow, [](GLFWwindow* window, unsigned int codepoint, int mods) {
        void* userPtr = glfwGetWindowUserPointer(window);
        if (userPtr) {
            reinterpret_cast<GlfwWindow*>(userPtr)->onCharMods(codepoint, mods);
        }
    });

    glfwSetCursorPosCallback(glfwWindow, [](GLFWwindow* window, double xpos, double ypos) {
        void* userPtr = glfwGetWindowUserPointer(window);
        if (userPtr) {
            reinterpret_cast<GlfwWindow*>(userPtr)->onCursorPos(xpos, ypos);
        }
    });
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(glfwWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    glfwSetCursorEnterCallback(glfwWindow, [](GLFWwindow* window, int entered) {
        void* userPtr = glfwGetWindowUserPointer(window);
        if (userPtr) {
            reinterpret_cast<GlfwWindow*>(userPtr)->onCursorEnter(entered);
        }
    });

    glfwSetMouseButtonCallback(glfwWindow, [](GLFWwindow* window, int button, int action, int mods) {
        void* userPtr = glfwGetWindowUserPointer(window);
        if (userPtr) {
            reinterpret_cast<GlfwWindow*>(userPtr)->onMouseButton(button, action, mods);
        }
    });
    glfwSetInputMode(glfwWindow, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

    glfwSetScrollCallback(glfwWindow, [](GLFWwindow* window, double xoffset, double yoffset) {
        void* userPtr = glfwGetWindowUserPointer(window);
        if (userPtr) {
            reinterpret_cast<GlfwWindow*>(userPtr)->onScroll(xoffset, yoffset);
        }
    });

    glfwSetDropCallback(glfwWindow, [](GLFWwindow* window, int count, const char** paths) {
        void* userPtr = glfwGetWindowUserPointer(window);
        if (userPtr) {
            reinterpret_cast<GlfwWindow*>(userPtr)->onDrop(count, paths);
        }
    });

    glfwSetFramebufferSizeCallback(glfwWindow, [](GLFWwindow* window, int width, int height) {
        void* userPtr = glfwGetWindowUserPointer(window);
        if (userPtr) {
            reinterpret_cast<GlfwWindow*>(userPtr)->onFramebufferSize(width, height);
        }
    });

    glfwSetWindowContentScaleCallback(glfwWindow, [](GLFWwindow* window, float xscale, float yscale) {
        void* userPtr = glfwGetWindowUserPointer(window);
        if (userPtr) {
            reinterpret_cast<GlfwWindow*>(userPtr)->onWindowContentScale(xscale, yscale);
        }
    });

    glfwSetWindowMaximizeCallback(glfwWindow, [](GLFWwindow* window, int maximized) {
        void* userPtr = glfwGetWindowUserPointer(window);
        if (userPtr) {
            reinterpret_cast<GlfwWindow*>(userPtr)->setIsMaximized(maximized == GLFW_TRUE);
        }
    });

    glfwSetJoystickCallback([](int jid, int event) {
        static_cast<GlfwGamepad*>(Gamepad)->onJoystick(jid, event);
    });


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
            Logfile::get()->writeError(std::string() + "Error: GlfwWindow::initializeOpenGL: glewInit: " + (char*)glewGetErrorString(glewError));
        }

        glViewport(0, 0, windowSettings.pixelWidth, windowSettings.pixelHeight);
    }
#endif
}


void GlfwWindow::toggleFullscreen(bool nativeFullscreen) {
    // TODO: Ignore nativeFullscreen for now.
    windowSettings.isFullscreen = !windowSettings.isFullscreen;
    GLFWmonitor* fullscreenMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(fullscreenMonitor);
    if (windowSettings.isFullscreen) {
        widthOld = windowSettings.width;
        heightOld = windowSettings.height;
        if (nativeFullscreen) {
            glfwSetWindowMonitor(glfwWindow, fullscreenMonitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        } else {
            glfwSetWindowMonitor(glfwWindow, fullscreenMonitor, 0, 0, windowSettings.width, windowSettings.height, mode->refreshRate);
        }
    } else {
        windowSettings.width = widthOld;
        windowSettings.height = heightOld;
        glfwSetWindowMonitor(glfwWindow, nullptr, 0, 0, windowSettings.width, windowSettings.height, mode->refreshRate);
    }
}

void GlfwWindow::setWindowVirtualSize(int width, int height) {
    windowSettings.width = width;
    windowSettings.height = height;
    windowSettings.pixelWidth = width;
    windowSettings.pixelHeight = height;

    int oldWidth = 0, oldHeight = 0;
    int oldPixelWidth = 0, oldPixelHeight = 0;
    glfwGetWindowSize(glfwWindow, &oldWidth, &oldHeight);
    glfwGetFramebufferSize(glfwWindow, &oldPixelWidth, &oldPixelHeight);
    windowSettings.pixelWidth = width * oldPixelWidth / oldWidth;
    windowSettings.pixelWidth = height * oldPixelHeight / oldHeight;

    glfwSetWindowSize(glfwWindow, windowSettings.width, windowSettings.height);
    if (renderSystem != RenderSystem::VULKAN) {
        if (windowSettings.pixelWidth != 0 && windowSettings.pixelHeight != 0) {
            EventManager::get()->queueEvent(EventPtr(new Event(RESOLUTION_CHANGED_EVENT)));
        }
    }
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN && !windowSettings.useDownloadSwapchain) {
        //sgl::vk::Device* device = sgl::AppSettings::get()->getPrimaryDevice();
        //device->waitIdle();
    }
#endif
}

void GlfwWindow::setWindowPixelSize(int width, int height) {
    windowSettings.width = width;
    windowSettings.height = height;
    windowSettings.pixelWidth = width;
    windowSettings.pixelHeight = height;

    int oldWidth = 0, oldHeight = 0;
    int oldPixelWidth = 0, oldPixelHeight = 0;
    glfwGetWindowSize(glfwWindow, &oldWidth, &oldHeight);
    glfwGetFramebufferSize(glfwWindow, &oldPixelWidth, &oldPixelHeight);
    windowSettings.width = width * oldWidth / oldPixelWidth;
    windowSettings.height = height * oldHeight / oldPixelHeight;

    glfwSetWindowSize(glfwWindow, windowSettings.width, windowSettings.height);
    if (renderSystem != RenderSystem::VULKAN) {
        if (windowSettings.pixelWidth != 0 && windowSettings.pixelHeight != 0) {
            EventManager::get()->queueEvent(EventPtr(new Event(RESOLUTION_CHANGED_EVENT)));
        }
    }
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN && !windowSettings.useDownloadSwapchain) {
        //sgl::vk::Device* device = sgl::AppSettings::get()->getPrimaryDevice();
        //device->waitIdle();
    }
#endif
}

glm::ivec2 GlfwWindow::getWindowPosition() {
    int x, y;
    glfwGetWindowPos(glfwWindow, &x, &y);
    return { x, y };
}

void GlfwWindow::setWindowPosition(int x, int y) {
    glfwSetWindowPos(glfwWindow, x, y);
}

void GlfwWindow::update() {
}

bool GlfwWindow::processEvents() {
    if (isFirstFrame) {
        if (windowSettings.savePosition && windowSettings.windowPosition.x != std::numeric_limits<int>::min()
                && !usesWaylandBackend) {
            setWindowPosition(windowSettings.windowPosition.x, windowSettings.windowPosition.y);
        }
        onFramebufferSize(windowSettings.width, windowSettings.height);
        isFirstFrame = false;
    }

    glfwPollEvents();
    isRunning = !glfwWindowShouldClose(glfwWindow);
    return isRunning;
}

void GlfwWindow::onKey(int key, int scancode, int action, int mods) {
    //if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    //    glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
    //}
    static_cast<GlfwKeyboard*>(Keyboard)->onKey(key, scancode, action, mods);
    if (onKeyCallback) {
        onKeyCallback(key, scancode, action, mods);
    }
}

void GlfwWindow::onChar(unsigned int codepoint) {
    static_cast<GlfwKeyboard*>(Keyboard)->onChar(codepoint);
}

void GlfwWindow::onCharMods(unsigned int codepoint, int mods) {
    static_cast<GlfwKeyboard*>(Keyboard)->onCharMods(codepoint, mods);
}

void GlfwWindow::onCursorPos(double xpos, double ypos) {
    static_cast<GlfwMouse*>(Mouse)->onCursorPos(xpos, ypos);
}

void GlfwWindow::onCursorEnter(int entered) {
    static_cast<GlfwMouse*>(Mouse)->onCursorEnter(entered);
}

void GlfwWindow::onMouseButton(int button, int action, int mods) {
    static_cast<GlfwMouse*>(Mouse)->onMouseButton(button, action, mods);
}

void GlfwWindow::onScroll(double xoffset, double yoffset) {
    static_cast<GlfwMouse*>(Mouse)->onScroll(xoffset, yoffset);
}

void GlfwWindow::onDrop(int count, const char** paths) {
    std::vector<std::string> droppedFiles;
    for (int i = 0; i < count; i++) {
        droppedFiles.emplace_back(paths[i]);
    }
    if (onDropCallback) {
        onDropCallback(droppedFiles);
    }
}

void GlfwWindow::onFramebufferSize(int width, int height) {
    windowSettings.width = width;
    windowSettings.height = height;
    windowSettings.pixelWidth = windowSettings.width;
    windowSettings.pixelHeight = windowSettings.height;
    glfwGetWindowSize(glfwWindow, &windowSettings.width, &windowSettings.height);
    glfwGetFramebufferSize(glfwWindow, &windowSettings.pixelWidth, &windowSettings.pixelHeight);

#ifdef SUPPORT_WEBGPU
    if (renderSystem == RenderSystem::WEBGPU) {
        webgpu::Swapchain* swapchain = AppSettings::get()->getWebGPUSwapchain();
        if (swapchain) {
            swapchain->recreateSwapchain();
        }
    }
#endif
    if (renderSystem != RenderSystem::VULKAN) {
        if (windowSettings.pixelWidth != 0 && windowSettings.pixelHeight != 0) {
            EventManager::get()->queueEvent(EventPtr(new Event(RESOLUTION_CHANGED_EVENT)));
        }
    }
#ifdef SUPPORT_VULKAN
    else {
        vk::Swapchain* swapchain = AppSettings::get()->getSwapchain();
        if (swapchain && !swapchain->getIsWaitingForResizeEnd()) {
            swapchain->recreateSwapchain();
        }
    }
#endif
    updateHighDPIScaleFactor();
}

void GlfwWindow::onWindowContentScale(float xscale, float yscale) {
    updateHighDPIScaleFactor();
}

void GlfwWindow::setIsMaximized(bool isMaximized) {
    windowSettings.isMaximized = isMaximized;
}


void GlfwWindow::setRefreshRateCallback(std::function<void(int)> callback) {
    refreshRateCallback = std::move(callback);
}

void GlfwWindow::setOnKeyCallback(std::function<void(int, int, int, int)> callback) {
    onKeyCallback = std::move(callback);
}

void GlfwWindow::setOnDropCallback(std::function<void(const std::vector<std::string>&)> callback) {
    onDropCallback = std::move(callback);
}


void GlfwWindow::clear(const Color &color) {
#ifdef SUPPORT_OPENGL
    glClearColor(color.getFloatR(), color.getFloatG(), color.getFloatB(), 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
}

void GlfwWindow::flip() {
    if (renderSystem == RenderSystem::OPENGL) {
        glfwSwapBuffers(glfwWindow);
    } else {
        throw std::runtime_error("GlfwWindow::flip: Unsupported operation when using Vulkan.");
    }
}

void GlfwWindow::serializeSettings(SettingsFile &settings) {
    auto& windowVals = settings.getSettingsObject()["window"];
    windowVals["width"] = windowSettings.width;
    windowVals["height"] = windowSettings.height;
    windowVals["fullscreen"] = windowSettings.isFullscreen;
    windowVals["maximized"] = windowSettings.isMaximized;
    windowVals["resizable"] = windowSettings.isResizable;
    windowVals["multisamples"] = windowSettings.multisamples;
    windowVals["depthSize"] = windowSettings.depthSize;
    windowVals["stencilSize"] = windowSettings.stencilSize;
    windowVals["vSync"] = windowSettings.vSync;
#ifndef __EMSCRIPTEN__
    windowVals["savePosition"] = windowSettings.savePosition;
    if (windowSettings.savePosition) {
        windowSettings.windowPosition = getWindowPosition();
        windowVals["windowPosition"] = glmVecToJsonValue(windowSettings.windowPosition);
    }
#endif
    windowVals["useDownloadSwapchain"] = windowSettings.useDownloadSwapchain;
}

WindowSettings GlfwWindow::deserializeSettings(const SettingsFile &settings) {
    WindowSettings windowSettings;
    if (!settings.getSettingsObject().hasMember("window")
            || !settings.getSettingsObject()["window"].hasMember("width")
            || !settings.getSettingsObject()["window"].hasMember("height")) {
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
    const auto& windowVals = settings.getSettingsObject()["window"];
    getJsonOptional(windowVals, "width", windowSettings.width);
    getJsonOptional(windowVals, "height", windowSettings.height);
    getJsonOptional(windowVals, "fullscreen", windowSettings.isFullscreen);
    getJsonOptional(windowVals, "maximized", windowSettings.isMaximized);
    getJsonOptional(windowVals, "resizable", windowSettings.isResizable);
    getJsonOptional(windowVals, "multisamples", windowSettings.multisamples);
    getJsonOptional(windowVals, "depthSize", windowSettings.depthSize);
    getJsonOptional(windowVals, "stencilSize", windowSettings.stencilSize);
    getJsonOptional(windowVals, "vSync", windowSettings.vSync);
    getJsonOptional(windowVals, "debugContext", windowSettings.debugContext);
#ifndef __EMSCRIPTEN__
    getJsonOptional(windowVals, "savePosition", windowSettings.savePosition);
    getJsonOptional(windowVals, "windowPosition", windowSettings.windowPosition);
#endif
    //getJsonOptional(windowVals, "useDownloadSwapchain", windowSettings.useDownloadSwapchain);
    return windowSettings;
}

void GlfwWindow::saveScreenshot(const char* filename) {
    if (renderSystem == RenderSystem::OPENGL) {
#ifdef SUPPORT_OPENGL
        BitmapPtr bitmap(new Bitmap(windowSettings.pixelWidth, windowSettings.pixelHeight, 32));
        glReadPixels(
                0, 0, windowSettings.pixelWidth, windowSettings.pixelHeight,
                GL_RGBA, GL_UNSIGNED_BYTE, bitmap->getPixels());
        bitmap->savePNG(filename, true);
        Logfile::get()->write(
                std::string() + "INFO: GlfwWindow::saveScreenshot: Screenshot saved to \""
                + filename + "\".", BLUE);
#endif
    } else {
        throw std::runtime_error("GlfwWindow::saveScreenshot: Unsupported operation when using Vulkan.");
    }
}

void GlfwWindow::setWindowIconFromFile(const std::string& imageFilename) {
    sgl::BitmapPtr bitmap(new sgl::Bitmap);
    bitmap->fromFile(imageFilename.c_str());
    if (bitmap->getBPP() != 32) {
        Logfile::get()->writeError(
                std::string() + "Error in GlfwWindow::setWindowIconFromFile: Unsupported bit depth.", false);
        return;
    }

    GLFWimage glfwImage{};
    glfwImage.width = bitmap->getW();
    glfwImage.height = bitmap->getH();
    glfwImage.pixels = bitmap->getPixels();
    glfwSetWindowIcon(glfwWindow, 1, &glfwImage);
}

void GlfwWindow::setCursorType(CursorType cursorType) {
    if (currentCursorType == cursorType) {
        return;
    }
    currentCursorType = cursorType;
    if (cursorType == CursorType::DEFAULT) {
        glfwSetCursor(glfwWindow, nullptr);
        return;
    }

    auto it = cursors.find(cursorType);
    if (it != cursors.end()) {
        glfwSetCursor(glfwWindow, it->second);
    } else {
        int shape = -1;
        if (cursorType == CursorType::ARROW) {
            shape = GLFW_ARROW_CURSOR;
        } else if (cursorType == CursorType::IBEAM) {
            shape = GLFW_IBEAM_CURSOR;
        } else if (cursorType == CursorType::WAIT) {
            shape = -1;
        } else if (cursorType == CursorType::CROSSHAIR) {
            shape = GLFW_CROSSHAIR_CURSOR;
        } else if (cursorType == CursorType::WAITARROW) {
            shape = -1;
        } else if (cursorType == CursorType::SIZENWSE) {
#ifdef GLFW_RESIZE_NWSE_CURSOR
            shape = GLFW_RESIZE_NWSE_CURSOR;
#else
            shape = GLFW_HRESIZE_CURSOR;
#endif
        } else if (cursorType == CursorType::SIZENESW) {
#ifdef GLFW_RESIZE_NESW_CURSOR
            shape = GLFW_RESIZE_NESW_CURSOR;
#else
            shape = GLFW_HRESIZE_CURSOR;
#endif
        } else if (cursorType == CursorType::SIZEWE) {
#ifdef GLFW_RESIZE_EW_CURSOR
            shape = GLFW_RESIZE_EW_CURSOR;
#else
            shape = GLFW_HRESIZE_CURSOR;
#endif
        } else if (cursorType == CursorType::SIZENS) {
#ifdef GLFW_RESIZE_NS_CURSOR
            shape = GLFW_RESIZE_NS_CURSOR;
#else
            shape = GLFW_VRESIZE_CURSOR;
#endif
        } else if (cursorType == CursorType::SIZEALL) {
#ifdef GLFW_RESIZE_ALL_CURSOR
            shape = GLFW_RESIZE_ALL_CURSOR;
#else
            shape = GLFW_HRESIZE_CURSOR;
#endif
        }
#ifdef GLFW_NOT_ALLOWED_CURSOR
        else if (cursorType == CursorType::NO) {
            shape = GLFW_NOT_ALLOWED_CURSOR;
        }
#endif
        else if (cursorType == CursorType::HAND) {
            shape = GLFW_HAND_CURSOR; // GLFW_POINTING_HAND_CURSOR
        }
        GLFWcursor* cursor = nullptr;
        if (shape >= 0) {
            cursor = glfwCreateStandardCursor(shape);
        }
        glfwSetCursor(glfwWindow, cursor);
        cursors[cursorType] = cursor;
    }
}


void GlfwWindow::setShowCursor(bool _show) {
    if (showCursor == _show) {
        return;
    }
    showCursor = _show;
    int cursorMode = captureMouse ? GLFW_CURSOR_DISABLED : (showCursor ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
    glfwSetInputMode(glfwWindow, GLFW_CURSOR, cursorMode);
}

#ifdef SUPPORT_OPENGL
void* GlfwWindow::getOpenGLFunctionPointer(const char* functionName) {
    return (void*)glfwGetProcAddress(functionName);
}
#endif

void GlfwWindow::setCaptureMouse(bool _capture) {
    if (captureMouse == _capture) {
        return;
    }
    captureMouse = _capture;
    // TODO: Newer versions of GLFW also provide GLFW_CURSOR_CAPTURED.
    int cursorMode = captureMouse ? GLFW_CURSOR_DISABLED : (showCursor ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
    glfwSetInputMode(glfwWindow, GLFW_CURSOR, cursorMode);
}

}
