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
#include <glm/vec2.hpp>
#include <Defs.hpp>
#include <Utils/Singleton.hpp>
#include <Utils/Convert.hpp>
#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/Utils/Device.hpp>
#endif

namespace sgl {

class Window;

#ifdef SUPPORT_VULKAN
namespace vk { class Instance; class Device; class Swapchain; }
#endif

// At the moment, only OpenGL and Vulkan are supported.
enum class RenderSystem {
    OPENGL, OPENGLES, VULKAN, DIRECT3D_11, DIRECT3D_12, METAL
};

enum class OperatingSystem {
    WINDOWS, LINUX, ANDROID, MACOS, IOS, UNKNOWN
};

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
    void loadSettings(const char *filename);
    inline void setSaveSettings(bool _saveSettings) { saveSettings = _saveSettings; }
    inline SettingsFile &getSettings() { return settings; }

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
    /// Initialize instance and device for OpenGL-Vulkan interoperability. Must be called before initializeSubsystems.
#if defined(SUPPORT_OPENGL) && defined(SUPPORT_VULKAN)
    void initializeVulkanInteropSupport(
            const std::vector<const char*>& requiredDeviceExtensionNames = {},
            const std::vector<const char*>& optionalDeviceExtensionNames = {},
            const vk::DeviceFeatures& requestedDeviceFeatures = vk::DeviceFeatures());
    inline VulkanInteropCapabilities getVulkanInteropCapabilities() { return vulkanInteropCapabilities; }
#endif
//#if defined(SUPPORT_OPENGL) && defined(SUPPORT_VULKAN)
//    void initializeCudaInteropSupport(
//            const std::vector<const char*>& requiredDeviceExtensionNames = {},
//            const std::vector<const char*>& optionalDeviceExtensionNames = {},
//            const vk::DeviceFeatures& requestedDeviceFeatures = vk::DeviceFeatures());
//    inline VulkanInteropCapabilities getVulkanInteropCapabilities() { return vulkanInteropCapabilities; }
//#endif
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

    void getCurrentDisplayMode(int& width, int& height, int& refreshRate, int displayIndex = 0);
    void getDesktopDisplayMode(int& width, int& height, int& refreshRate, int displayIndex = 0);
    glm::ivec2 getCurrentDisplayModeResolution(int displayIndex = 0);
    glm::ivec2 getDesktopResolution(int displayIndex = 0);

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
    Window* mainWindow = nullptr;

#ifdef SUPPORT_VULKAN
    vk::Instance* instance = nullptr;
    vk::Device* primaryDevice = nullptr;
    vk::Swapchain* swapchain = nullptr;
    VulkanInteropCapabilities vulkanInteropCapabilities = VulkanInteropCapabilities::NOT_LOADED;
    bool sdlVulkanLibraryLoaded = false;
    bool isDebugPrintfEnabled = false;
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
