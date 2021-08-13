/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2015, Christoph Neuhauser
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

#include <GL/glew.h>
#include "AppSettings.hpp"
#include <Utils/File/Logfile.hpp>
#include <Utils/File/FileUtils.hpp>
#include <Graphics/Renderer.hpp>
#include <Graphics/Mesh/Material.hpp>
#include <Utils/Timer.hpp>
#include <SDL/SDLWindow.hpp>
#include <SDL/Input/SDLMouse.hpp>
#include <SDL/Input/SDLKeyboard.hpp>
#include <SDL/Input/SDLGamepad.hpp>
//#include <SDL2/SDL_ttf.h>
#include <ImGui/ImGuiWrapper.hpp>
#include <iostream>
#include <fstream>
#include <boost/algorithm/string/trim.hpp>
#include <boost/filesystem.hpp>

#ifdef USE_BOOST_LOCALE
#include <boost/locale.hpp>
#endif

#ifdef SUPPORT_OPENGL
#include <Graphics/OpenGL/RendererGL.hpp>
#include <Graphics/OpenGL/ShaderManager.hpp>
#include <Graphics/OpenGL/TextureManager.hpp>
#include <Graphics/OpenGL/SystemGL.hpp>
#endif

#ifdef SUPPORT_VULKAN
#include <Graphics/Scene/Camera.hpp>
#include <Graphics/Vulkan/Utils/Instance.hpp>
#include <Graphics/Vulkan/Utils/Swapchain.hpp>
#include <Graphics/Vulkan/Utils/Device.hpp>
#include <Graphics/Vulkan/Shader/ShaderManager.hpp>
#endif

#ifdef _WIN32
#include <windows.h>
#include <VersionHelpers.h>
#ifdef SUPPORT_VULKAN
#include <vulkan/vulkan_win32.h>
#endif
#endif

namespace sgl {

DLL_OBJECT TimerInterface *Timer = NULL;

#ifdef SUPPORT_OPENGL
DLL_OBJECT RendererInterface *Renderer = NULL;
DLL_OBJECT ShaderManagerInterface *ShaderManager = NULL;
DLL_OBJECT TextureManagerInterface *TextureManager = NULL;
DLL_OBJECT MaterialManagerInterface *MaterialManager = NULL;
#endif

#ifdef SUPPORT_VULKAN
namespace vk {
DLL_OBJECT ShaderManagerVk *ShaderManager = NULL;
}
#endif

DLL_OBJECT MouseInterface *Mouse = NULL;
DLL_OBJECT KeyboardInterface *Keyboard = NULL;
DLL_OBJECT GamepadInterface *Gamepad = NULL;

#ifdef WIN32
// Don't upscale window content on Windows with High-DPI settings
void setDPIAware()
{
    bool minWin81 = IsWindows8Point1OrGreater();//IsWindowsVersionOrGreater(HIBYTE(0x0603), LOBYTE(0x0603), 0); // IsWindows8Point1OrGreater
    if (minWin81) {
        HMODULE library = LoadLibrary("User32.dll");
        typedef BOOL (__stdcall *SetProcessDPIAware_Function)();
        SetProcessDPIAware_Function setProcessDPIAware = (SetProcessDPIAware_Function)GetProcAddress(library, "SetProcessDPIAware");
        setProcessDPIAware();
        FreeLibrary((HMODULE)library);
    } else {
        typedef BOOL (__stdcall *SetProcessDPIAware_Function)();
        HMODULE library = LoadLibrary("User32.dll");
        SetProcessDPIAware_Function setProcessDPIAware = (SetProcessDPIAware_Function)GetProcAddress(library, "SetProcessDPIAware");
        setProcessDPIAware();
        FreeLibrary((HMODULE)library);
    }
}
#else
void setDPIAware()
{
}
#endif

AppSettings::AppSettings() {
    Logfile::get()->createLogfile((FileUtils::get()->getConfigDirectory() + "Logfile.html").c_str(), "ShapeDetector");

    operatingSystem = OperatingSystem::UNKNOWN;
#ifdef WIN32
    operatingSystem = OperatingSystem::WINDOWS;
#endif
#ifdef __linux__
    operatingSystem = OperatingSystem::LINUX;
#endif
#ifdef ANDROID
    operatingSystem = OperatingSystem::ANDROID;
#endif
#ifdef MACOSX
    operatingSystem = OperatingSystem::MACOSX;
#endif
}

// Load the settings from the configuration file
void AppSettings::loadSettings(const char *filename)
{
    settings.loadFromFile(filename);
    settingsFilename = filename;
}

void SettingsFile::saveToFile(const char *filename)
{
    std::ofstream file(filename);
    file << "{\n";

    for (auto it = settings.begin(); it != settings.end(); ++it) {
        file << "\"" + it->first + "\": \"" + it->second  + "\"\n";
    }

    file << "}\n";
    file.close();
}

void SettingsFile::loadFromFile(const char *filename)
{
    // VERY basic and lacking JSON parser
    std::ifstream file(filename);
    if (!file.is_open()) {
        return;
    }
    std::string line;

    while (std::getline(file, line)) {
        boost::trim(line);
        if (line == "{" || line == "}") {
            continue;
        }

        size_t string1Start = line.find("\"")+1;
        size_t string1End = line.find("\"", string1Start);
        size_t string2Start = line.find("\"", string1End+1)+1;
        size_t string2End = line.find("\"", string2Start);

        std::string key = line.substr(string1Start, string1End-string1Start);
        std::string value = line.substr(string2Start, string2End-string2Start);
        settings.insert(make_pair(key, value));
    }

    file.close();
}

Window *AppSettings::createWindow()
{
#ifdef USE_BOOST_LOCALE
    boost::locale::generator gen;
    std::locale l = gen("");
    std::locale::global(l);
#endif

    // Make sure the "Data" directory exists. If not: Us the "Data" directory in the parent folder if it exists.
    if (!hasCustomDataDirectory && !sgl::FileUtils::get()->exists("Data")
            && sgl::FileUtils::get()->directoryExists("../Data")) {
        dataDirectory = "../Data/";
        hasCustomDataDirectory = true;
    }
    if (!sgl::FileUtils::get()->directoryExists(dataDirectory)) {
        sgl::Logfile::get()->writeError(
                "Error: AppSettings::createWindow: Data directory \"" + dataDirectory + "\" does not exist.");
        exit(1);
    }

    // Disable upscaling on Windows with High-DPI settings
    setDPIAware();

    // There may only be one instance of a window for now!
    static int i = 0; i++;
    if (i != 1) {
        Logfile::get()->writeError("ERROR: AppSettings::createWindow: More than one instance of a window created!");
        return NULL;
    }

    // Initialize SDL - the only window system for now (support for Qt is planned).
    if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
        Logfile::get()->writeError("ERROR: AppSettings::createWindow: Couldn't initialize SDL!");
        Logfile::get()->writeError(std::string() + "SDL Error: " + SDL_GetError());
    }

#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN) {
        instance = new vk::Instance;
    }
