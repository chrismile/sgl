/*
 * AppSettings.cpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph Neuhauser
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
#include <Graphics/Vulkan/Utils/Instance.hpp>
#include <Graphics/Scene/Camera.hpp>

#endif

#ifdef WIN32
#include <windows.h>
#include <VersionHelpers.h>
#endif

namespace sgl {

RendererInterface *Renderer = NULL;
TimerInterface *Timer = NULL;
ShaderManagerInterface *ShaderManager = NULL;
TextureManagerInterface *TextureManager = NULL;
MaterialManagerInterface *MaterialManager = NULL;

MouseInterface *Mouse = NULL;
KeyboardInterface *Keyboard = NULL;
GamepadInterface *Gamepad = NULL;

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

    // Make sure the "Data" directory exists.
    // If not: Create a symbolic link to "Data" in the parent folder if it exists.
    if (!sgl::FileUtils::get()->exists("Data") && sgl::FileUtils::get()->directoryExists("../Data")) {
        boost::filesystem::create_directory_symlink("../Data", "Data");
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

void AppSettings::setRenderSystem(RenderSystem renderSystem) {
    assert(!mainWindow);
    this->renderSystem = renderSystem;
}

void AppSettings::initializeSubsystems()
{
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

    /*if (TTF_Init() == -1) {
        Logfile::get()->writeError("ERROR: SDLWindow::initializeAudio: Couldn't initialize SDL_ttf!");
        Logfile::get()->writeError(std::string() + "SDL_ttf initialization error: " + TTF_GetError());
    }*/

    // Create the subsystem implementations
    Timer = new TimerInterface;
    //AudioManager = new SDLMixerAudioManager;
    MaterialManager = new MaterialManagerInterface;

#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        TextureManager = new TextureManagerGL;
        ShaderManager = new ShaderManagerGL;
        Renderer = new RendererGL;
        SystemGL::get();
    }
#endif
#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN) {
        Camera::depthRange = Camera::DEPTH_RANGE_ZERO_ONE;
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
    mainWindow->serializeSettings(settings);
    settings.saveToFile(settingsFilename.c_str());

    if (useGUI) {
        ImGuiWrapper::get()->shutdown();
    }

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
    //delete AudioManager;
    delete Timer;

    mainWindow->close();

#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN) {
        delete instance;
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
