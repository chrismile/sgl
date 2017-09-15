/*!
 * SDLTimer.hpp
 *
 *  Created on: 19.01.2015
 *      Author: Christoph
 */

#ifndef SDL_SDLTIMER_HPP_
#define SDL_SDLTIMER_HPP_

#include <SDL2/SDL.h>
#include <Utils/Timer.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

namespace sgl {

class SDLTimer : public TimerInterface
{
public:
	SDLTimer();
	//! in milliseconds
	virtual uint32_t getTicks() { return SDL_GetTicks(); }
	virtual void delay(uint32_t milliseconds) { SDL_Delay(milliseconds); }

	/*! High-resolution timer
	 * Windows implementation: QueryPerformanceCounter (can vary across platforms and threads...)
	 * POSIX solution: std::chrono (not reliable at least on MSVC) */
	virtual uint64_t getMicroSecondsTicks();

private:
	bool highResTimerSupported;
#ifdef _WIN32
	LARGE_INTEGER highResFrequency;
#endif
};

}

/*! SDL_SDLTIMER_HPP_ */
#endif
