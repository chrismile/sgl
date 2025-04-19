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

#ifndef LOGIC_APPLOGIC_HPP_
#define LOGIC_APPLOGIC_HPP_

#include <iostream>
#include <memory>

#ifdef SUPPORT_SDL
union SDL_Event;
#endif

#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/libs/volk/volk.h>
namespace sgl { namespace vk {
class Renderer;
}}
#endif

#ifdef SUPPORT_WEBGPU
namespace sgl { namespace webgpu {
class Renderer;
}}
#endif

#include "Utils/FramerateSmoother.hpp"

namespace sgl {

class Event;
typedef std::shared_ptr<Event> EventPtr;
class Window;

class DLL_OBJECT AppLogic {
public:
    AppLogic();
    virtual ~AppLogic();
    virtual void run();
    virtual void updateBase(float dt);

    /// Override these functions in the derived classes
    virtual void update(float dt) {} // Called once per rendered frame
    virtual void updateFixed(float dt) {} // Called at a fixed rate (e.g. for physics simulation)
#ifdef SUPPORT_SDL
    virtual void processSDLEvent(const SDL_Event &event) {}
#endif
    virtual void resolutionChanged(EventPtr event) {}
    virtual void render() {}

    virtual void setPrintFPS(bool enabled);
    [[nodiscard]] inline float getFPS() const { return fps; }
    inline void quit() { running = false; }

protected:
    // Main loop logic.
    virtual void runStep();
    uint64_t accumulatedTimeFixed = 0;
    uint64_t fpsTimer = 0;
    int64_t fixedFPSInMicroSeconds = 0;
    Window *window = nullptr;

    virtual void makeScreenshot();
    virtual void saveScreenshot(const std::string &filename);
    bool screenshot;
    float fps;
    FramerateSmoother framerateSmoother;

#ifdef SUPPORT_VULKAN
    sgl::vk::Renderer* rendererVk = nullptr;
#endif

#ifdef SUPPORT_WEBGPU
    sgl::webgpu::Renderer* rendererWgpu = nullptr;
#endif

    // For debuggers.
    virtual void beginFrameMarker() {}
    virtual void endFrameMarker() {}

private:
    bool running;
    uint64_t fpsCounterUpdateFrequency;
    bool printFPS;
};

}

/*! LOGIC_APPLOGIC_HPP_ */
#endif
