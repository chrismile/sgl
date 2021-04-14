/*!
 * AppSettings.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph Neuhauser
 */

#ifndef SYSTEM_APPSETTINGS_HPP_
#define SYSTEM_APPSETTINGS_HPP_

#include <map>
#include <string>
#include <glm/vec2.hpp>
#include <Defs.hpp>
#include <Utils/Singleton.hpp>
#include <Utils/Convert.hpp>

namespace sgl {

class Window;

#ifdef SUPPORT_VULKAN
namespace vk { class Instance; }
#endif

// At the moment, only OpenGL and Vulkan are supported.
enum class RenderSystem {
    OPENGL, OPENGLES, VULKAN, DIRECT3D_11, DIRECT3D_12, METAL
};

enum class OperatingSystem {
    WINDOWS, LINUX, ANDROID, MACOSX, IOS, UNKNOWN
};

class SettingsFile {
public:
    inline std::string getValue(const char *key) const { auto it = settings.find(key); return it == settings.end() ? "" : it->second; }
    inline int getIntValue(const char *key) const { return fromString<int>(getValue(key)); }
    inline float getFloatValue(const char *key) const { return fromString<float>(getValue(key)); }
    inline bool getBoolValue(const char *key) const { std::string val = getValue(key); if (val == "false" || val == "0") return false; return val.length() > 0; }
    inline void addKeyValue(const std::string &key, const std::string &value) { settings[key] = value; }
    template<class T> inline void addKeyValue(const std::string &key, const T &value) { settings[key] = toString(value); }
    inline void clear() { settings.clear(); }

    void getValueOpt(const char *key, std::string &toset) const { auto it = settings.find(key); if (it != settings.end()) { toset = it->second; } }
    template<class T> void getValueOpt(const char *key, T &toset) const { auto it = settings.find(key); if (it != settings.end()) { toset = fromString<T>(it->second); } }

    void saveToFile(const char *filename);
    void loadFromFile(const char *filename);

private:
    std::map<std::string, std::string> settings;
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
    void loadSettings(const char *filename);
    SettingsFile &getSettings() { return settings; }

    /// setRenderSystem must be called before calling initializeSubsystems and createWindow.
    void setRenderSystem(RenderSystem renderSystem);
    void initializeSubsystems();
    Window* createWindow();
    /// Only Vulkan supports headless rendering (without a window) for now.
    HeadlessData createHeadless();
    void release();

    /// Called in main if GUI should be loaded.
    void setLoadGUI(
            const unsigned short* fontRangeData = nullptr, bool useDocking = true, bool useMultiViewport = true,
            float uiScaleFactor = 1.0f);

    inline RenderSystem getRenderSystem() { return renderSystem; }
    inline OperatingSystem getOS() { return operatingSystem; }
    Window* getMainWindow();
    Window* setMainWindow(Window* window);

#ifdef SUPPORT_VULKAN
    vk::Instance* getVulkanInstance() { return instance; }
#endif

    void getCurrentDisplayMode(int& width, int& height, int& refreshRate, int displayIndex = 0);
    void getDesktopDisplayMode(int& width, int& height, int& refreshRate, int displayIndex = 0);
    glm::ivec2 getCurrentDisplayModeResolution(int displayIndex = 0);
    glm::ivec2 getDesktopResolution(int displayIndex = 0);

    // Get the directory where the application data is stored.
    inline const std::string& getDataDirectory() const { return dataDirectory; }
    inline bool getHasCustomDataDirectory() const { return hasCustomDataDirectory; }

private:
    SettingsFile settings;
    std::string settingsFilename;
    RenderSystem renderSystem = RenderSystem::OPENGL;
    OperatingSystem operatingSystem;
    Window* mainWindow = nullptr;

#ifdef SUPPORT_VULKAN
    vk::Instance* instance = nullptr;
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
};

}

/*! SYSTEM_APPSETTINGS_HPP_ */
#endif
