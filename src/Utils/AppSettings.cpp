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

#include "AppSettings.hpp"
#include <Utils/StringUtils.hpp>
#include <Utils/Env.hpp>
#include <Utils/Timer.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/File/FileUtils.hpp>
#include <Utils/Json/ConversionHelpers.hpp>
#include <Input/Mouse.hpp>
#include <Input/Keyboard.hpp>
#include <Input/Gamepad.hpp>
#include <Graphics/Window.hpp>
#include <Graphics/Utils/HiDPI.hpp>
#ifndef DISABLE_IMGUI
#include <ImGui/ImGuiWrapper.hpp>
#endif
#include <iostream>
#include <fstream>
#include <filesystem>

#ifdef USE_BOOST_LOCALE
#include <boost/locale.hpp>
#endif

#ifdef SUPPORT_OPENGL
#include <GL/glew.h>
#include <Graphics/Renderer.hpp>
#include <Graphics/Mesh/Material.hpp>
#include <Graphics/OpenGL/RendererGL.hpp>
#include <Graphics/OpenGL/ShaderManager.hpp>
#include <Graphics/OpenGL/TextureManager.hpp>
#include <Graphics/OpenGL/SystemGL.hpp>
#ifndef DISABLE_DEVICE_SELECTION_SUPPORT
#include <Graphics/Utils/DeviceSelection.hpp>
#ifdef __linux__
#include <Graphics/OpenGL/Context/DeviceSelectionEGL.hpp>
#endif
#ifdef _WIN32
#include <Graphics/OpenGL/Context/DeviceSelectionWGL.hpp>
#endif
#endif
#endif

#if (defined(SUPPORT_VULKAN) || defined(SUPPORT_WEBGPU)) && !defined(DISABLE_SCENE_GRAPH_SOURCES)
#include <Graphics/Scene/Camera.hpp>
#endif

#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/Utils/Instance.hpp>
#include <Graphics/Vulkan/Utils/Swapchain.hpp>
#include <Graphics/Vulkan/Utils/Device.hpp>
#include <Graphics/Vulkan/Shader/ShaderManager.hpp>
#ifndef DISABLE_DEVICE_SELECTION_SUPPORT
#include <Graphics/Vulkan/Utils/DeviceSelectionVulkan.hpp>
#endif
#endif

#ifdef SUPPORT_WEBGPU
#include <Graphics/WebGPU/Utils/Instance.hpp>
#include <Graphics/WebGPU/Utils/Device.hpp>
#include <Graphics/WebGPU/Utils/Swapchain.hpp>
#include <Graphics/WebGPU/Shader/ShaderManager.hpp>
#endif

#ifdef SUPPORT_D3D12
#include <Graphics/D3D12/Shader/ShaderManager.hpp>
#endif

#ifdef SUPPORT_SDL
#include <SDL/SDLWindow.hpp>
#include <SDL/Input/SDLMouse.hpp>
#include <SDL/Input/SDLKeyboard.hpp>
#include <SDL/Input/SDLGamepad.hpp>
#endif

#ifdef SUPPORT_GLFW
#include <GLFW/GlfwWindow.hpp>
#include <GLFW/Input/GlfwMouse.hpp>
#include <GLFW/Input/GlfwKeyboard.hpp>
#include <GLFW/Input/GlfwGamepad.hpp>
#include <GLFW/glfw3.h>
#endif

#if defined(SUPPORT_SDL2) && defined(SUPPORT_VULKAN)
#include <SDL2/SDL_vulkan.h>
#endif
#if defined(SUPPORT_SDL3) && defined(SUPPORT_VULKAN)
#include <SDL3/SDL_vulkan.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <VersionHelpers.h>
#include <shellscalingapi.h>
#ifdef SUPPORT_VULKAN
#include <vulkan/vulkan_win32.h>
#endif
#endif

#ifdef __APPLE__
#include <dlfcn.h>
#endif

namespace sgl {

DLL_OBJECT TimerInterface* Timer = nullptr;

#ifdef SUPPORT_OPENGL
DLL_OBJECT RendererInterface* Renderer = nullptr;
DLL_OBJECT ShaderManagerInterface* ShaderManager = nullptr;
DLL_OBJECT TextureManagerInterface* TextureManager = nullptr;
DLL_OBJECT MaterialManagerInterface* MaterialManager = nullptr;
#endif

#ifdef SUPPORT_VULKAN
namespace vk {
DLL_OBJECT ShaderManagerVk* ShaderManager = nullptr;
}
#endif

#ifdef SUPPORT_D3D12
namespace d3d12 {
DLL_OBJECT ShaderManagerD3D12* ShaderManager = nullptr;
}
#endif


#ifdef SUPPORT_WEBGPU
namespace webgpu {
DLL_OBJECT ShaderManagerWgpu* ShaderManager = nullptr;
}
#endif

DLL_OBJECT MouseInterface* Mouse = nullptr;
DLL_OBJECT KeyboardInterface* Keyboard = nullptr;
DLL_OBJECT GamepadInterface* Gamepad = nullptr;

#ifdef WIN32
// Don't upscale window content on Windows with High-DPI settings
void setDPIAware(HMODULE user32Module, HMODULE shcoreModule) {
    bool minWin10Ver1703 = IsWindows10OrGreater();// IsWindowsVersionOrGreater(HIBYTE(0x0A00), LOBYTE(0x0A00), 0);
    bool minWin81 = IsWindows8Point1OrGreater(); // == IsWindowsVersionOrGreater(HIBYTE(0x0603), LOBYTE(0x0603), 0);
    if (minWin10Ver1703) {
        // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setprocessdpiawarenesscontext
        // https://learn.microsoft.com/en-us/windows/win32/hidpi/dpi-awareness-context
        typedef BOOL (__stdcall *SetProcessDpiAwarenessContext_Function)(DPI_AWARENESS_CONTEXT value);
        auto setProcessDpiAwarenessContext = reinterpret_cast<SetProcessDpiAwarenessContext_Function>(GetProcAddress(
                user32Module, "SetProcessDpiAwarenessContext"));
        if (!setProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2)) {
            minWin10Ver1703 = false;
        }
    }
    if (!minWin10Ver1703 && minWin81) {
        // https://learn.microsoft.com/en-us/windows/win32/api/shellscalingapi/nf-shellscalingapi-setprocessdpiawareness
        typedef HRESULT (__stdcall *SetProcessDpiAwareness_Function)(PROCESS_DPI_AWARENESS value);
        auto setProcessDpiAwareness = reinterpret_cast<SetProcessDpiAwareness_Function>(GetProcAddress(
                shcoreModule, "SetProcessDpiAwareness"));
        HRESULT res = setProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
        if (res != S_OK) {
            minWin81 = false;
        }
    }
    if (!minWin10Ver1703 && !minWin81) {
        // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setprocessdpiaware
        typedef BOOL (__stdcall *SetProcessDPIAware_Function)();
        auto setProcessDPIAware = reinterpret_cast<SetProcessDPIAware_Function>(GetProcAddress(
                user32Module, "SetProcessDPIAware"));
        setProcessDPIAware();
    }
}
#endif

