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

#include <Utils/File/Logfile.hpp>
#include <Utils/Timer.hpp>
#include <Graphics/Window.hpp>
#include <ImGui/ImGuiWrapper.hpp>
#include <Utils/AppSettings.hpp>
#include <Utils/Events/EventManager.hpp>
#include <Input/Mouse.hpp>
#include <Input/Keyboard.hpp>
#include <Input/Gamepad.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/File/FileUtils.hpp>
#include <Graphics/Renderer.hpp>
#include <string>

#ifdef SUPPORT_VULKAN
#include "Graphics/Vulkan/Utils/Swapchain.hpp"
#endif

#include "AppLogic.hpp"

namespace sgl {

AppLogic::AppLogic() : framerateSmoother(16)
{
    Timer->setFixedPhysicsFPS(true, 30);
    Timer->setFPSLimit(true, 60);
    running = true;
    screenshot = false;
    fpsCounterUpdateFrequency = uint64_t(1e6);
    printFPS = true;
    fps = 60.0f;

    Window *window = AppSettings::get()->getMainWindow();
    window->setEventHandler([this](const SDL_Event &event) { this->processSDLEvent(event); });
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
    int64_t fixedFPSInMicroSeconds = int64_t(1000000) / Timer->getFixedPhysicsFPS();
    uint64_t fpsTimer = 0;

    while (running) {
        Timer->update();
        accumulatedTimeFixed += Timer->getElapsedMicroseconds();

        do {
            updateFixed(float(Timer->getFixedPhysicsFPS()));
            accumulatedTimeFixed -= fixedFPSInMicroSeconds;
        } while(Timer->getFixedPhysicsFPSEnabled() && int64_t(accumulatedTimeFixed) >= fixedFPSInMicroSeconds);

        running = window->processEvents();

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

        if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
            window->clear(Color(0, 0, 0));
        }
#ifdef SUPPORT_VULKAN
        if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
            sgl::vk::Swapchain* swapchain = sgl::AppSettings::get()->getSwapchain();
            if (swapchain) {
                sgl::AppSettings::get()->getSwapchain()->beginFrame();
            }
        }
#endif
        if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
            commandBuffers.clear();
        }
        render();
        if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
            sgl::vk::Swapchain* swapchain = sgl::AppSettings::get()->getSwapchain();
            if (swapchain) {
                if (AppSettings::get()->getUseGUI()) {
                    commandBuffers.push_back(ImGuiWrapper::get()->getVkCommandBuffers().at(swapchain->getImageIndex()));
                }
                sgl::AppSettings::get()->getSwapchain()->renderFrame(commandBuffers);
            }
        }

        if (uint64_t(abs((int64_t)fpsTimer - (int64_t)Timer->getTicksMicroseconds())) > fpsCounterUpdateFrequency) {
            fps = 1.0f/dt;//Timer->getElapsedSeconds();
            fpsTimer = Timer->getTicksMicroseconds();
            if (printFPS) {
                std::cout << fps << std::endl;
            }
        }

        // Check for errors
        if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
            Renderer->errorCheck();
        }
        window->errorCheck();

        if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
            // Save a screenshot before flipping the backbuffer surfaces if necessary
            if (screenshot) {
                makeScreenshot();
            }
            Timer->waitForFPSLimit();
            window->flip();
        }
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