#endif

    SDLWindow *window = new SDLWindow;
    WindowSettings windowSettings = window->deserializeSettings(settings);
    window->initialize(windowSettings, renderSystem);

    mainWindow = window;
    return window;
}

HeadlessData AppSettings::createHeadless() {
    HeadlessData headlessData;
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN) {
        bool debugContext = false;
        settings.getValueOpt("window-debugContext", debugContext);
        instance->createInstance({}, debugContext);
        headlessData.instance = instance;
    } else {
#endif
        headlessData.mainWindow = createWindow();
#ifdef SUPPORT_VULKAN
    }
#endif
    return headlessData;
}

#if defined(SUPPORT_OPENGL) && defined(SUPPORT_VULKAN)
void checkGlExtension(const char *extensionName, VulkanInteropCapabilities& vulkanInteropCapabilities) {
    if (!sgl::SystemGL::get()->isGLExtensionAvailable(extensionName)) {
        sgl::Logfile::get()->writeInfo(
                std::string() + "Warning in AppSettings::initializeVulkanInteropSupport: The OpenGL extension "
                + extensionName + " is not supported on this system. Disabling external memory support.");
        vulkanInteropCapabilities = VulkanInteropCapabilities::NO_INTEROP;
    }
}

void AppSettings::initializeVulkanInteropSupport(
        const std::vector<const char*>& requiredDeviceExtensionNames,
        const std::vector<const char*>& optionalDeviceExtensionNames,
        const vk::DeviceFeatures& requestedDeviceFeatures) {
    vulkanInteropCapabilities = VulkanInteropCapabilities::EXTERNAL_MEMORY;

    if (operatingSystem != sgl::OperatingSystem::LINUX && operatingSystem != sgl::OperatingSystem::ANDROID
            && operatingSystem != sgl::OperatingSystem::WINDOWS) {
        sgl::Logfile::get()->writeInfo(
                "Warning in AppSettings::initializeVulkanInteropSupport: Only Windows and Linux-based systems "
                "support sharing memory objects and semaphores between OpenGL and Vulkan applications.");
        vulkanInteropCapabilities = VulkanInteropCapabilities::NO_INTEROP;
    }

    instance = new vk::Instance;

    if (!sgl::SystemGL::get()->isGLExtensionAvailable("GL_EXT_memory_object")) {
        sgl::Logfile::get()->writeInfo(
                "Warning in AppSettings::initializeVulkanInteropSupport: The OpenGL extension GL_EXT_memory_object "
                "is not supported on this system. Disabling external memory support.");
        vulkanInteropCapabilities = VulkanInteropCapabilities::NO_INTEROP;
    }
    if (!sgl::SystemGL::get()->isGLExtensionAvailable("GL_EXT_semaphore")) {
        sgl::Logfile::get()->writeInfo(
                "Warning in AppSettings::initializeVulkanInteropSupport: The OpenGL extension GL_EXT_semaphore "
                "is not supported on this system. Disabling external memory support.");
        vulkanInteropCapabilities = VulkanInteropCapabilities::NO_INTEROP;
    }
    std::vector<const char*> openglExtensionNames = {
            "GL_EXT_memory_object", "GL_EXT_semaphore"
    };
#ifdef _WIN32
    // Use Win32 memory objects.
    openglExtensionNames.push_back("GL_EXT_memory_object_win32");
    openglExtensionNames.push_back("GL_EXT_semaphore_win32");
#else
    // Use POSIX file descriptors for external handles.
    openglExtensionNames.push_back("GL_EXT_memory_object_fd");
    openglExtensionNames.push_back("GL_EXT_semaphore_fd");
#endif
    for (const char *extensionName : openglExtensionNames) {
        if (vulkanInteropCapabilities == VulkanInteropCapabilities::NO_INTEROP) {
            break;
        }
        checkGlExtension(extensionName, vulkanInteropCapabilities);
    }

    std::vector<const char*> instanceExtensionNames;
    if (vulkanInteropCapabilities == VulkanInteropCapabilities::EXTERNAL_MEMORY) {
        instanceExtensionNames = {
                VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
                VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME
        };
        if (!instance->getInstanceExtensionsAvailable(instanceExtensionNames)) {
            sgl::Logfile::get()->writeInfo(
                    "Warning in AppSettings::initializeVulkanInteropSupport: The Vulkan instance extensions "
                    "VK_KHR_external_memory_capabilities or VK_KHR_external_semaphore_capabilities are not supported "
                    "on this system. Disabling external memory support.");
            vulkanInteropCapabilities = VulkanInteropCapabilities::NO_INTEROP;
            instanceExtensionNames = {};
        }
    }

    instance->createInstance(
            instanceExtensionNames, getMainWindow()->getWindowSettings().debugContext);
    primaryDevice = new sgl::vk::Device;
    primaryDevice->setOpenGlInteropEnabled(true);

    std::vector<const char*> externalMemoryDeviceExtensionNames;
    if (vulkanInteropCapabilities == VulkanInteropCapabilities::EXTERNAL_MEMORY) {
#ifdef _WIN32
        externalMemoryDeviceExtensionNames = {
                VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME, VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
                VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME, VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME
        };
#else
        externalMemoryDeviceExtensionNames = {
                VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME, VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
                VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME, VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME
        };
#endif
    }
    std::vector<const char*> optionalDeviceExtensionNamesAll = externalMemoryDeviceExtensionNames;
    optionalDeviceExtensionNamesAll.insert(
            optionalDeviceExtensionNamesAll.end(), optionalDeviceExtensionNames.begin(),
            optionalDeviceExtensionNames.end());

    primaryDevice->createDeviceHeadless(
            instance, requiredDeviceExtensionNames, optionalDeviceExtensionNamesAll, requestedDeviceFeatures);

    for (const char* deviceExtension : externalMemoryDeviceExtensionNames) {
        if (!primaryDevice->isDeviceExtensionSupported(deviceExtension)) {
            sgl::Logfile::get()->writeInfo(
                    std::string() + "Warning in AppSettings::initializeVulkanInteropSupport: The Vulkan device "
                    "extension " + deviceExtension + " is not supported on this system. Disabling external memory "
                    "support.");
            vulkanInteropCapabilities = VulkanInteropCapabilities::NO_INTEROP;
            break;
        }
    }
}
#endif

