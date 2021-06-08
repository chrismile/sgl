/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Christoph Neuhauser
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

#include <iostream>
#include <Utils/File/Logfile.hpp>
#include "../Utils/Device.hpp"
#include "../Utils/Swapchain.hpp"
#include "../Render/Renderer.hpp"
#include "TimerVk.hpp"

namespace sgl { namespace vk {

TimerVk::TimerVk(Renderer* renderer) : renderer(renderer), device(renderer->getDevice()) {
    const VkPhysicalDeviceLimits& limits = renderer->getDevice()->getPhysicalDeviceProperties().limits;
    if (!limits.timestampComputeAndGraphics) {
        Logfile::get()->throwError("Error in TimerVk::TimerVk: Device does not support timestamps.");
    }

    VkQueryPoolCreateInfo queryPoolCreateInfo = {};
    queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    queryPoolCreateInfo.queryCount = maxNumQueries;
    vkCreateQueryPool(device->getVkDevice(), &queryPoolCreateInfo, nullptr, &queryPool);

    queryBuffer = new uint64_t[maxNumQueries];

    // Nanoseconds per timestamp step.
    timestampPeriod = double(limits.timestampPeriod);
}

TimerVk::~TimerVk() {
    clear();
    vkDestroyQueryPool(device->getVkDevice(), queryPool, nullptr);
    delete[] queryBuffer;
}

void TimerVk::startGPU(const std::string& eventName) {
    Swapchain* swapchain = AppSettings::get()->getSwapchain();
    uint32_t frameIdx = swapchain ? swapchain->getImageIndex() : 0;

    if (frameIdx >= frameData.size()) {
        frameData.resize(frameIdx + 1);
    }

    FrameData& currentFrameData = frameData.at(frameIdx);
    auto it = currentFrameData.queryStartIndices.find(eventName);

    if (it != currentFrameData.queryStartIndices.end()) {
        if (frameIdx == 0) {
            currentQueryIdx = 0;
        }

        if (currentFrameData.numQueries != 0) {
            addTimesForFrame(frameIdx);
        }

        currentFrameData.queryStartIndices.clear();
        currentFrameData.queryEndIndices.clear();
        currentFrameData.queryStart = currentQueryIdx;
        currentFrameData.numQueries = 0;
    }

    vkCmdWriteTimestamp(
            renderer->getVkCommandBuffer(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            queryPool, currentQueryIdx);
    currentFrameData.queryStartIndices[eventName] = currentQueryIdx;
    currentFrameData.numQueries++;
    currentQueryIdx++;
}

void TimerVk::endGPU(const std::string& eventName) {
    Swapchain* swapchain = AppSettings::get()->getSwapchain();
    uint32_t frameIdx = swapchain ? swapchain->getImageIndex() : 0;

    FrameData& currentFrameData = frameData.at(frameIdx);
    vkCmdWriteTimestamp(
            renderer->getVkCommandBuffer(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            queryPool, currentQueryIdx);
    currentFrameData.queryStartIndices[eventName] = currentQueryIdx;
    currentFrameData.numQueries++;
    currentQueryIdx++;
}

void TimerVk::addTimesForFrame(uint32_t frameIdx) {
    FrameData& currentFrameData = frameData.at(frameIdx);

    vkGetQueryPoolResults(
            device->getVkDevice(), queryPool, currentFrameData.queryStart, currentFrameData.numQueries,
            currentFrameData.numQueries * sizeof(uint64_t), queryBuffer, sizeof(uint64_t),
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

    for (auto startIt = currentFrameData.queryStartIndices.begin();
            startIt != currentFrameData.queryStartIndices.end(); startIt++) {
        auto endIt = currentFrameData.queryEndIndices.find(startIt->first);
        if (endIt == currentFrameData.queryEndIndices.end()) {
            Logfile::get()->throwError(
                    "Error in TimerVk::addTimesForFrame: No call to 'end' for event \"" + startIt->first + "\".");
        }
        uint64_t startTimestamp = queryBuffer[startIt->second];
        uint64_t endTimestamp = queryBuffer[endIt->second];
        elapsedTimeNS[startIt->first] +=
                uint64_t(std::round(double(endTimestamp - startTimestamp) * timestampPeriod));
        numSamples[startIt->first] += 1;
    }

    vkCmdResetQueryPool(
            renderer->getVkCommandBuffer(), queryPool,
            currentFrameData.queryStart, currentFrameData.numQueries);
}

void TimerVk::startCPU(const std::string& eventName) {
    startTimesCPU[eventName] = std::chrono::system_clock::now();
}

void TimerVk::endCPU(const std::string& eventName) {
    auto startTimestamp = startTimesCPU[eventName];
    auto endTimestamp = std::chrono::system_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::nanoseconds>(endTimestamp - startTimestamp);
    uint64_t elapsedTimeNanoseconds = elapsedTime.count();
    elapsedTimeNS[eventName] += elapsedTimeNanoseconds;
    numSamples[eventName] += 1;
}

void TimerVk::finishGPU() {
    for (size_t frameIdx = 0; frameIdx < frameData.size(); frameIdx++) {
        FrameData& currentFrameData = frameData.at(frameIdx);

        if (currentFrameData.numQueries != 0) {
            addTimesForFrame(frameIdx);
        }

        currentFrameData.queryStartIndices.clear();
        currentFrameData.queryEndIndices.clear();
        currentFrameData.queryStart = 0;
        currentFrameData.numQueries = 0;
    }
}

void TimerVk::clear() {
    elapsedTimeNS.clear();
    numSamples.clear();
}

double TimerVk::getTimeMS(const std::string &name) {
    return static_cast<double>(elapsedTimeNS[name]) / static_cast<double>(numSamples[name]) * 1e-6;
}

void TimerVk::printTimeMS(const std::string &name) {
    double timeMS = getTimeMS(name);
    std::cout << "TIMER - " << name << ": " << timeMS << "ms" << std::endl;
}

void TimerVk::printTotalAvgTime() {
    double timeMS = 0.0;
    for (auto& it : numSamples) {
        timeMS += static_cast<double>(elapsedTimeNS[it.first]) / static_cast<double>(it.second) * 1e-6;
    }
    std::cout << "TOTAL TIME (avg): " << timeMS << "ms" << std::endl;
}

}}
