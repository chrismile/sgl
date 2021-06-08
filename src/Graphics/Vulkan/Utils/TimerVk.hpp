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

#ifndef SGL_TIMERVK_HPP
#define SGL_TIMERVK_HPP

#include <map>
#include <chrono>
#include <vulkan/vulkan.h>

namespace sgl { namespace vk {

struct Buffer;
typedef std::shared_ptr<Buffer> BufferPtr;
struct Renderer;

class DLL_OBJECT TimerVk {
public:
    TimerVk(Renderer* renderer);
    ~TimerVk();

    /// Clears all stored queries.
    void clear();

    /**
     * Start measuring time for event with specified name on the GPU.
     * @param eventName The name of the event.
     */
    void startGPU(const std::string& eventName);
    /**
     * Stop measuring time for event with specified name on the GPU.
     * @param eventName The name of the event.
     */
    void endGPU(const std::string& eventName);

    /**
     * Start measuring time for event with specified name on the CPU.
     * @param eventName The name of the event.
     */
    void startCPU(const std::string& eventName);
    /**
     * Stop measuring time for event with specified name on the CPU.
     * @param eventName The name of the event.
     */
    void endCPU(const std::string& eventName);

    /**
     * Makes sure all time measurements finish on the GPU.
     * This function should be called before calling @see getTimeMS, @see printTimeMS or @see printTotalAvgTime.
     */
    void finishGPU();

    /// Get the (average) time the event with the specified name took.
    double getTimeMS(const std::string& eventName);
    /// Prints the time returned by @see getTimeMS.
    void printTimeMS(const std::string& eventName);
    // Prints sum of all average times.
    void printTotalAvgTime();

private:
    void addTimesForFrame(uint32_t frameIdx);

    Renderer* renderer;
    Device* device;

    VkQueryPool queryPool;

    const uint32_t maxNumQueries = 100;
    uint32_t currentQueryIdx = 0;
    uint64_t* queryBuffer = nullptr;
    double timestampPeriod = 0.0;

    // Data per-frame (as one should not sync while swapchain images are still unprocessed).
    struct FrameData {
        std::map<std::string, uint32_t> queryStartIndices;
        std::map<std::string, uint32_t> queryEndIndices;
        uint32_t queryStart = 0;
        uint32_t numQueries = 0;
    };
    std::vector<FrameData> frameData;

    /// The accumulated time each event took
    std::map<std::string, uint64_t> elapsedTimeNS;
    /// The number of measurements of each event (for computing average)
    std::map<std::string, uint64_t> numSamples;

    // CPU timer
    std::map<std::string, std::chrono::time_point<std::chrono::system_clock>> startTimesCPU;
};

typedef std::shared_ptr<RayTracingPipeline> RayTracingPipelinePtr;

}}

#endif //SGL_TIMERVK_HPP