void AppSettings::setDataDirectory(const std::string& dataDirectory) {
        char lastChar = dataDirectory.empty() ? '\0' : dataDirectory.at(dataDirectory.size() - 1);
        if (lastChar != '/' && lastChar != '\\') {
            this->dataDirectory = dataDirectory + "/";
        } else {
            this->dataDirectory = dataDirectory;
        }
        hasCustomDataDirectory = true;
    }

void AppSettings::setRenderSystem(RenderSystem renderSystem) {
    assert(!mainWindow);
    this->renderSystem = renderSystem;
}

void AppSettings::initializeSubsystems()
{
    /*if (TTF_Init() == -1) {
        Logfile::get()->writeError("ERROR: SDLWindow::initializeAudio: Couldn't initialize SDL_ttf!");
        Logfile::get()->writeError(std::string() + "SDL_ttf initialization error: " + TTF_GetError());
    }*/

    // Create the subsystem implementations
    Timer = new TimerInterface;
    //AudioManager = new SDLMixerAudioManager;

#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        TextureManager = new TextureManagerGL;
        ShaderManager = new ShaderManagerGL;
        Renderer = new RendererGL;
        MaterialManager = new MaterialManagerInterface;
        SystemGL::get();
    }
#endif
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN) {
        Camera::depthRange = Camera::DEPTH_RANGE_ZERO_ONE;
    }
    if (primaryDevice) {
        vk::ShaderManager = new vk::ShaderManagerVk(primaryDevice);
    }
