/*
 * AppLogic.cpp
 *
 *  Created on: 02.09.2015
 *      Author: Christoph Neuhauser
 */

#include "AppLogic.hpp"
#include <Utils/File/Logfile.hpp>
#include <Utils/Timer.hpp>
#include <Graphics/Window.hpp>
#include <Utils/AppSettings.hpp>
#include <Utils/Events/EventManager.hpp>
#include <Input/Mouse.hpp>
#include <Input/Keyboard.hpp>
#include <Input/Gamepad.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/File/FileUtils.hpp>
#include <Graphics/Renderer.hpp>
#include <string>

namespace sgl {

AppLogic::AppLogic() : framerateSmoother(16)
{
    Timer->setFixedPhysicsFPS(true, 30);
    Timer->setFPSLimit(true, 60);
    running = true;
    screenshot = false;
    fpsCounterUpdateFrequency = 1e6;
    printFPS = true;
    fps = 60.0f;
}

AppLogic::~AppLogic()
{
}

void AppLogic::saveScreenshot(const std::string &filename)
{
    AppSettings::get()->getMainWindow()->saveScreenshot(filename.c_str());
}

void AppLogic::makeScreenshot()
{
    std::string filename = FileUtils::get()->getConfigDirectory() + "Screenshot";
    bool nonExistent = false;
    for (int i = 1; i < 999; ++i) {
        if (!FileUtils::get()->exists(filename + toString(i) + ".png")) {
            filename += toString(i);
            filename += ".png";
            nonExistent = true;
            break;
        }
    }
    if (!nonExistent) filename += "999.png";
    saveScreenshot(filename);
    screenshot = false;
}

void AppLogic::run()
{
    Window *window = AppSettings::get()->getMainWindow();
    // Used for only calling "updateFixed(...)" at fixed update rate
    uint64_t accumulatedTimeFixed = 0;
    float fixedFPSInMicroSeconds = Timer->getFixedPhysicsFPS()*1000000ul;
    uint64_t fpsTimer = 0;

    while (running) {
        Timer->update();
        accumulatedTimeFixed += Timer->getElapsedMicroseconds();

        do {
            updateFixed(Timer->getFixedPhysicsFPS());
            accumulatedTimeFixed -= fixedFPSInMicroSeconds;
        } while(Timer->getFixedPhysicsFPSEnabled() && accumulatedTimeFixed >= fixedFPSInMicroSeconds);

        running = window->processEvents([this](const SDL_Event &event) { this->processSDLEvent(event); });

        //float dt = Timer->getElapsedSeconds();
        framerateSmoother.addSample(1.0f/Timer->getElapsedSeconds());
        float dt = 1.0f / framerateSmoother.computeAverage();
        Mouse->update(dt);
        Keyboard->update(dt);
        Gamepad->update(dt);
        updateBase(dt);
        update(dt);

        // Decided to quit during update?
        if (!running) {
            break;
        }

        window->clear(Color(0, 0, 0));
        render();

        if (uint64_t(abs((int64_t)fpsTimer - (int64_t)Timer->getTicksMicroseconds())) > fpsCounterUpdateFrequency) {
            fps = 1.0f/dt;//Timer->getElapsedSeconds();
            fpsTimer = Timer->getTicksMicroseconds();
            if (printFPS) {
                std::cout << fps << std::endl;
            }
        }

        // Check for errors
        Renderer->errorCheck();
        window->errorCheck();

        // Save a screenshot before flipping the backbuffer surfaces if necessary
        if (screenshot) {
            makeScreenshot();
        }
        Timer->waitForFPSLimit();
        window->flip();
    }

    Logfile::get()->write("INFO: End of main loop.", BLUE);
}

void AppLogic::updateBase(float dt)
{
    EventManager::get()->update();
    if (Keyboard->keyPressed(SDLK_PRINTSCREEN)
            || ((Keyboard->getModifier()&KMOD_CTRL) && Keyboard->keyPressed(SDLK_p))) {
        screenshot = true;
    }
    if (Keyboard->keyPressed(SDLK_RETURN) && (Keyboard->getModifier()&KMOD_ALT)) {
        Logfile::get()->writeInfo("Switching to fullscreen (ALT-TAB)");
        AppSettings::get()->getMainWindow()->toggleFullscreen();
    }
}

void AppLogic::setPrintFPS(bool enabled)
{
    printFPS = enabled;
}


}
