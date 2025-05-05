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

#ifndef SYSTEM_APPSETTINGS_HPP_
#define SYSTEM_APPSETTINGS_HPP_

#include <map>
#include <string>
#ifdef USE_GLM
#include <glm/vec2.hpp>
#endif

#include <Defs.hpp>
#include <Utils/Singleton.hpp>
#include <Utils/Convert.hpp>
#include <Graphics/Utils/RenderSystem.hpp>

#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/Utils/Device.hpp>
#endif

namespace sgl {

class Window;
class OffscreenContext;

#ifdef SUPPORT_VULKAN
namespace vk { class Instance; class Device; class Swapchain; }
#endif

#ifdef SUPPORT_WEBGPU
namespace webgpu { class Instance; class Device; class Swapchain; }
#endif

class DLL_OBJECT SettingsFile {
public:
    inline std::string getValue(const char *key) const { auto it = settings.find(key); return it == settings.end() ? "" : it->second; }
    inline int getIntValue(const char *key) const { return fromString<int>(getValue(key)); }
    inline float getFloatValue(const char *key) const { return fromString<float>(getValue(key)); }
    inline bool getBoolValue(const char *key) const { std::string val = getValue(key); if (val == "false" || val == "0") return false; return val.length() > 0; }
    inline void addKeyValue(const std::string &key, const std::string &value) { settings[key] = value; }
    template<class T> inline void addKeyValue(const std::string &key, const T &value) { settings[key] = toString(value); }
    [[nodiscard]] inline bool hasKey(const std::string &key) const { return settings.find(key) != settings.end(); }
    inline void removeKey(const std::string &key) { settings.erase(key); }
    inline void clear() { settings.clear(); }

    bool getValueOpt(const char *key, std::string &toset) const {
        auto it = settings.find(key);
        if (it != settings.end()) {
            toset = it->second;
            return true;
        }
        return false;
    }
    template<class T> bool getValueOpt(const char *key, T &toset) const {
        auto it = settings.find(key);
        if (it != settings.end()) {
            toset = fromString<T>(it->second);
            return true;
        }
        return false;
    }
    bool getValueOpt(const char *key, bool &toset) const {
        auto it = settings.find(key);
        if (it != settings.end()) {
            std::string val = getValue(key);
            toset = val != "false" && val != "0";
            return true;
        }
        return false;
    }

    void saveToFile(const char *filename);
    void loadFromFile(const char *filename);

private:
    std::map<std::string, std::string> settings;
};

/// State of OpenGL-Vulkan interoperability capabilities.
enum class VulkanInteropCapabilities {
    // Vulkan is not loaded, can't be used.
    NOT_LOADED,
    // No OpenGL-Vulkan interoperability. Data exchange must be done via the CPU.
    NO_INTEROP,
    // OpenGL-Vulkan interoperability is supported via external memory sharing.
    EXTERNAL_MEMORY
};

union HeadlessData {
    Window* mainWindow;
#ifdef SUPPORT_VULKAN
    vk::Instance* instance;
#endif
};

/**
 * AppSettings handles application-specific settings on the one hand, and basic settings like the screen resolution or
 * the audio system on the other hand. AppSettings will create the instances of the render system, etc., on startup.
 */
class DLL_OBJECT AppSettings : public Singleton<AppSettings>
{
public:
    AppSettings();
    void setWindowBackend(WindowBackend _windowBackend);
    [[nodiscard]] WindowBackend getWindowBackend() const { return windowBackend; }
    void loadSettings(const char *filename);
    inline void setSaveSettings(bool _saveSettings) { saveSettings = _saveSettings; }
    inline SettingsFile &getSettings() { return settings; }

    /**
     * Sets the description of the functionality of the application.#
     * This is used, e.g., on Linux for adding a desktop entry.
     * @param description The description of the functionality of the application.
     */
    void setApplicationDescription(const std::string& description);

    /**
     * Sets the icon to use, e.g., for the application window.
     * On Linux, this also creates an entry in the directory ~/.local/share/applications.
     * @param iconPath The path (local or global) to the application icon.
     */
    void loadApplicationIconFromFile(const std::string& iconPath);