AppSettings::AppSettings() {
    Logfile::get()->createLogfile(
            FileUtils::get()->getConfigDirectory() + "Logfile.html", FileUtils::get()->getAppName());

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

    applicationDescription = "An application using the library sgl";
}

void AppSettings::setWindowBackend(WindowBackend _windowBackend) {
    if (mainWindow) {
        sgl::Logfile::get()->throwError("Error in AppSettings::setWindowBackend: Window is already created.");
    }
    windowBackend = _windowBackend;
}

// Load the settings from the configuration file
void AppSettings::loadSettings(const char* filename) {
    std::string settingsFilePathJson = filename;
    if (FileUtils::get()->hasExtension(filename, ".txt")) {
        settingsFilePathJson = FileUtils::get()->removeExtension(filename) + ".json";
    }
    if (FileUtils::get()->fileExists(settingsFilePathJson)) {
        settings.loadFromFileJson(settingsFilePathJson);
    } else {
        settings.loadFromFile(filename);
    }
    if (FileUtils::get()->fileExists(filename)) {
        // Remove legacy .txt file.
        FileUtils::get()->removeFile(filename);
    }
    settingsFilename = settingsFilePathJson;
}

void SettingsFile::saveToFile(const char* filename) {
    writeSimpleJson(filename, settings, 4);
}

void SettingsFile::loadFromFile(const char* filename) {
    // VERY basic and lacking legacy JSON parser
    std::ifstream file(filename);
    if (!file.is_open()) {
        return;
    }
    std::string line;

    while (std::getline(file, line)) {
        sgl::stringTrim(line);
        if (line == "{" || line == "}") {
            continue;
        }

        size_t string1Start = line.find('\"') + 1;
        size_t string1End = line.find('\"', string1Start);
        size_t string2Start = line.find('\"', string1End + 1) + 1;
        size_t string2End = line.find('\"', string2Start);

        std::string key = line.substr(string1Start, string1End - string1Start);
        std::string value = line.substr(string2Start, string2End - string2Start);
        JsonValue jsonValue;
        if (isInteger(value)) {
            jsonValue = fromString<int64_t>(value);
        } else if (isNumeric(value)) {
            jsonValue = fromString<double>(value);
        } else {
            jsonValue = value;
        }
        if (startsWith(key, "window-")) {
            key = key.substr(7);
            if (key == "windowPosition") {
                jsonValue = glmVecToJsonValue(sgl::fromString<glm::ivec2>(value));
            }
            settings["window"][key] = jsonValue;
        } else {
            settings[key] = jsonValue;
        }
    }

    file.close();
}

void SettingsFile::loadFromFileJson(const std::string& filename) {
    settings = readSimpleJson(filename, true);
}

void AppSettings::initializeDataDirectory() {
    // Make sure the "Data" directory exists. If not: Use the "Data" directory in the parent folder if it exists.
    if (!hasCustomDataDirectory && !sgl::FileUtils::get()->exists("Data")
            && sgl::FileUtils::get()->directoryExists("../Data")) {
        dataDirectory = "../Data/";
        hasCustomDataDirectory = true;
    }
    if (!sgl::FileUtils::get()->directoryExists(dataDirectory)) {
        sgl::Logfile::get()->writeError(
                "Error: AppSettings::initializeDataDirectory: Data directory \""
                + dataDirectory + "\" does not exist.");
        exit(1);
    }
}

void AppSettings::setApplicationDescription(const std::string& description) {
    applicationDescription = description;
}

void AppSettings::loadApplicationIconFromFile(const std::string& _iconPath) {
    iconPath = _iconPath;
    if (sgl::AppSettings::get()->getOS() == sgl::OperatingSystem::LINUX) {
        std::string iconPathAbsolute = std::filesystem::canonical(iconPath).string();
        std::string appNameLower = sgl::toLowerCopy(sgl::FileUtils::get()->getAppName());
        std::ofstream desktopFile(
                sgl::FileUtils::get()->getUserDirectory() + ".local/share/applications/" + appNameLower + ".desktop");
        desktopFile << "[Desktop Entry]\n";
        desktopFile << "Name=" << sgl::FileUtils::get()->getAppName() << "\n";
        desktopFile << "Icon=" << iconPathAbsolute << "\n";
        desktopFile << "Comment=" << applicationDescription << "\n";
        desktopFile << "Exec=\"" << sgl::FileUtils::get()->getExecutablePath() << "\" %u\n";
        desktopFile << "Path=" << sgl::FileUtils::get()->getExecutableDirectory() << "\n";
        desktopFile << "Version=1.0\n";
        desktopFile << "Type=Application\n";
        desktopFile << "Terminal=false\n";
        desktopFile << "StartupNotify=true\n";
    }
}

