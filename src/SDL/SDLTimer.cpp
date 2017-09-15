/*
 * SDLTimer.cpp
 *
 *  Created on: 30.01.2015
 *      Author: Christoph
 */

#include "SDLTimer.hpp"
#include <Utils/File/Logfile.hpp>

#ifdef _WIN32
#include <windows.h>

namespace sgl {

SDLTimer::SDLTimer()
{
	highResTimerSupported = true;
	if (!QueryPerformanceFrequency(&highResFrequency)) {
		Logfile::get()->writeError("SDLTimer::SDLTimer: High-resolution timer not supported!");
		highResTimerSupported = false;
	}
}

uint64_t SDLTimer::getMicroSecondsTicks()
{
	if (highResTimerSupported) {
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		//return li.QuadPart * 1000000ULL / highResFrequency.QuadPart; // Possible uint64_t overflow?
		double inverseMicroSecsFrequency = 1000000.0 / highResFrequency.QuadPart;
		return li.QuadPart * inverseMicroSecsFrequency;
	}
	return getTicks() * 1000ULL;
}
#else
#include <iostream>
#include <chrono>

namespace sgl {

SDLTimer::SDLTimer()
{
	highResTimerSupported = true;
}

uint64_t SDLTimer::getMicroSecondsTicks()
{
	auto now = std::chrono::high_resolution_clock::now();
	auto duration = now.time_since_epoch();
	auto millis = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
	return millis;
}

#endif

}
