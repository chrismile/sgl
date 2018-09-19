//
// Created by christoph on 19.09.18.
//

#ifndef SGL_FRAMERATESMOOTHER_HPP
#define SGL_FRAMERATESMOOTHER_HPP

#include <vector>

class FramerateSmoother
{
public:
    FramerateSmoother(int numSamples);
    void addSample(float fps);
    float computeAverage();
    float computeMedian();

private:
    int numSamples;
    std::vector<float> samples;
    int writeIndex;
};


#endif //SGL_FRAMERATESMOOTHER_HPP