    /// setRenderSystem must be called before calling initializeSubsystems and createWindow.
    void setRenderSystem(RenderSystem renderSystem);
    void initializeSubsystems();
    Window* createWindow();
    /// Only Vulkan supports headless rendering (without a window) for now.
    HeadlessData createHeadless();
    /// 'initializeDataDirectory' needs to be called when not using 'createWindow' or 'createHeadless'.
    void initializeDataDirectory();
#ifdef SUPPORT_OPENGL
    /// Sets an offscreen OpenGL context for use together with a Vulkan instance, device & swapchain.
    void setOffscreenContext(sgl::OffscreenContext* _offscreenContext);
    /// Returns the offscreen OpenGL context (or nullptr if not set).
    inline sgl::OffscreenContext* getOffscreenContext() { return offscreenContext; }
    /// Initializes the OpenGL function pointers for the offscreen context.
    void initializeOffscreenContextFunctionPointers();
#endif
#ifdef SUPPORT_VULKAN
    /// Returns the Vulkan instance.
    inline vk::Instance* getVulkanInstance() { return instance; }
    /// Set the primary device (used for, e.g., GUI rendering for ImGui).
    inline void setPrimaryDevice(vk::Device* device) { primaryDevice = device; }
    inline vk::Device* getPrimaryDevice() { return primaryDevice; }
    /// Set the used swapchain.
    inline void setSwapchain(vk::Swapchain* swapchain) { this->swapchain = swapchain; }
    inline vk::Swapchain* getSwapchain() { return swapchain; }
    /**
     * Enable Vulkan shader debug printf. In this case, the device extension VK_KHR_shader_non_semantic_info
     * must be used.
     */
    void setVulkanDebugPrintfEnabled();
    // Sets Vulkan instance extensions used by @see createWindow and @see createHeadless.
    void setRequiredVulkanInstanceExtensions(const std::vector<const char*>& extensions);
    [[nodiscard]] inline const std::vector<const char*>& getRequiredVulkanInstanceExtensions() const {
        return requiredInstanceExtensionNames;
    }
    // Using the matrix block at descriptor set 1 should be disabled for headless raster passes.
    [[nodiscard]] inline bool getUseMatrixBlock() { return useMatrixBlock; }
    inline void setUseMatrixBlock(bool _useMatrixBlock) { useMatrixBlock = _useMatrixBlock; }
#if defined(SUPPORT_OPENGL) && defined(SUPPORT_VULKAN)
    /// Initialize instance and device for OpenGL-Vulkan interoperability. Must be called before initializeSubsystems.
    void initializeVulkanInteropSupport(
            const std::vector<const char*>& requiredDeviceExtensionNames = {},
            const std::vector<const char*>& optionalDeviceExtensionNames = {},
            const vk::DeviceFeatures& requestedDeviceFeatures = vk::DeviceFeatures());
    inline VulkanInteropCapabilities getVulkanInteropCapabilities() { return vulkanInteropCapabilities; }
    /**
     * Enables Vulkan to OpenGL offscreen context interop support. Must be called before createWindow.
     * The difference to initializeVulkanInteropSupport is that in this case, a Vulkan swapchain is used for rendering
     * to the main window.
     */
    void enableVulkanOffscreenOpenGLContextInteropSupport();
    [[nodiscard]] inline bool getInstanceSupportsVulkanOpenGLInterop() const {
        return instanceSupportsVulkanOpenGLInterop;
    }
    /// Returns a list of Vulkan extensions necessary for interop with OpenGL.
    std::vector<const char*> getVulkanOpenGLInteropDeviceExtensions();
    /// Checks whether the Vulkan extensions necessary for interop with OpenGL are available.
    bool checkVulkanOpenGLInteropDeviceExtensionsSupported(sgl::vk::Device* device);
    /// Returns a list of OpenGL extensions necessary for interop with Vulkan.
    std::vector<const char*> getOpenGLVulkanInteropExtensions();
    /// Checks whether the OpenGL extensions necessary for interop with Vulkan are available.
    bool checkOpenGLVulkanInteropExtensionsSupported();
#endif
//#if defined(SUPPORT_OPENGL) && defined(SUPPORT_VULKAN)
//    void initializeCudaInteropSupport(
//            const std::vector<const char*>& requiredDeviceExtensionNames = {},
//            const std::vector<const char*>& optionalDeviceExtensionNames = {},
//            const vk::DeviceFeatures& requestedDeviceFeatures = vk::DeviceFeatures());
//    inline VulkanInteropCapabilities getVulkanInteropCapabilities() { return vulkanInteropCapabilities; }
//#endif
#endif
#ifdef SUPPORT_WEBGPU
    /// Returns the Vulkan instance.
    inline webgpu::Instance* getWebGPUInstance() { return webgpuInstance; }
    /// Set the primary device (used for, e.g., GUI rendering for ImGui).
    inline void setWebGPUPrimaryDevice(webgpu::Device* device) { webgpuPrimaryDevice = device; }
    inline webgpu::Device* getWebGPUPrimaryDevice() { return webgpuPrimaryDevice; }
    /// Set the used swapchain.
    inline void setWebGPUSwapchain(webgpu::Swapchain* _webgpuSwapchain) { this->webgpuSwapchain = _webgpuSwapchain; }
    inline webgpu::Swapchain* getWebGPUSwapchain() { return webgpuSwapchain; }
#endif
    void release();

