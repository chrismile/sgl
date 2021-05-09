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

#include <cassert>
#include <iostream>

#include <GL/glew.h>

#include "TimerGL.hpp"

namespace sgl
{

TimerGL::~TimerGL()
{
    deleteAll();
}


void TimerGL::startGPU(const std::string &name, float timeStamp)
{
    size_t index = numSamples.size();
    auto it = regionNameMap.find(name);
    if (it == regionNameMap.end()) {
        // Create a new query & add the data of a new region
        GLuint queryID = 0;
        glGenQueries(1, &queryID);
        regionNameMap.insert(make_pair(name, index));
        queryIDs.push_back(queryID);
        isGPUQuery.push_back(true);
        elapsedTimeNS.push_back(0);
        numSamples.push_back(0);
        queryHasFinished.push_back(false);
        frameTimeList.clear();
    } else {
        // Add time to already stored event of last frame
        index = it->second;
        addQueryTime(index, lastTimeStamp);
    }

    lastIndex = index;
    lastTimeStamp = timeStamp;
    glBeginQuery(GL_TIME_ELAPSED, queryIDs.at(index));
}

void TimerGL::startCPU(const std::string &name, float timeStamp)
{
    size_t index = numSamples.size();
    auto it = regionNameMap.find(name);
    if (it == regionNameMap.end()) {
        // Create a new query & add the data of a new region
        regionNameMap.insert(make_pair(name, index));
        queryIDs.push_back(0); // Just for GPU, add any value.
        isGPUQuery.push_back(false);
        elapsedTimeNS.push_back(0);
        numSamples.push_back(0);
        queryHasFinished.push_back(false);
        frameTimeList.clear();
    } else {
        index = it->second;
    }

    lastIndex = index;
    lastTimeStamp = timeStamp;
    startTime = std::chrono::system_clock::now();
}


void TimerGL::end()
{
    if (isGPUQuery.at(lastIndex)) {
        glEndQuery(GL_TIME_ELAPSED);
        queryHasFinished.at(lastIndex) = true;
    } else {
        addQueryTime(lastIndex, lastTimeStamp);
    }
}

void TimerGL::stopMeasuring()
{
    if (isGPUQuery.at(lastIndex)) {
        assert(queryHasFinished.at(lastIndex));
        addQueryTime(lastIndex, lastTimeStamp);
        queryHasFinished.at(lastIndex) = false;
    }
}

void TimerGL::addQueryTime(size_t index, float timeStamp)
{
    if (isGPUQuery.at(index)) {
        GLuint64 timer;
        glGetQueryObjectui64v(queryIDs.at(index), GL_QUERY_RESULT, &timer);
        elapsedTimeNS.at(index) += timer;
        numSamples.at(index) += 1;
        queryHasFinished.at(index) = false;
        frameTimeList.push_back(std::make_pair(timeStamp, timer));
    } else {
        auto endTime = std::chrono::system_clock::now();
        auto elapsedTime = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
        uint64_t timer = elapsedTime.count();
        elapsedTimeNS.at(index) += timer;
        numSamples.at(index) += 1;
        frameTimeList.push_back(std::make_pair(timeStamp, timer));
    };
}


void TimerGL::deleteAll()
{
    size_t n = numSamples.size();
    for (size_t i = 0; i < n; i++) {
        if (isGPUQuery.at(i)) {
            glDeleteQueries(1, &queryIDs.at(i));
        }
    }

    regionNameMap.clear();
    queryIDs.clear();
    elapsedTimeNS.clear();
    numSamples.clear();
}


double TimerGL::getTimeMS(const std::string &name)
{
    auto it = regionNameMap.find(name);
    if (it == regionNameMap.end()) {
        std::cerr << "Invalid name in TimerGL::getTimeMS" << std::endl;
        return 0.0;
    }
    int index = it->second;
    if (queryHasFinished.at(index)) {
        addQueryTime(index, lastTimeStamp);
    }
    return static_cast<double>(elapsedTimeNS.at(index)) / static_cast<double>(numSamples.at(index)) * 1e-6;
}


void TimerGL::printTimeMS(const std::string &name)
{
    double timeMS = getTimeMS(name);
    std::cout << "TIMER - " << name << ": " << timeMS << "ms" << std::endl;
}


void TimerGL::printTotalAvgTime()
{
    double timeMS = 0.0;
    for (auto it = regionNameMap.begin(); it != regionNameMap.end(); it++) {
        int index = it->second;
        timeMS += static_cast<double>(elapsedTimeNS.at(index)) / static_cast<double>(numSamples.at(index)) * 1e-6;
    }
    std::cout << "TOTAL TIME (avg): " << timeMS << "ms" << std::endl;
}

}
