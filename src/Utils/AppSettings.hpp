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
#include <Defs.hpp>
#include <Utils/Singleton.hpp>
#include <Utils/Convert.hpp>

namespace sgl {

class Window;

enum RenderSystem {
    OPENGL, OPENGLES, DIRECTX
};
enum OperatingSystem {
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

/*! AppSettings can application-specific settings on the one hand, and basic settings
 * like the screen resolution or the audio system on the other hand.
 * AppSettings will create the instances of the render system, etc. on startup.
 */
class DLL_OBJECT AppSettings : public Singleton<AppSettings>
{
public:
    void loadSettings(const char *filename);
    SettingsFile &getSettings() { return settings; }
    void initializeSubsystems();
    Window *createWindow();
    void release();

    // Called in main if GUI should be loaded
    void setLoadGUI(const unsigned short* fontRangeData = nullptr) { useGUI = true; this->fontRangesData = fontRangeData; }

    inline RenderSystem getRenderSystem() { return renderSystem; }
    inline OperatingSystem getOS() { return operatingSystem; }
    Window *getMainWindow();
    Window *setMainWindow(Window *window);

private:
    SettingsFile settings;
    std::string settingsFilename;
    RenderSystem renderSystem;
    OperatingSystem operatingSystem;
    Window *mainWindow;
    bool useGUI = false;
    const unsigned short* fontRangesData = nullptr;
};

}

/*! SYSTEM_APPSETTINGS_HPP_ */
#endif
