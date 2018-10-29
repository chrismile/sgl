/**
 * @file TimerGL.hpp
 *
 *  Created on: 20.08.2018
 *      Author: Christoph Neuhauser
 */

#ifndef TIMERGL_HPP_
#define TIMERGL_HPP_

#include <string>
#include <vector>
#include <map>

namespace sgl
{

// http://www.lighthouse3d.com/tutorials/opengl-timer-query/
// https://www.khronos.org/opengl/wiki/Query_Object

// NOTE: Does not support nested start-end calls!

/*!
 * TimerGL has functions for profiling OpenGL API calls.
 * The time for an event can be measured multiple frames to get the average time returned by "getTimeMS".
 */
class TimerGL
{
public:
    /// Calls deleteAll
    ~TimerGL();

    /// Clears all stored queries
	void deleteAll();

	/// Start measuring time for event with specified name (NOTE: No nested calls!)
    void start(const std::string &name);
    /// End measuring time for last event
    void end();

    /// Get the (average) time the event with the specified name took
	double getTimeMS(const std::string &name);
	/// Prints the time returned by "getTimeMS"
	void printTimeMS(const std::string &name);
	// Prints sum of all average times
	void printTotalAvgTime();

private:
	void addQueryTime(size_t index);
	size_t lastIndex = 0;

    /// The names of the event regions mapped to indices for the lists below
	std::map<std::string, size_t> regionNameMap;
    /// The OpenGL query IDs
	std::vector<unsigned int> queryIDs;
	/// The accumulated time each event took
	std::vector<uint64_t> elapsedTimeNS;
	/// The number of measurements of each event (for computing average)
	std::vector<size_t> numSamples;
	/// Whether a certain query has ended, but was not yet added to elapsedTimeNS and numSamples
	std::vector<bool> queryHasFinished;
};

}

#endif /* TIMERGL_HPP_ */
