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

#include <string>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

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
#include <tracy/Tracy.hpp>

#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/Utils/Swapchain.hpp>
#include <Graphics/Vulkan/Render/Renderer.hpp>
#endif

#ifdef SUPPORT_WEBGPU
#include <Graphics/WebGPU/Utils/Swapchain.hpp>
#include <Graphics/WebGPU/Render/Renderer.hpp>
#endif

#ifdef SUPPORT_SDL
#include <SDL/SDLWindow.hpp>
#endif

#include "AppLogic.hpp"

namespace sgl {

AppLogic::AppLogic() : framerateSmoother(16) {
    Timer->setFixedPhysicsFPS(true, 30);
    Timer->setFPSLimit(true, 60);
    running = true;
    screenshot = false;
    fpsCounterUpdateFrequency = uint64_t(1e6);
    printFPS = true;
    fps = 60.0f;

    window = AppSettings::get()->getMainWindow();
#ifdef SUPPORT_SDL
    if (getIsSdlWindowBackend(window->getBackend())) {
        static_cast<SDLWindow*>(window)->setEventHandler([this](const SDL_Event &event) {
            this->processSDLEvent(event);
        });
    }
#endif

#ifdef SUPPORT_VULKAN
    if (sgl::AppSettings::get()->getPrimaryDevice()) {
        rendererVk = new sgl::vk::Renderer(sgl::AppSettings::get()->getPrimaryDevice());
    }
#endif

#ifdef SUPPORT_WEBGPU
    if (sgl::AppSettings::get()->getWebGPUPrimaryDevice()) {
        rendererWgpu = new sgl::webgpu::Renderer(sgl::AppSettings::get()->getWebGPUPrimaryDevice());
    }
#endif
}

AppLogic::~AppLogic() {
#ifdef SUPPORT_VULKAN
    if (sgl::AppSettings::get()->getPrimaryDevice()) {
        delete rendererVk;
        rendererVk = nullptr;
    }
#endif

#ifdef SUPPORT_WEBGPU
    if (sgl::AppSettings::get()->getWebGPUPrimaryDevice()) {
        delete rendererWgpu;
        rendererWgpu = nullptr;
    }
#endif
}

void AppLogic::saveScreenshot(const std::string &filename) {
    AppSettings::get()->getMainWindow()->saveScreenshot(filename.c_str());
}

void AppLogic::makeScreenshot() {
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

void AppLogic::run() {
    window = AppSettings::get()->getMainWindow();
    // Used for only calling "updateFixed(...)" at fixed update rate
    accumulatedTimeFixed = 0;
    fixedFPSInMicroSeconds = int64_t(1000000) / Timer->getFixedPhysicsFPS();
    fpsTimer = 0;

#ifdef __EMSCRIPTEN__
    auto mainLoopCallback = [](void *arg) {
        auto* appLogic = reinterpret_cast<AppLogic*>(arg);
        appLogic->runStep();
        if (!appLogic->running) {
            emscripten_cancel_main_loop();
            delete appLogic;
            sgl::AppSettings::get()->release();
        }
    };
    emscripten_set_main_loop_arg(mainLoopCallback, (void*)this, 0, true);
#else
    while (running) {
        runStep();
    }
#endif

    Logfile::get()->write("INFO: End of main loop.", BLUE);
}

void AppLogic::runStep() {
    Timer->update();
    accumulatedTimeFixed += Timer->getElapsedMicroseconds();

    do {
        updateFixed(float(Timer->getFixedPhysicsFPS()));
        accumulatedTimeFixed -= fixedFPSInMicroSeconds;
    } while(Timer->getFixedPhysicsFPSEnabled() && int64_t(accumulatedTimeFixed) >= fixedFPSInMicroSeconds);

    bool windowRunning = window->processEvents();
    running = running && windowRunning;

    //float dt = Timer->getElapsedSeconds();
    framerateSmoother.addSample(1.0f/Timer->getElapsedSeconds());
    float dt = 1.0f / framerateSmoother.computeAverage();
    Mouse->update(dt);
    Keyboard->update(dt);
    Gamepad->update(dt);
    updateBase(dt);
    update(dt);
    Keyboard->clearKeyBuffer(); //< getKeyBuffer can be used in child class update.

    // Decided to quit during update?
    if (!running) {
        return;
    }

    beginFrameMarker();

    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
        window->clear(Color(0, 0, 0));
    }
#ifdef SUPPORT_VULKAN
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
        auto* swapchain = sgl::AppSettings::get()->getSwapchain();
        if (swapchain) {
            swapchain->beginFrame();
        }
    }
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
        rendererVk->beginCommandBuffer();
    }
#endif

#ifdef SUPPORT_WEBGPU
    bool swapchainValid = true;
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::WEBGPU) {
        auto* swapchain = sgl::AppSettings::get()->getWebGPUSwapchain();
        if (swapchain) {
            swapchainValid = swapchain->beginFrame();
        }
    }
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::WEBGPU && swapchainValid) {
        rendererWgpu->beginCommandBuffer();
    }
#endif

#ifdef SUPPORT_WEBGPU
    if (swapchainValid) {
        render();
    }
#else
    render();
#endif

#ifdef SUPPORT_VULKAN
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
        rendererVk->endCommandBuffer();
        auto* swapchain = sgl::AppSettings::get()->getSwapchain();
        if (swapchain) {
            swapchain->renderFrame(rendererVk->getFrameCommandBuffers());
        }
    }
#endif

#ifdef SUPPORT_WEBGPU
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::WEBGPU && swapchainValid) {
        rendererWgpu->endCommandBuffer();
        auto* swapchain = sgl::AppSettings::get()->getWebGPUSwapchain();
        if (swapchain) {
            swapchain->renderFrame(rendererWgpu->getFrameCommandBuffers());
        }
        rendererWgpu->freeFrameCommandBuffers();
    }
#endif

    if (uint64_t(abs((int64_t)fpsTimer - (int64_t)Timer->getTicksMicroseconds())) > fpsCounterUpdateFrequency) {
        fps = 1.0f/dt;//Timer->getElapsedSeconds();
        fpsTimer = Timer->getTicksMicroseconds();
        if (printFPS) {
            std::cout << fps << std::endl;
        }
    }

    // Check for errors
#ifdef SUPPORT_OPENGL
    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
        Renderer->errorCheck();
    }
#endif
    window->errorCheck();

    if (sgl::AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
        // Save a screenshot before flipping the backbuffer surfaces if necessary
        if (screenshot) {
            makeScreenshot();
        }
        Timer->waitForFPSLimit();
        window->flip();
    }

    endFrameMarker();

#ifdef TRACY_ENABLE
    FrameMark;
#endif
}

void AppLogic::updateBase(float dt) {
    EventManager::get()->update();
    if (Keyboard->keyPressed(ImGuiKey_PrintScreen)
            || (Keyboard->getModifier(ImGuiKey_ModCtrl) && Keyboard->keyPressed(ImGuiKey_P))) {
        screenshot = true;
    }
    if (Keyboard->keyPressed(ImGuiKey_Enter) && Keyboard->getModifier(ImGuiKey_ModAlt)) {
        Logfile::get()->writeInfo("Switching to fullscreen (ALT-TAB)");
        AppSettings::get()->getMainWindow()->toggleFullscreen();
    }
}

void AppLogic::setPrintFPS(bool enabled) {
    printFPS = enabled;
}


}
