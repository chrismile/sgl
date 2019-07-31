/**
 * @file TimerGL.cpp
 *
 *  Created on: 20.08.2018
 *      Author: Christoph Neuhauser
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
        GLuint queryID = 0;
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
