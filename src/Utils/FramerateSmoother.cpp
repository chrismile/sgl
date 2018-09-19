//
// Created by christoph on 19.09.18.
//

#include <algorithm>
#include "FramerateSmoother.hpp"

FramerateSmoother::FramerateSmoother(int numSamples) : numSamples(numSamples)
{
    samples.resize(numSamples, 60.0f);
    writeIndex = 0;
}

void FramerateSmoother::addSample(float fps)
{
    samples.at(writeIndex) = fps;
    writeIndex = (writeIndex+1) % numSamples;
}

float FramerateSmoother::computeAverage()
{
    float avg = 0.0f;
    for (float sample : samples) {
        avg += sample;
    }
    avg /= numSamples;
    return avg;
}

float FramerateSmoother::computeMedian()
{
    std::vector<float> sortedSamples = samples;
    std::sort(sortedSamples.begin(), sortedSamples.end());
    return sortedSamples.at(numSamples/2);
}