Window* AppSettings::createWindow() {
#ifdef USE_BOOST_LOCALE
    boost::locale::generator gen;
    std::locale l = gen("");
    std::locale::global(l);
#endif

#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN) {
        std::string forceVkDownloadSwapchainVar = getEnvVarString("FORCE_VK_DLSWAP");
        if (!forceVkDownloadSwapchainVar.empty()) {
            settings.getSettingsObject()["window"]["useDownloadSwapchain"] = forceVkDownloadSwapchainVar;
        }
    }
#endif

    initializeDataDirectory();

    // Disable upscaling on Windows with High-DPI settings
#if defined(_WIN32) && (defined(SUPPORT_SDL) || defined(SUPPORT_GLFW) || !defined(DISABLE_IMGUI))
    user32Module = LoadLibrary("User32.dll");
    shcoreModule = LoadLibrary("Shcore.dll");
    setDPIAware(user32Module, shcoreModule);
    setWindowsLibraryHandles(user32Module);
#endif

    // There may only be one instance of a window for now!
    static int windowIdx = 0;
    windowIdx++;
    if (windowIdx != 1) {
        Logfile::get()->writeError("Error in AppSettings::createWindow: More than one instance of a window created!");
        return nullptr;
    }

#if defined(SUPPORT_OPENGL) && !defined(DISABLE_DEVICE_SELECTION_SUPPORT)
    if (renderSystem == RenderSystem::OPENGL && deviceSelector) {
        deviceSelector->deserializeSettingsGlobal();
#if __linux__ && defined(SUPPORT_SDL3)
        // Unfortunately, this currently only seems to exist for offscreen contexts (cf. SDL_EGL_InitializeOffscreen).
        // Also, SDL_EGL_CreateSurface fails with "unable to create an EGL window surface" when forcing EGL.
        /*if (windowBackend == WindowBackend::SDL3_IMPL) {
            auto* deviceSelectorEgl = static_cast<sgl::DeviceSelectorEGL*>(deviceSelector);
            int selectedEglDeviceIdx = deviceSelectorEgl->getSelectedEglDeviceIdx();
            if (selectedEglDeviceIdx >= 0) {
                std::string deviceIdxString = std::to_string(selectedEglDeviceIdx);
                SDL_SetHint(SDL_HINT_VIDEO_FORCE_EGL, "1");
                SDL_SetHint(SDL_HINT_VIDEO_EGL_ALLOW_GETDISPLAY_FALLBACK, "1");
                SDL_SetHint("SDL_HINT_EGL_DEVICE", deviceIdxString.c_str());
            }
        }*/
#endif
    }
#endif

#ifdef SUPPORT_SDL2
    if (windowBackend == WindowBackend::SDL2_IMPL) {
#ifndef __EMSCRIPTEN__
        // Don't initialize SDL_INIT_AUDIO, as we otherwise get "dsp: No such audio device" when building with vcpkg
        // and without the prerequisites for audio installed on the Linux system used for compilation.
        if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC \
                | SDL_INIT_GAMECONTROLLER | SDL_INIT_SENSOR) == -1) {
#else
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) == -1) {
#endif
            Logfile::get()->writeError("ERROR: AppSettings::createWindow: Couldn't initialize SDL2!");
            Logfile::get()->writeError(std::string() + "SDL Error: " + SDL_GetError());
        }
        SDLWindow::errorCheckSDL();
    }
#endif
#ifdef SUPPORT_SDL3
    if (windowBackend == WindowBackend::SDL3_IMPL) {
#ifndef __EMSCRIPTEN__
        // Don't initialize SDL_INIT_AUDIO, as we otherwise get "dsp: No such audio device" when building with vcpkg
        // and without the prerequisites for audio installed on the Linux system used for compilation.
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMEPAD | SDL_INIT_SENSOR)) {
#else
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
#endif
            Logfile::get()->writeError("Error in AppSettings::createWindow: Couldn't initialize SDL3!");
            Logfile::get()->writeError(std::string() + "SDL Error: " + SDL_GetError());
        }
        SDLWindow::errorCheckSDL();
    }
#endif

#ifdef SUPPORT_GLFW
    if (windowBackend == WindowBackend::GLFW_IMPL) {
        if (!glfwInit()) {
            const char* description = nullptr;
            int errorCode = glfwGetError(&description);
            if (errorCode != GLFW_NO_ERROR && description) {
                sgl::Logfile::get()->writeError(
                        "Error in AppSettings::createWindow: glfwInit failed (code 0x"
                        + sgl::toHexString(errorCode) + "): " + description);
            } else if (errorCode != GLFW_NO_ERROR) {
                sgl::Logfile::get()->writeError(
                        "Error in AppSettings::createWindow: glfwInit failed (code 0x"
                        + sgl::toHexString(errorCode) + ")");
            } else {
                sgl::Logfile::get()->writeError("Error in AppSettings::createWindow: glfwInit failed.");
            }
        }
        glfwSetErrorCallback([](int errorCode, const char* description) {
            sgl::Logfile::get()->writeError(
                    "GLFW errror callback (code 0x" + sgl::toHexString(errorCode) + "): " + description);
        });
        const char* glfwVersionString = glfwGetVersionString();
        if (glfwVersionString) {
            sgl::Logfile::get()->write("GLFW version: " + std::string(glfwVersionString), sgl::BLUE);
        }
    }