    /// Called in main if GUI should be loaded.
    void setLoadGUI(
            const unsigned short* fontRangeData = nullptr, bool useDocking = true, bool useMultiViewport = true,
            float uiScaleFactor = 1.0f);
    [[nodiscard]] inline bool getUseGUI() const { return useGUI; }

    inline RenderSystem getRenderSystem() { return renderSystem; }
    inline OperatingSystem getOS() { return operatingSystem; }
    Window* getMainWindow();
    Window* setMainWindow(Window* window);

    int getNumDisplays();
    void getCurrentDisplayMode(int& width, int& height, int& refreshRate, int displayIndex = 0);
    void getDesktopDisplayMode(int& width, int& height, int& refreshRate, int displayIndex = 0);
#ifdef USE_GLM
    glm::ivec2 getCurrentDisplayModeResolution(int displayIndex = 0);
    glm::ivec2 getDesktopResolution(int displayIndex = 0);
#endif
    void captureMouse(bool _capture);

    // Get the directory where the application data is stored.
    [[nodiscard]] inline const std::string& getDataDirectory() const { return dataDirectory; }
    [[nodiscard]] inline bool getHasCustomDataDirectory() const { return hasCustomDataDirectory; }
    void setDataDirectory(const std::string& dataDirectory);

private:
    SettingsFile settings;
    std::string settingsFilename;
    bool saveSettings = true; ///< Whether to save the settings to a filename.
    RenderSystem renderSystem = RenderSystem::OPENGL;
    OperatingSystem operatingSystem;
#if defined(SUPPORT_SDL3)
    WindowBackend windowBackend = WindowBackend::SDL3_IMPL;
#elif defined(SUPPORT_SDL2)
    WindowBackend windowBackend = WindowBackend::SDL2_IMPL;
#elif defined(SUPPORT_GLFW)
    WindowBackend windowBackend = WindowBackend::GLFW_IMPL;
#else
    WindowBackend windowBackend = WindowBackend::NONE;
#endif
    Window* mainWindow = nullptr;
    std::string applicationDescription;

#ifdef SUPPORT_OPENGL
    sgl::OffscreenContext* offscreenContext = nullptr;
    bool offscreenContextFunctionPointersInitialized = false;
#endif

#ifdef SUPPORT_VULKAN
    vk::Instance* instance = nullptr;
    vk::Device* primaryDevice = nullptr;
    vk::Swapchain* swapchain = nullptr;
    std::vector<const char*> requiredInstanceExtensionNames;
    std::vector<const char*> defaultInstanceExtensionNames;
    VulkanInteropCapabilities vulkanInteropCapabilities = VulkanInteropCapabilities::NOT_LOADED;
    bool useMatrixBlock = true; //< Use matrix block in descriptor set 1.
#ifdef SUPPORT_SDL
    bool sdlVulkanLibraryLoaded = false;
#endif
    bool isDebugPrintfEnabled = false;
#endif

#if defined(SUPPORT_OPENGL) && defined(SUPPORT_VULKAN)
    bool shallEnableVulkanOffscreenOpenGLContextInteropSupport = false;
    bool instanceSupportsVulkanOpenGLInterop = false;
#endif

#ifdef SUPPORT_WEBGPU
    webgpu::Instance* webgpuInstance = nullptr;
    webgpu::Device* webgpuPrimaryDevice = nullptr;
    webgpu::Swapchain* webgpuSwapchain = nullptr;
#endif

    // Where the application data is stored.
    std::string dataDirectory = "Data/";
    bool hasCustomDataDirectory = false;

    // UI data.
    bool useGUI = false;
    const unsigned short* fontRangesData = nullptr;
    bool useDocking = true;
    bool useMultiViewport = true;
    float uiScaleFactor = 1.0f;
    std::string iconPath;
};

}

/*! SYSTEM_APPSETTINGS_HPP_ */
#endif
