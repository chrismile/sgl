/*
 * Timer.cpp
 *
 *  Created on: 18.01.2015
 *      Author: Christoph Neuhauser
 */

#include "Timer.hpp"

namespace sgl {

TimerInterface::TimerInterface() : frameSmoother(10, 1.0f/60.0f)
{
	elapsed  = 0;
	currTime  = 0;
	lastTime = 0;
	currTimeSecs = 0.0f;
	elapsedSecs = 0.0f;
	fixedTimeStep = false;
	fixedFPS = 0;
}

void TimerInterface::update()
{
	if (fixedTimeStep) {
		waitForFPS(fixedFPS+40);
	} else {
		waitForFPS(1000);
	}

	// First frame?
	if (lastTime == 0) {
		lastTime = getTicks()-1;
	}

	currTime = getTicks();
	elapsed = currTime - lastTime;
	lastTime = currTime;

	// Normalize elapsed for too big time steps
	if (elapsed >= 200) {
		elapsed = 200;
	}

	// Add time if FPS is smoothed
	if (fixedTimeStep) 	{
		frameSmoother.addValue(elapsed/1000.0f);
	}

	// Convert the time to seconds
	currTimeSecs = currTime/1000.0f;
	elapsedSecs = elapsed/1000.0f;
}


float TimerInterface::getElapsed()
{
	if (!fixedTimeStep) {
		return elapsed/1000.0f;
	} else {
		if (1.0f/frameSmoother.getSmoothedValue() >= static_cast<float>(fixedFPS)) {
			return 1.0f/fixedFPS;
		} else {
			return frameSmoother.getSmoothedValue();
		}
	}
}

void TimerInterface::waitForFPS(unsigned int fps)
{
	while(!isFPS(fps)) {
		delay(1);
	}
}

bool TimerInterface::isFPS(unsigned int fps)
{
	if (getTicks() - lastTime < 1000.0f / fps) {
		return false;
	}
	return true;
}


void TimerInterface::setFixedFPS(uint32_t fps, int smoothingFilterSize)
{
	fixedTimeStep = true;
	fixedFPS = fps;
	frameSmoother.setStdValue(1.0f/static_cast<float>(fps));
	if(smoothingFilterSize > 1) {
		frameSmoother.setBufferSize(smoothingFilterSize);
	}
}

void TimerInterface::disableFixedFPS()
{
	fixedTimeStep = false;
}

}
