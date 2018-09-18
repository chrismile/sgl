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
#include <Graphics/OpenGL/RendererGL.hpp>
#include <Graphics/OpenGL/ShaderManager.hpp>
#include <Graphics/OpenGL/TextureManager.hpp>
#include <Graphics/OpenGL/SystemGL.hpp>
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
#include <windows.h>
#include <VersionHelpers.h>
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
		file << "\"" + it->first + "\": \"" + it->second  + "\"";
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

	SDLWindow *window = new SDLWindow;
	WindowSettings windowSettings = window->deserializeSettings(settings);
	windowSettings.width = 1920;
	windowSettings.height = 1080;
	//windowSettings.resizable = false;
	//windowSettings.fullscreen = true;
	//windowSettings.vSync = false;
	window->initialize(windowSettings);

	mainWindow = window;
	return window;
}

void AppSettings::initializeSubsystems()
{
	Logfile::get()->createLogfile((FileUtils::get()->getConfigDirectory() + "Logfile.html").c_str(), "ShapeDetector");

#ifndef OPENGLES
	renderSystem = OPENGL;
#else
	renderSystem = OPENGLES;
#endif

	operatingSystem = UNKNOWN;
#ifdef WIN32
	operatingSystem = WINDOWS;
#endif
#ifdef __linux__
	operatingSystem = LINUX;
#endif
#ifdef ANDROID
	operatingSystem = ANDROID;
#endif
#ifdef MACOSX
	operatingSystem = MACOSX;
#endif

	/*if (TTF_Init() == -1) {
		Logfile::get()->writeError("ERROR: SDLWindow::initializeAudio: Couldn't initialize SDL_ttf!");
		Logfile::get()->writeError(std::string() + "SDL_ttf initialization error: " + TTF_GetError());
	}*/

	// Create the subsystem implementations
	Timer = new TimerInterface;
	//AudioManager = new SDLMixerAudioManager;
	TextureManager = new TextureManagerGL;
	ShaderManager = new ShaderManagerGL;
	MaterialManager = new MaterialManagerInterface;
	Renderer = new RendererGL;

	Mouse = new SDLMouse;
	Keyboard = new SDLKeyboard;
	Gamepad = new SDLGamepad;

	if (useGUI) {
		ImGuiWrapper::get()->initialize();
	}

	SystemGL::get();
}

void AppSettings::release()
{
    mainWindow->serializeSettings(settings);
	settings.saveToFile(settingsFilename.c_str());

    ImGuiWrapper::get()->shutdown();

    delete Renderer;
	delete MaterialManager;
	delete ShaderManager;
	delete TextureManager;
	//delete AudioManager;
	delete Timer;

	//Mix_CloseAudio();
	//TTF_Quit();
	SDL_Quit();
}

Window *AppSettings::getMainWindow()
{
	return mainWindow;
}

Window *AppSettings::setMainWindow(Window *window)
{
	return mainWindow;
}

}
