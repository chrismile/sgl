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

#ifndef SYSTEM_TIMER_HPP_
#define SYSTEM_TIMER_HPP_

#include <cstring>
#include <cstdint>
#include <Defs.hpp>
#include <Utils/Singleton.hpp>

namespace sgl {

class DLL_OBJECT TimerInterface
{
public:
    TimerInterface();

    void update();
    void sleepMilliseconds(unsigned int milliseconds);
    void waitForFPSLimit();

    uint64_t getTicksMicroseconds() const;
    float getTimeInSeconds() const { return currentTime / 1e6f; }
    uint64_t getElapsedMicroseconds() const { return elapsedMicroSeconds; }
    float getElapsedSeconds() const { return elapsedMicroSeconds / 1e6f; }

    /**
     * In real-time applications, we usually have two main goals:
     * - Hit perfect VSync refresh rate of the monitor (e.g. 60FPS)
     * - Update physics simulations at a fixed FPS rate (e.g. 30FPS)
     * The following two functions can help for reaching these goals.
     */

    /// Sets whether an FPS cap should be used. This is necessary for e.g. applications without VSync that
    /// don't want to utilize 100% of the system resources.
    inline void setFPSLimit(bool enabled, unsigned int fpsLimit) {
        fpsLimitEnabled = enabled; this->fpsLimit = fpsLimit;
    }
    inline bool getFPSLimitEnabled() { return fpsLimitEnabled; }
    inline unsigned int getTargetFPS() { return fpsLimit; }

    /// Sets whether we want fixed FPS for physics updates. You can place functions that expect this fixed
    /// FPS in AppSettings::fixedUpdate(float dt).
    inline void setFixedPhysicsFPS(bool enabled, unsigned int physicsFPS) {
        fixedPhysicsFPSEnabled = enabled; this->physicsFPS = physicsFPS;
    }
    inline bool getFixedPhysicsFPSEnabled() { return fixedPhysicsFPSEnabled; }
    inline unsigned int getFixedPhysicsFPS() { return physicsFPS; }

private:
    uint64_t currentTime, lastTime, elapsedMicroSeconds;
    uint64_t perfFreq;
    uint64_t startFrameTime;

    bool fpsLimitEnabled;
    unsigned int fpsLimit;
    bool fixedPhysicsFPSEnabled;
    unsigned int physicsFPS;
};

DLL_OBJECT extern TimerInterface *Timer;

}

/*! SYSTEM_TIMER_HPP_ */
#endif
