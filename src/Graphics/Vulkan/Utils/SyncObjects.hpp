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

#ifndef SGL_SYNCOBJECTS_HPP
#define SGL_SYNCOBJECTS_HPP

#include <Utils/File/Logfile.hpp>
#include "Device.hpp"

namespace sgl { namespace vk {

class Semaphore;
typedef std::shared_ptr<Semaphore> SemaphorePtr;
class Fence;
typedef std::shared_ptr<Fence> FencePtr;

class DLL_OBJECT Semaphore {
public:
    explicit Semaphore(Device* device, VkSemaphoreCreateFlags semaphoreCreateFlags = 0);
    ~Semaphore();

    void signalSemaphoreVk();
    void waitSemaphoreVk();

    inline VkSemaphore getVkSemaphore() const { return semaphoreVk; }

protected:
    Semaphore() = default;
    void _initialize(Device* device, VkSemaphoreCreateFlags semaphoreCreateFlags, void* next = nullptr);

    sgl::vk::Device* device = nullptr;
    VkSemaphore semaphoreVk = VK_NULL_HANDLE;
};

class DLL_OBJECT Fence {
public:
    explicit Fence(Device* device, VkFenceCreateFlags fenceCreateFlags = VK_FENCE_CREATE_SIGNALED_BIT);
    ~Fence();

    /**
     * Waits for the fence to become signaled.
     * @param timeoutNanoseconds The number of nanoseconds to wait before returning if the fence doesn't become
     * signaled.
     * @return True if the fence became signaled, and false if a timeout occurred.
     */
    bool wait(uint64_t timeoutNanoseconds);
    void reset();

    inline VkFence getVkFence() { return fence; }

private:
    sgl::vk::Device* device = nullptr;
    VkFence fence = VK_NULL_HANDLE;
};

}}

#endif //SGL_SYNCOBJECTS_HPP
