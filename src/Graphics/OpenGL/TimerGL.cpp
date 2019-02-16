/**
 * @file TimerGL.cpp
 *
 *  Created on: 20.08.2018
 *      Author: Christoph Neuhauser
 */

#include "TimerGL.hpp"
#include <iostream>
#include <GL/glew.h>

namespace sgl
{

TimerGL::~TimerGL()
{
    deleteAll();
}


void TimerGL::start(const std::string &name)
{
    size_t index = numSamples.size();
    auto it = regionNameMap.find(name);
    if (it == regionNameMap.end()) {
        // Create a new query & add the data of a new region
        GLuint queryID = 0;
        glGenQueries(1, &queryID);
        regionNameMap.insert(make_pair(name, index));
        queryIDs.push_back(queryID);
        elapsedTimeNS.push_back(0);
        numSamples.push_back(0);
        queryHasFinished.push_back(false);
    } else {
        // Add time to already stored event of last frame
        index = it->second;
        addQueryTime(index);
    }

    lastIndex = index;
    glBeginQuery(GL_TIME_ELAPSED, queryIDs.at(index));
}


void TimerGL::end()
{
    glEndQuery(GL_TIME_ELAPSED);
    queryHasFinished.at(lastIndex) = true;
}

void TimerGL::addQueryTime(size_t index)
{
    GLuint64 timer;
    glGetQueryObjectui64v(queryIDs.at(index), GL_QUERY_RESULT, &timer);
    elapsedTimeNS.at(index) += timer;
    numSamples.at(index) += 1;
    queryHasFinished.at(index) = false;
}


void TimerGL::deleteAll()
{
    size_t n = numSamples.size();
    for (size_t i = 0; i < n; i++) {
        glDeleteQueries(1, &queryIDs.at(i));
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
        // Add time to already stored event of last frame
        addQueryTime(index);
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
