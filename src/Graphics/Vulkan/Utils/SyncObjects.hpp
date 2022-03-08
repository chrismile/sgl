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

#ifdef _WIN32
typedef void* HANDLE;
#endif

namespace sgl { namespace vk {

class Semaphore;
typedef std::shared_ptr<Semaphore> SemaphorePtr;
class Fence;
typedef std::shared_ptr<Fence> FencePtr;

class DLL_OBJECT Semaphore {
public:
    /**
     * @param device The device associated with the semaphore.
     * @param semaphoreCreateFlags The semaphore creation flags.
     * @param semaphoreType VK_SEMAPHORE_TYPE_BINARY or VK_SEMAPHORE_TYPE_TIMELINE.
     * @param timelineSemaphoreInitialValue If semaphoreType is set to VK_SEMAPHORE_TYPE_TIMELINE
     */
    explicit Semaphore(
            Device* device, VkSemaphoreCreateFlags semaphoreCreateFlags = 0,
            VkSemaphoreType semaphoreType = VK_SEMAPHORE_TYPE_BINARY, uint64_t timelineSemaphoreInitialValue = 0);
    virtual ~Semaphore();

    [[nodiscard]] inline VkSemaphore getVkSemaphore() const { return semaphoreVk; }
    [[nodiscard]] inline VkSemaphoreType getVkSemaphoreType() const { return semaphoreType; }
    [[nodiscard]] inline bool isBinarySemaphore() const { return semaphoreType == VK_SEMAPHORE_TYPE_BINARY; }
    [[nodiscard]] inline bool isTimelineSemaphore() const { return semaphoreType == VK_SEMAPHORE_TYPE_TIMELINE; }

    // --- For timeline semaphores. ---
    void waitSemaphoreVk(uint64_t timelineValue);
    void signalSemaphoreVk(uint64_t timelineValue);
    uint64_t getSemaphoreCounterValue();
    [[nodiscard]] inline uint64_t getWaitSemaphoreValue() const { return waitSemaphoreValue; }
    [[nodiscard]] inline uint64_t getSignalSemaphoreValue() const { return signalSemaphoreValue; }
    inline void setWaitSemaphoreValue(uint64_t value) { waitSemaphoreValue = value; }
    inline void setSignalSemaphoreValue(uint64_t value) { signalSemaphoreValue = value; }

#ifdef _WIN32
    /**
     * Imports a Direct3D 12 fence shared resource handle. The object will take ownership of the handle and close it on
     * destruction. Below, an example of how to create the shared handle can be found.
     *
     * HANDLE resourceHandle;
     * std::wstring sharedHandleNameString = std::wstring(L"Local\\D3D12ResourceHandle") + std::to_wstring(resourceIdx);
     * ID3D12Device::CreateSharedHandle(dxObject, nullptr, GENERIC_ALL, sharedHandleNameString.data(), &resourceHandle);
     */
    void importD3D12SharedResourceHandle(HANDLE resourceHandle);
#endif

protected:
    Semaphore() = default;
    void _initialize(
            Device* device, VkSemaphoreCreateFlags semaphoreCreateFlags,
            VkSemaphoreType semaphoreType, uint64_t timelineSemaphoreInitialValue,
            void* next = nullptr);

    sgl::vk::Device* device = nullptr;
    VkSemaphore semaphoreVk = VK_NULL_HANDLE;
    VkSemaphoreType semaphoreType = VK_SEMAPHORE_TYPE_BINARY;

    // --- Timeline semaphore data ---
    uint64_t waitSemaphoreValue = 0;
    uint64_t signalSemaphoreValue = 0;

#ifdef _WIN32
    HANDLE handle = nullptr;
#else
    int fileDescriptor = -1;
#endif
};

class DLL_OBJECT Fence {
public:
    explicit Fence(Device* device, VkFenceCreateFlags fenceCreateFlags = 0);
    ~Fence();

    /**
     * Waits for the fence to become signaled.
     * @param timeoutNanoseconds The number of nanoseconds to wait before returning if the fence doesn't become
     * signaled.
     * @return True if the fence became signaled, and false if a timeout occurred.
     */
    bool wait(uint64_t timeoutNanoseconds = UINT64_MAX);
    void reset();

    inline VkFence getVkFence() { return fence; }

private:
    sgl::vk::Device* device = nullptr;
    VkFence fence = VK_NULL_HANDLE;
};

}}

#endif //SGL_SYNCOBJECTS_HPP
