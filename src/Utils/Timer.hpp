/*!
 * Timer.hpp
 *
 *  Created on: 11.01.2015
 *      Author: Christoph Neuhauser
 */

#ifndef SYSTEM_TIMER_HPP_
#define SYSTEM_TIMER_HPP_

#include <cstring>
#include <cstdint>
#include <Defs.hpp>
#include <Utils/Singleton.hpp>

namespace sgl {

class TimerInterface
{
public:
    TimerInterface();

    void update();
    void sleepMilliseconds(unsigned int milliseconds);
    void waitForFPSLimit();

    uint64_t getTicksMicroseconds();
    float getTimeInSeconds() { return currentTime / 1e6; }
    uint64_t getElapsedMicroseconds() { return elapsedMicroSeconds; }
    float getElapsedSeconds() { return elapsedMicroSeconds / 1e6; }

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

extern TimerInterface *Timer;

}

/*! SYSTEM_TIMER_HPP_ */
#endif
