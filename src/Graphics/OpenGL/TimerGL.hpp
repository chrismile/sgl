/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2018, Christoph Neuhauser
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

#ifndef TIMERGL_HPP_
#define TIMERGL_HPP_

#include <string>
#include <vector>
#include <map>
#include <chrono>

namespace sgl
{

// http://www.lighthouse3d.com/tutorials/opengl-timer-query/
// https://www.khronos.org/opengl/wiki/Query_Object

// NOTE: Does not support nested start-end calls!

/*!
 * TimerGL has functions for profiling OpenGL API calls.
 * The time for an event can be measured multiple frames to get the average time returned by "getTimeMS".
 */
class DLL_OBJECT TimerGL
{
public:
    /// Calls deleteAll
    ~TimerGL();

    /// Clears all stored queries
    void deleteAll();

    /**
     * Start measuring time for event with specified name (NOTE: No nested calls!)
     * @param name The name of the event
     * @param timeStamp The time stamp of the current frame (for recording frame time graphs).
     */
    void startGPU(const std::string &name, float timeStamp);
    /// End measuring time for last event
    void end();
    /**
     * Stop last measuring event. Necessary, as time is queried at beginning of next call to @ref start.
     * Problematic if we want to quit before next call to start.
     * NOTE: There must not be another call to @ref start for the same event afterwards!
     */
    void stopMeasuring();

    /**
     * Start measuring time for event with specified name (NOTE: No nested calls!)
     * @param name The name of the event
     * @param timeStamp The time stamp of the current frame (for recording frame time graphs).
     */
    void startCPU(const std::string &name, float timeStamp);

    /// Get the (average) time the event with the specified name took
    double getTimeMS(const std::string &name);
    /// Prints the time returned by "getTimeMS"
    void printTimeMS(const std::string &name);
    // Prints sum of all average times
    void printTotalAvgTime();
    /// Get the (average) time the event with the specified name took
    const std::vector<std::pair<float, uint64_t>> &getCurrentFrameTimeList() { return frameTimeList; }

private:
    void addQueryTime(size_t index, float timeStamp);
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
    /// Whether a certain query has ended, but was not yet added to elapsedTimeNS and numSamples
    std::vector<bool> isGPUQuery;

    /// List: Frame time stamp -> frame time in milliseconds
    float lastTimeStamp = 0.0f;
    std::vector<std::pair<float, uint64_t>> frameTimeList;

    // CPU timer
    std::chrono::time_point<std::chrono::system_clock> startTime;
};

}

#endif /* TIMERGL_HPP_ */