#endif

#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN) {
        instance = new vk::Instance;
        if (isDebugPrintfEnabled) {
            instance->setIsDebugPrintfEnabled(isDebugPrintfEnabled);
        }

#ifdef SUPPORT_OPENGL
        if (shallEnableVulkanOffscreenOpenGLContextInteropSupport) {
            instanceSupportsVulkanOpenGLInterop = true;
            requiredInstanceExtensionNames.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
            requiredInstanceExtensionNames.push_back(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
            if (!instance->getInstanceExtensionsAvailable(requiredInstanceExtensionNames)) {
                sgl::Logfile::get()->writeWarning(
                        "Warning in AppSettings::createWindow: The Vulkan instance extensions "
                        "VK_KHR_external_memory_capabilities or VK_KHR_external_semaphore_capabilities are not supported "
                        "on this system. Disabling OpenGL interoperability support.", false);
                instanceSupportsVulkanOpenGLInterop = false;
                requiredInstanceExtensionNames = defaultInstanceExtensionNames;
            }
        }
#endif

#if defined(__APPLE__) && defined(SUPPORT_SDL)
        if (getIsSdlWindowBackend(windowBackend) && sdlVulkanLibraryLoaded) {
            sdlVulkanLibraryLoaded = false;
            const char* const moduleNames[] = { "libvulkan.dylib", "libvulkan.1.dylib", "libMoltenVK.dylib" };
            for (int i = 0; i < IM_ARRAYSIZE(moduleNames); i++) {
                void* module = dlopen(moduleNames[i], RTLD_NOW | RTLD_LOCAL);
                if (module) {
                    SDL_Vulkan_LoadLibrary(moduleNames[i]);
                    sdlVulkanLibraryLoaded = true;
                    dlclose(module);
                    break;
                }
            }
        }
#endif
    }
#endif

#ifdef SUPPORT_WEBGPU
    if (renderSystem == RenderSystem::WEBGPU) {
        webgpuInstance = new webgpu::Instance;
    }
#endif

#ifdef SUPPORT_SDL
    if (getIsSdlWindowBackend(windowBackend)) {
        mainWindow = new SDLWindow;
    }
#endif
#ifdef SUPPORT_GLFW
    if (windowBackend == WindowBackend::GLFW_IMPL) {
        mainWindow = new GlfwWindow;
    }
#endif

    WindowSettings windowSettings = mainWindow->deserializeSettings(settings);
    mainWindow->initialize(windowSettings, renderSystem);
    if (!iconPath.empty()) {
        mainWindow->setWindowIconFromFile(iconPath);
    }

#if defined(SUPPORT_OPENGL) && !defined(DISABLE_DEVICE_SELECTION_SUPPORT)
    if (renderSystem == RenderSystem::OPENGL && deviceSelector) {
#if __linux__ && defined(SUPPORT_SDL3)
        if (windowBackend == WindowBackend::SDL3_IMPL) {
            auto* deviceSelectorEgl = static_cast<sgl::DeviceSelectorEGL*>(deviceSelector);
            deviceSelectorEgl->retrieveUsedDevice();
        }
#endif
    }
#endif

    return mainWindow;
}

HeadlessData AppSettings::createHeadless() {
    initializeDataDirectory();

    HeadlessData headlessData;
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN) {
        bool debugContext = false;
        getJsonOptional(settings.getSettingsObject()["window"], "debugContext", debugContext);
        instance = new vk::Instance;
        if (isDebugPrintfEnabled) {
            instance->setIsDebugPrintfEnabled(isDebugPrintfEnabled);
        }
        instance->createInstance(requiredInstanceExtensionNames, debugContext);
        headlessData.instance = instance;
    } else {
#endif
        headlessData.mainWindow = createWindow();
#ifdef SUPPORT_VULKAN
    }
#endif
    return headlessData;
}

#ifdef SUPPORT_VULKAN
void AppSettings::setVulkanDebugPrintfEnabled() {
    isDebugPrintfEnabled = true;
}

void AppSettings::setRequiredVulkanInstanceExtensions(const std::vector<const char*>& extensions) {
    defaultInstanceExtensionNames = extensions;
    requiredInstanceExtensionNames = extensions;
}
#endif

