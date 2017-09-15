/*
 * Timer.hpp
 *
 *  Created on: 11.01.2015
 *      Author: Christoph
 */

#ifndef SYSTEM_TIMER_HPP_
#define SYSTEM_TIMER_HPP_

#include <cstring>
#include <cstdint>
#include <Defs.hpp>
#include <Utils/Singleton.hpp>

namespace sgl {

template<typename T>
class DLL_OBJECT FrameSmoother
{
public:
	FrameSmoother(int _filterSize, const T &stdValue)
		: cursor(0), filterSize(_filterSize), stdValue(stdValue) {
		buffer = new T[filterSize];
	}

	~FrameSmoother() {
		delete[] buffer;
	}

	void setBufferSize(int _filterSize) {
		reset();
		delete[] buffer;
		filterSize = _filterSize;
		buffer = new T[filterSize];
		cursor = 0;
	}

	// Adds a FPS value to be smoothed
	void addValue(const T &value) {
		buffer[cursor++ % filterSize] = value;
	}

	T getSmoothedValue() const {
		int n = cursor < filterSize ? cursor : filterSize;
		if (n == 0) {
			return stdValue;
		} else {
			T avg;
			memset(&avg, 0, sizeof(avg));
			for (int i = 0; i < n; ++i) {
				avg += buffer[i];
			}
			avg /= static_cast<T>(n);
			return avg;
		}
	}

	void reset() {
		cursor = 0;
	}

	void setStdValue(const T &value) {
		stdValue = value;
	}

protected:
	T *buffer;
	int cursor;
	int filterSize;
	T stdValue;
};



class DLL_OBJECT TimerInterface
{
public:
	TimerInterface();
	virtual ~TimerInterface() {}
	void update();
	virtual uint32_t getTicks()=0; // in milliseconds
	virtual void delay(uint32_t milliseconds)=0;

	inline float getElapsedRealTime() { return elapsed; }
	float getElapsed(); // Get elapsed time in seconds
	inline uint32_t getTimeInMS() { return currTime; }
	inline float getTimeInS() { return currTimeSecs; }

	void waitForFPS(uint32_t fps);
	bool isFPS(uint32_t fps);
	void setFixedFPS(uint32_t fps, int smoothingFilterSize = -1);

	// High-resolution timer
	// Windows implementation: QueryPerformanceCounter (can vary across platforms and threads...)
	// POSIX solution: std::chrono (not reliable at least on MSVC)
	virtual uint64_t getMicroSecondsTicks()=0;

private:
	uint32_t elapsed;  // Elapsed time since the last frame
	uint32_t currTime;  // Current time in milliseconds
	uint32_t lastTime; // Time of last frame
	float currTimeSecs;  // Current time in milliseconds
	float elapsedSecs;  // Elapsed time since the last frame in milliseconds

	// Fixed FPS and frame smoothing
	bool fixedTimeStep;
	uint32_t fixedFPS;
	FrameSmoother<float> frameSmoother;
};

extern TimerInterface *Timer;

}

#endif /* SYSTEM_TIMER_HPP_ */
