/*
 * Timer.cpp
 *
 *  Created on: 18.01.2015
 *      Author: Christoph Neuhauser
 */

#include <chrono>
#include <thread>

#include <SDL2/SDL.h>

#include "Timer.hpp"

namespace sgl {

TimerInterface::TimerInterface() : currentTime(0), lastTime(0), elapsedMicroSeconds(0),
        fpsLimitEnabled(true), fpsLimit(60), fixedPhysicsFPSEnabled(true), physicsFPS(60)
{
    perfFreq = SDL_GetPerformanceFrequency();
    startFrameTime = SDL_GetPerformanceCounter();
}

void TimerInterface::sleepMilliseconds(unsigned int milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void TimerInterface::waitForFPSLimit()
{
    if (!fpsLimitEnabled) {
        return;
    }

    uint64_t timeSinceUpdate = getTicksMicroseconds() - lastTime;
#ifdef _WIN32
    int64_t sleepTimeMicroSeconds = 1e6 / (fpsLimit+10) - timeSinceUpdate;
#else
    int64_t sleepTimeMicroSeconds = 1e6 / (fpsLimit+2) - timeSinceUpdate;
#endif
    if (sleepTimeMicroSeconds > 0) {
        std::this_thread::sleep_for(std::chrono::microseconds(sleepTimeMicroSeconds));
    }
}

void TimerInterface::update()
{
    if (lastTime == 0) {
        // Set elapsed time in first frame to frame limit (or 60FPS if not set otherwise)
        lastTime = getTicksMicroseconds();
        if (fpsLimitEnabled) {
            elapsedMicroSeconds = 1.0/fpsLimit*1e6;
        } else {
            elapsedMicroSeconds = 1.0/60.0*1e6;
        }
    } else {
        currentTime = getTicksMicroseconds();
        elapsedMicroSeconds = currentTime - lastTime;
        lastTime = currentTime;
    }

    // Normalize elapsed time for too big time steps, e.g. 10 seconds.
    if (elapsedMicroSeconds >= 10e6) {
        elapsedMicroSeconds = 10e6;
    }
}

uint64_t TimerInterface::getTicksMicroseconds()
{
    /*auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    return microseconds;*/
    uint64_t currentTime = static_cast<double>(SDL_GetPerformanceCounter() - startFrameTime) / perfFreq * 1e6;
    return currentTime;
}

}