#if defined(SUPPORT_OPENGL) && defined(SUPPORT_VULKAN)
void checkGlExtension(const char* extensionName, VulkanInteropCapabilities& vulkanInteropCapabilities) {
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
        sgl::Logfile::get()->writeWarning(
                "Warning in AppSettings::initializeVulkanInteropSupport: Only Windows and Linux-based systems "
                "support sharing memory objects and semaphores between OpenGL and Vulkan applications.",
                false);
        vulkanInteropCapabilities = VulkanInteropCapabilities::NO_INTEROP;
    }

    instance = new vk::Instance;
    if (isDebugPrintfEnabled) {
        instance->setIsDebugPrintfEnabled(isDebugPrintfEnabled);
    }

    if (!sgl::SystemGL::get()->isGLExtensionAvailable("GL_EXT_memory_object")) {
        sgl::Logfile::get()->writeWarning(
                "Warning in AppSettings::initializeVulkanInteropSupport: The OpenGL extension GL_EXT_memory_object "
                "is not supported on this system. Disabling external memory support.", false);
        vulkanInteropCapabilities = VulkanInteropCapabilities::NO_INTEROP;
    }
    if (!sgl::SystemGL::get()->isGLExtensionAvailable("GL_EXT_semaphore")) {
        sgl::Logfile::get()->writeWarning(
                "Warning in AppSettings::initializeVulkanInteropSupport: The OpenGL extension GL_EXT_semaphore "
                "is not supported on this system. Disabling external memory support.", false);
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
    for (const char* extensionName : openglExtensionNames) {
        if (vulkanInteropCapabilities == VulkanInteropCapabilities::NO_INTEROP) {
            break;
        }
        checkGlExtension(extensionName, vulkanInteropCapabilities);
    }

    std::vector<const char*> instanceExtensionNames = requiredInstanceExtensionNames;
    if (vulkanInteropCapabilities == VulkanInteropCapabilities::EXTERNAL_MEMORY) {
        instanceExtensionNames.push_back(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
        instanceExtensionNames.push_back(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
        if (!instance->getInstanceExtensionsAvailable(instanceExtensionNames)) {
            sgl::Logfile::get()->writeWarning(
                    "Warning in AppSettings::initializeVulkanInteropSupport: The Vulkan instance extensions "
                    "VK_KHR_external_memory_capabilities or VK_KHR_external_semaphore_capabilities are not supported "
                    "on this system. Disabling external memory support.", false);
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
            instance, requiredDeviceExtensionNames,
            optionalDeviceExtensionNamesAll, requestedDeviceFeatures);
    if (primaryDevice->getVkPhysicalDevice() == VK_NULL_HANDLE) {
        sgl::Logfile::get()->writeWarning(
                "Warning in AppSettings::initializeVulkanInteropSupport: Disabling Vulkan interoperability "
                "support, as no suitable physical device was found.", false);
        delete primaryDevice;
        primaryDevice = nullptr;
        vulkanInteropCapabilities = VulkanInteropCapabilities::NO_INTEROP;
        return;
    }

    for (const char* deviceExtension : externalMemoryDeviceExtensionNames) {
        if (!primaryDevice->isDeviceExtensionSupported(deviceExtension)) {
            // "Please update your GPU drivers if you are using the old version of the INTEL iGPU i965 Linux drivers."
            sgl::Logfile::get()->writeWarning(
                    std::string() + "Warning in AppSettings::initializeVulkanInteropSupport: The Vulkan device "
                    "extension " + deviceExtension + " is not supported on this system. Disabling external memory "
                    "support. Please try updating your GPU drivers.", false);
            delete primaryDevice;
            primaryDevice = nullptr;
            vulkanInteropCapabilities = VulkanInteropCapabilities::NO_INTEROP;
            break;
        }
    }
}

void AppSettings::enableVulkanOffscreenOpenGLContextInteropSupport() {
    shallEnableVulkanOffscreenOpenGLContextInteropSupport = true;
}

std::vector<const char*> AppSettings::getVulkanOpenGLInteropDeviceExtensions() {
    std::vector<const char*> externalMemoryDeviceExtensionNames;
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
    return externalMemoryDeviceExtensionNames;
}

bool AppSettings::checkVulkanOpenGLInteropDeviceExtensionsSupported(sgl::vk::Device* device) {
    std::vector<const char*> externalMemoryDeviceExtensionNames = getVulkanOpenGLInteropDeviceExtensions();
    for (const char* deviceExtension : externalMemoryDeviceExtensionNames) {
        if (!device->isDeviceExtensionSupported(deviceExtension)) {
            // "Please update your GPU drivers if you are using the old version of the INTEL iGPU i965 Linux drivers."
            sgl::Logfile::get()->writeWarning(
                    std::string() + "Warning in AppSettings::checkVulkanOpenGLInteropDeviceExtensionsSupported: "
                    "The Vulkan device extension " + deviceExtension + " is not supported on this system. "
                    "Disabling OpenGL interop support. Please try updating your GPU drivers.", false);
            return false;
        }
    }
    return true;
}

std::vector<const char*> AppSettings::getOpenGLVulkanInteropExtensions() {
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
    return openglExtensionNames;
}

bool AppSettings::checkOpenGLVulkanInteropExtensionsSupported() {
    std::vector<const char*> openglExtensionNames = getOpenGLVulkanInteropExtensions();
    for (const char* extensionName : openglExtensionNames) {
        if (!sgl::SystemGL::get()->isGLExtensionAvailable(extensionName)) {
            sgl::Logfile::get()->writeInfo(
                    std::string() + "Warning in AppSettings::checkOpenGLVulkanInteropExtensionsSupported: "
                    "The OpenGL extension " + extensionName + " is not supported on this system. "
                    "Disabling OpenGL interop support.");
            return false;
        }
    }
    return true;
}
#endif

#ifdef SUPPORT_OPENGL
void AppSettings::setUseAppDeviceSelectorOpenGL(
#ifdef _WIN32
        DWORD* _NvOptimusEnablement, DWORD* _AmdPowerXpressRequestHighPerformance
#endif
    ) {
#if defined(__linux__) && !defined(DISABLE_DEVICE_SELECTION_SUPPORT)
        // SDL2 has SDL_HINT_VIDEO_X11_FORCE_EGL, but no way of setting the used device.
#ifdef SUPPORT_SDL3
    if (!deviceSelector && windowBackend == WindowBackend::SDL3_IMPL) {
        deviceSelector = new sgl::DeviceSelectorEGL();
    }
#endif
#endif

#if defined(_WIN32) && !defined(DISABLE_DEVICE_SELECTION_SUPPORT)
    if (!deviceSelector) {
        deviceSelector = new sgl::DeviceSelectorWGL(_NvOptimusEnablement, _AmdPowerXpressRequestHighPerformance);
    }
#endif
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

#ifdef SUPPORT_OPENGL
void AppSettings::setOffscreenContext(sgl::OffscreenContext* _offscreenContext) {
    offscreenContext = _offscreenContext;
}

void AppSettings::initializeOffscreenContextFunctionPointers() {
    if (offscreenContextFunctionPointersInitialized) {
        return;
    }
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
#ifndef __EMSCRIPTEN__
    if (glewError == GLEW_ERROR_NO_GLX_DISPLAY) {
        Logfile::get()->writeWarning("Warning: GLEW is not built with EGL support.");
    } else
#endif
    if (glewError != GLEW_OK) {
        Logfile::get()->writeError(
                std::string() + "Error in AppSettings::initializeOffscreenContextFunctionPointers: glewInit: "
                + (char*)glewGetErrorString(glewError));
    }
    offscreenContextFunctionPointersInitialized = true;
}
#endif

void AppSettings::initializeSubsystems() {
    /*if (TTF_Init() == -1) {
        Logfile::get()->writeError("ERROR: SDLWindow::initializeAudio: Couldn't initialize SDL_ttf!");
        Logfile::get()->writeError(std::string() + "SDL_ttf initialization error: " + TTF_GetError());
    }*/

    // Create the subsystem implementations
    Timer = new TimerInterface;
    //AudioManager = new SDLMixerAudioManager;

#ifdef SUPPORT_OPENGL
    if (offscreenContext) {
        // Initialize GLEW (usually done by the window).
        initializeOffscreenContextFunctionPointers();
    }
    if (renderSystem == RenderSystem::OPENGL || offscreenContext) {
        TextureManager = new TextureManagerGL;
        ShaderManager = new ShaderManagerGL;
        Renderer = new RendererGL;
        MaterialManager = new MaterialManagerInterface;
        SystemGL::get();
    }
#endif
#ifdef SUPPORT_VULKAN
#ifndef DISABLE_SCENE_GRAPH_SOURCES
    if (renderSystem == RenderSystem::VULKAN) {
        Camera::depthRange = Camera::DepthRange::ZERO_ONE;
        Camera::coordinateOrigin = Camera::CoordinateOrigin::TOP_LEFT;
    }
#endif
    if (primaryDevice) {
        vk::ShaderManager = new vk::ShaderManagerVk(primaryDevice);
    }
#ifdef SUPPORT_D3D12
    if (renderSystem == RenderSystem::DIRECT3D_12) {
        d3d12::ShaderManager = new d3d12::ShaderManagerD3D12();
    }
#endif
#ifndef DISABLE_DEVICE_SELECTION_SUPPORT
    if (renderSystem == RenderSystem::VULKAN && primaryDevice) {
        deviceSelector = primaryDevice->getDeviceSelector();
    }
#endif
#endif

#ifdef SUPPORT_WEBGPU
#ifndef DISABLE_SCENE_GRAPH_SOURCES
    if (renderSystem == RenderSystem::WEBGPU) {
        Camera::depthRange = Camera::DepthRange::ZERO_ONE;
        Camera::coordinateOrigin = Camera::CoordinateOrigin::TOP_LEFT;
    }
#endif
    if (webgpuPrimaryDevice) {
        webgpu::ShaderManager = new webgpu::ShaderManagerWgpu(webgpuPrimaryDevice);
    }
#endif

#ifdef SUPPORT_SDL
    if (getIsSdlWindowBackend(windowBackend)) {
        Mouse = new SDLMouse;
        Keyboard = new SDLKeyboard;
        Gamepad = new SDLGamepad;
    }
#endif
#ifdef SUPPORT_GLFW
    if (windowBackend == WindowBackend::GLFW_IMPL) {
        Mouse = new GlfwMouse;
        Keyboard = new GlfwKeyboard;
        Gamepad = new GlfwGamepad;
    }
#endif

#ifndef DISABLE_IMGUI
    if (useGUI) {
        ImGuiWrapper::get()->initialize(fontRangesData, useDocking, useMultiViewport, uiScaleFactor);
    }
#endif
}

void AppSettings::release() {
#ifdef SUPPORT_VULKAN
    if (primaryDevice) {
        primaryDevice->waitIdle();
    }
#endif

    if (mainWindow) {
        mainWindow->serializeSettings(settings);
    }

#ifndef DISABLE_IMGUI
    if (useGUI) {
        ImGuiWrapper::get()->shutdown();
    }
#endif

#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL || offscreenContext) {
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
#ifndef DISABLE_DEVICE_SELECTION_SUPPORT
    if (renderSystem == RenderSystem::OPENGL && deviceSelector) {
        deviceSelector->serializeSettingsGlobal();
        delete deviceSelector;
        deviceSelector = nullptr;
    }
#endif
#endif

#ifdef SUPPORT_VULKAN
    if (primaryDevice) {
        if (vk::ShaderManager) {
            delete vk::ShaderManager;
            vk::ShaderManager = nullptr;
        }
    }
#endif

#ifdef SUPPORT_WEBGPU
    if (webgpuPrimaryDevice) {
        webgpuInstance->onPreDeviceDestroy();
        if (webgpu::ShaderManager) {
            delete webgpu::ShaderManager;
            webgpu::ShaderManager = nullptr;
        }
    }
#endif

    //delete AudioManager;
    delete Timer;

#if defined(SUPPORT_VULKAN) && !defined(DISABLE_VULKAN_SWAPCHAIN_SUPPORT)
    if (renderSystem == RenderSystem::VULKAN) {
        if (swapchain) {
            delete swapchain;
            swapchain = nullptr;
        }
    }
#endif

#ifdef SUPPORT_WEBGPU
    if (renderSystem == RenderSystem::WEBGPU) {
        if (webgpuSwapchain) {
            delete webgpuSwapchain;
            webgpuSwapchain = nullptr;
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
#ifdef SUPPORT_SDL
        if (getIsSdlWindowBackend(windowBackend) && sdlVulkanLibraryLoaded) {
            SDL_Vulkan_UnloadLibrary();
        }
#endif
        delete instance;
        instance = nullptr;
    }
#endif

#ifdef SUPPORT_WEBGPU
    if (webgpuPrimaryDevice) {
        delete webgpuPrimaryDevice;
        webgpuPrimaryDevice = nullptr;
    }
    if (webgpuInstance) {
        delete webgpuInstance;
        webgpuInstance = nullptr;
    }
#endif

    if (saveSettings) {
        settings.saveToFile(settingsFilename.c_str());
    }

#ifdef SUPPORT_SDL
    if (getIsSdlWindowBackend(windowBackend)) {
        //Mix_CloseAudio();
        //TTF_Quit();
        SDL_Quit();
    }
#endif

#ifdef _WIN32
    FreeLibrary(shcoreModule);
    FreeLibrary(user32Module);
#endif
}

void AppSettings::setLoadGUI(
        const unsigned short* fontRangeData, bool useDocking, bool useMultiViewport, float uiScaleFactor) {
    useGUI = true;
    this->fontRangesData = fontRangeData;
    this->useDocking = useDocking;
    this->useMultiViewport = useMultiViewport;
    this->uiScaleFactor = uiScaleFactor;
}

Window* AppSettings::getMainWindow() {
    return mainWindow;
}

Window* AppSettings::setMainWindow(Window* window) {
    return mainWindow;
}

int AppSettings::getNumDisplays() {
#ifdef SUPPORT_SDL2
    if (mainWindow->getBackend() == WindowBackend::SDL2_IMPL) {
        return SDL_GetNumVideoDisplays();
    }
#endif
#ifdef SUPPORT_SDL3
    if (mainWindow->getBackend() == WindowBackend::SDL3_IMPL) {
        int numDisplays = 0;
        auto* displays = SDL_GetDisplays(&numDisplays);
        if (!displays) {
            SDLWindow::errorCheckSDL();
            return 0;
        }
        SDL_free(displays);
        return numDisplays;
    }
#endif
#ifdef SUPPORT_GLFW
    if (mainWindow->getBackend() == WindowBackend::GLFW_IMPL) {
        int numMonitors = 0;
        glfwGetMonitors(&numMonitors);
        return numMonitors;
    }
#endif
    return 1;
}

void AppSettings::getCurrentDisplayMode(int& width, int& height, int& refreshRate, int displayIndex) {
#ifdef SUPPORT_SDL2
    if (mainWindow->getBackend() == WindowBackend::SDL2_IMPL) {
        SDL_DisplayMode displayMode;
        SDL_GetCurrentDisplayMode(displayIndex, &displayMode);
        width = displayMode.w;
        height = displayMode.h;
        refreshRate = displayMode.refresh_rate;
        return;
    }
#endif
#ifdef SUPPORT_SDL3
    if (mainWindow->getBackend() == WindowBackend::SDL3_IMPL) {
        int numDisplays = 0;
        auto* displays = SDL_GetDisplays(&numDisplays);
        if (!displays) {
            SDLWindow::errorCheckSDL();
            return;
        }
        if (displayIndex >= numDisplays) {
            sgl::Logfile::get()->writeError(
                    "Error in AppSettings::getDesktopDisplayMode: Display index " + std::to_string(displayIndex)
                    + " is out of bounds.");
            SDL_free(displays);
            return;
        }
        auto* displayMode = SDL_GetCurrentDisplayMode(displays[displayIndex]);
        SDL_free(displays);
        if (!displayMode) {
            SDLWindow::errorCheckSDL();
            return;
        }
        width = displayMode->w;
        height = displayMode->h;
        refreshRate = int(std::round(displayMode->refresh_rate));
        return;
    }
#endif
#ifdef SUPPORT_GLFW
    if (mainWindow->getBackend() == WindowBackend::GLFW_IMPL) {
        int numMonitors = 0;
        GLFWmonitor** monitors = glfwGetMonitors(&numMonitors);
        if (displayIndex >= numMonitors) {
            sgl::Logfile::get()->writeError("Error in AppSettings::getCurrentDisplayMode: Invalid display index.");
            return;
        }
        GLFWmonitor* monitor = monitors[displayIndex];
        const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
        width = videoMode->width;
        height = videoMode->height;
        return;
    }
#endif
    width = 800;
    height = 600;
    refreshRate = 600;
    displayIndex = 0;
}

void AppSettings::getDesktopDisplayMode(int& width, int& height, int& refreshRate, int displayIndex) {
#ifdef SUPPORT_SDL2
    if (mainWindow->getBackend() == WindowBackend::SDL2_IMPL) {
        SDL_DisplayMode displayMode;
        SDL_GetDesktopDisplayMode(displayIndex, &displayMode);
        width = displayMode.w;
        height = displayMode.h;
        refreshRate = displayMode.refresh_rate;
        return;
    }
#endif
#ifdef SUPPORT_SDL3
    if (mainWindow->getBackend() == WindowBackend::SDL3_IMPL) {
        int numDisplays = 0;
        auto* displays = SDL_GetDisplays(&numDisplays);
        if (!displays) {
            SDLWindow::errorCheckSDL();
            return;
        }
        if (displayIndex >= numDisplays) {
            sgl::Logfile::get()->writeError(
                    "Error in AppSettings::getDesktopDisplayMode: Display index " + std::to_string(displayIndex)
                    + " is out of bounds.");
            SDL_free(displays);
            return;
        }
        auto* displayMode = SDL_GetDesktopDisplayMode(displays[displayIndex]);
        SDL_free(displays);
        if (!displayMode) {
            SDLWindow::errorCheckSDL();
            return;
        }
        width = displayMode->w;
        height = displayMode->h;
        refreshRate = int(std::round(displayMode->refresh_rate));
        return;
    }
#endif
#ifdef SUPPORT_GLFW
    if (mainWindow->getBackend() == WindowBackend::GLFW_IMPL) {
        int numMonitors = 0;
        GLFWmonitor** monitors = glfwGetMonitors(&numMonitors);
        if (displayIndex >= numMonitors) {
            sgl::Logfile::get()->writeError("Error in AppSettings::getCurrentDisplayMode: Invalid display index.");
            return;
        }
        GLFWmonitor* monitor = monitors[displayIndex];
        const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
        width = videoMode->width;
        height = videoMode->height;
        refreshRate = videoMode->refreshRate;
        return;
    }
#endif
    width = 800;
    height = 600;
    refreshRate = 600;
    displayIndex = 0;
}

glm::ivec2 AppSettings::getCurrentDisplayModeResolution(int displayIndex) {
#ifdef SUPPORT_SDL2
    if (mainWindow->getBackend() == WindowBackend::SDL2_IMPL) {
        SDL_DisplayMode displayMode;
        SDL_GetCurrentDisplayMode(displayIndex, &displayMode);
        return { displayMode.w, displayMode.h };
    }
#endif
#ifdef SUPPORT_SDL3
    if (mainWindow->getBackend() == WindowBackend::SDL3_IMPL) {
        int numDisplays = 0;
        auto* displays = SDL_GetDisplays(&numDisplays);
        if (!displays) {
            SDLWindow::errorCheckSDL();
            return { 0, 0 };
        }
        if (displayIndex >= numDisplays) {
            sgl::Logfile::get()->writeError(
                    "Error in AppSettings::getDesktopDisplayMode: Display index " + std::to_string(displayIndex)
                    + " is out of bounds.");
            SDL_free(displays);
            return { 0, 0 };
        }
        auto* displayMode = SDL_GetCurrentDisplayMode(displays[displayIndex]);
        SDL_free(displays);
        if (!displayMode) {
            SDLWindow::errorCheckSDL();
            return { 0, 0 };
        }
        return { displayMode->w, displayMode->h };
    }
#endif
#ifdef SUPPORT_GLFW
    if (mainWindow->getBackend() == WindowBackend::GLFW_IMPL) {
        int numMonitors = 0;
        GLFWmonitor** monitors = glfwGetMonitors(&numMonitors);
        if (displayIndex >= numMonitors) {
            sgl::Logfile::get()->writeError("Error in AppSettings::getCurrentDisplayMode: Invalid display index.");
            return { 0, 0 };
        }
        GLFWmonitor* monitor = monitors[displayIndex];
        const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
        return { videoMode->width, videoMode->height };
    }
#endif
    return { 800, 600 };
}

glm::ivec2 AppSettings::getDesktopResolution(int displayIndex) {
#ifdef SUPPORT_SDL2
    if (mainWindow->getBackend() == WindowBackend::SDL2_IMPL) {
        SDL_DisplayMode displayMode;
        SDL_GetDesktopDisplayMode(displayIndex, &displayMode);
        return { displayMode.w, displayMode.h };
    }
#endif
#ifdef SUPPORT_SDL3
    if (mainWindow->getBackend() == WindowBackend::SDL3_IMPL) {
        int numDisplays = 0;
        auto* displays = SDL_GetDisplays(&numDisplays);
        if (!displays) {
            SDLWindow::errorCheckSDL();
            return { 0, 0 };
        }
        if (displayIndex >= numDisplays) {
            sgl::Logfile::get()->writeError(
                    "Error in AppSettings::getDesktopDisplayMode: Display index " + std::to_string(displayIndex)
                    + " is out of bounds.");
            SDL_free(displays);
            return { 0, 0 };
        }
        auto* displayMode = SDL_GetDesktopDisplayMode(displays[displayIndex]);
        SDL_free(displays);
        if (!displayMode) {
            SDLWindow::errorCheckSDL();
            return { 0, 0 };
        }
        return { displayMode->w, displayMode->h };
    }
#endif
#ifdef SUPPORT_GLFW
    if (mainWindow->getBackend() == WindowBackend::GLFW_IMPL) {
        // Technically, there would be glfwGetVideoModes, but it is hard to say which mode is the desktop mode...
        return getCurrentDisplayModeResolution(displayIndex);
    }
#endif
    return { 800, 600 };
}

void AppSettings::captureMouse(bool _capture) {
    if (!mainWindow) {
        return;
    }
    if (mainWindow && mainWindow->getUsesWaylandBackend()) {
        // Wayland does not support capturing the mouse.
        // SDL_CaptureMouse causes "that operation is not supported" SDL error.
        return;
    }
#ifdef SUPPORT_SDL2
    if (mainWindow->getBackend() == WindowBackend::SDL2_IMPL) {
        SDL_CaptureMouse(_capture ? SDL_TRUE : SDL_FALSE);
    }
#endif
#ifdef SUPPORT_SDL3
    if (mainWindow->getBackend() == WindowBackend::SDL3_IMPL) {
        SDL_CaptureMouse(_capture);
    }
#endif
#ifdef SUPPORT_GLFW
    if (mainWindow->getBackend() == WindowBackend::GLFW_IMPL) {
        static_cast<GlfwWindow*>(mainWindow)->setCaptureMouse(_capture);
    }
#endif
}

}