#endif

    Mouse = new SDLMouse;
    Keyboard = new SDLKeyboard;
    Gamepad = new SDLGamepad;

    if (useGUI) {
        ImGuiWrapper::get()->initialize(fontRangesData, useDocking, useMultiViewport, uiScaleFactor);
    }
}

void AppSettings::release()
{
#ifdef SUPPORT_VULKAN
    if (primaryDevice) {
        primaryDevice->waitIdle();
    }
#endif

    mainWindow->serializeSettings(settings);
    settings.saveToFile(settingsFilename.c_str());

    if (useGUI) {
        ImGuiWrapper::get()->shutdown();
    }

#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        if (Renderer) {
            delete Renderer;
            Renderer = nullptr;
        }
        if (ShaderManager) {
            delete ShaderManager;
            ShaderManager = nullptr;
        }
        if (TextureManager) {
            delete TextureManager;
            TextureManager = nullptr;
        }
        delete MaterialManager;
    }
#endif

#ifdef SUPPORT_VULKAN
    if (primaryDevice) {
        if (vk::ShaderManager) {
            delete vk::ShaderManager;
            vk::ShaderManager = nullptr;
        }
    }
#endif

    //delete AudioManager;
    delete Timer;

#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN) {
        if (swapchain) {
            delete swapchain;
            swapchain = nullptr;
        }
    }
#endif

    if (mainWindow) {
        delete mainWindow;
        mainWindow = nullptr;
    }


#ifdef SUPPORT_VULKAN
    if (primaryDevice) {
        delete primaryDevice;
        primaryDevice = nullptr;
    }
    if (instance) {
        delete instance;
        instance = nullptr;
    }
#endif

    //Mix_CloseAudio();
    //TTF_Quit();
    SDL_Quit();
}

void AppSettings::setLoadGUI(
        const unsigned short* fontRangeData, bool useDocking, bool useMultiViewport, float uiScaleFactor) {
    useGUI = true;
    this->fontRangesData = fontRangeData;
    this->useDocking = useDocking;
    this->useMultiViewport = useMultiViewport;
    this->uiScaleFactor = uiScaleFactor;
}

Window *AppSettings::getMainWindow()
{
    return mainWindow;
}

Window *AppSettings::setMainWindow(Window *window)
{
    return mainWindow;
}

void AppSettings::getCurrentDisplayMode(int& width, int& height, int& refreshRate, int displayIndex)
{
    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(displayIndex, &displayMode);
    width = displayMode.w;
    height = displayMode.h;
    refreshRate = displayMode.refresh_rate;
}

void AppSettings::getDesktopDisplayMode(int& width, int& height, int& refreshRate, int displayIndex)
{
    SDL_DisplayMode displayMode;
    SDL_GetDesktopDisplayMode(displayIndex, &displayMode);
    width = displayMode.w;
    height = displayMode.h;
    refreshRate = displayMode.refresh_rate;
}

glm::ivec2 AppSettings::getCurrentDisplayModeResolution(int displayIndex)
{
    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(displayIndex, &displayMode);
    return glm::ivec2(displayMode.w, displayMode.h);
}

glm::ivec2 AppSettings::getDesktopResolution(int displayIndex)
{
    SDL_DisplayMode displayMode;
    SDL_GetDesktopDisplayMode(displayIndex, &displayMode);
    return glm::ivec2(displayMode.w, displayMode.h);
}

}
