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

#include <chrono>
#include <thread>

#ifdef SUPPORT_SDL2
#include <SDL2/SDL.h>
#endif
#ifdef SUPPORT_SDL3
#include <SDL3/SDL.h>
#endif

#ifdef SUPPORT_GLFW
#include <GLFW/glfw3.h>
#endif

#include <Utils/AppSettings.hpp>
#include <Utils/File/Logfile.hpp>
#include <Graphics/Window.hpp>
#include "Timer.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>
#endif

namespace sgl {

TimerInterface::TimerInterface() : currentTime(0), lastTime(0), elapsedMicroSeconds(0),
        fpsLimitEnabled(true), fpsLimit(60), fixedPhysicsFPSEnabled(true), physicsFPS(60) {
#if defined(SUPPORT_SDL) || defined(SUPPORT_GLFW)
    WindowBackend windowBackend = AppSettings::get()->getWindowBackend();
#endif
#ifdef SUPPORT_SDL
    if (getIsSdlWindowBackend(windowBackend)) {
        perfFreq = SDL_GetPerformanceFrequency();
        startFrameTime = SDL_GetPerformanceCounter();
    }
#endif
#ifdef SUPPORT_GLFW
    if (windowBackend == WindowBackend::GLFW_IMPL) {
        perfFreq = glfwGetTimerFrequency();
        startFrameTime = glfwGetTimerValue();
    }
#endif

#ifdef _WIN32
    timerHandle = CreateWaitableTimerExW(
        nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    if (!timerHandle) {
        if (GetLastError() != ERROR_INVALID_PARAMETER) {
            Logfile::get()->writeError(
                "TimerInterface::TimerInterface: CreateWaitableTimerExW failed with error code "
                + std::to_string(GetLastError()) + ".");
        }
    }
#endif
}

TimerInterface::~TimerInterface() {
#ifdef _WIN32
    if (timerHandle) {
        CloseHandle(timerHandle);
        timerHandle = {};
    }
#endif
}

void TimerInterface::sleepMilliseconds(unsigned int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void TimerInterface::waitForFPSLimit() {
    if (!fpsLimitEnabled) {
        return;
    }

    uint64_t timeSinceUpdate = getTicksMicroseconds() - lastTime;
#ifdef _WIN32
    auto sleepTimeMicroSeconds = int64_t(1e6 / double(fpsLimit + 10) - double(timeSinceUpdate));
#else
    auto sleepTimeMicroSeconds = int64_t(1e6 / double(fpsLimit + 2) - double(timeSinceUpdate));
#endif
    if (sleepTimeMicroSeconds > 0) {
#ifdef _WIN32
        if (timerHandle) {
            /*
             * CREATE_WAITABLE_TIMER_HIGH_RESOLUTION is only supported since Windows 10, version 1803.
             * Wait time is specified in 100 nanosecond intervals.
             */
            LARGE_INTEGER waitTime;
            waitTime.QuadPart = -sleepTimeMicroSeconds * 10;
            if (!SetWaitableTimer(timerHandle, &waitTime, 0, nullptr, nullptr, 0)) {
                Logfile::get()->writeError(
                    "Error in TimerInterface::waitForFPSLimit: SetWaitableTimer failed with error code "
                    + std::to_string(GetLastError()) + ".");
                return;
            }
            if (WaitForSingleObject(timerHandle, INFINITE) != WAIT_OBJECT_0) {
                Logfile::get()->writeError(
                    "Error in TimerInterface::waitForFPSLimit: WaitForSingleObject failed with error code "
                    + std::to_string(GetLastError()) + ".");
                return;
            }
        } else
#endif
        std::this_thread::sleep_for(std::chrono::microseconds(sleepTimeMicroSeconds));
    }
}

void TimerInterface::update() {
    if (lastTime == 0) {
        // Set elapsed time in first frame to frame limit (or 60FPS if not set otherwise)
        lastTime = getTicksMicroseconds();
        if (fpsLimitEnabled) {
            elapsedMicroSeconds = int64_t(1.0 / fpsLimit * 1e6);
        } else {
            elapsedMicroSeconds = int64_t(1.0 / 60.0 * 1e6);
        }
    } else {
        currentTime = getTicksMicroseconds();
        elapsedMicroSeconds = currentTime - lastTime;
        lastTime = currentTime;
    }

    // Normalize elapsed time for too big time steps, e.g. 10 seconds.
    if (elapsedMicroSeconds >= uint64_t(10e6)) {
        elapsedMicroSeconds = uint64_t(10e6);
    }
}

uint64_t TimerInterface::getTicksMicroseconds() const {
#if defined(SUPPORT_SDL) || defined(SUPPORT_GLFW)
    auto* window = AppSettings::get()->getMainWindow();
#endif
#ifdef SUPPORT_SDL
    if (getIsSdlWindowBackend(window->getBackend())) {
        auto _currentTime =
                uint64_t(static_cast<double>(SDL_GetPerformanceCounter() - startFrameTime) / double(perfFreq) * 1e6);
        return _currentTime;
    }
#endif

#ifdef SUPPORT_GLFW
    if (window->getBackend() == WindowBackend::GLFW_IMPL) {
        auto _currentTime =
                uint64_t(static_cast<double>(glfwGetTimerValue() - startFrameTime) / double(perfFreq) * 1e6);
        return _currentTime;
    }
#endif

    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    return microseconds;
}

}
