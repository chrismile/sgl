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

#include "SyncObjects.hpp"

namespace sgl { namespace vk {

Semaphore::Semaphore(
        sgl::vk::Device* device, VkSemaphoreCreateFlags semaphoreCreateFlags,
        VkSemaphoreType semaphoreType, uint64_t timelineSemaphoreInitialValue) {
    _initialize(device, semaphoreCreateFlags, semaphoreType, timelineSemaphoreInitialValue);
}

Semaphore::~Semaphore() {
    vkDestroySemaphore(device->getVkDevice(), semaphoreVk, nullptr);
}

void Semaphore::_initialize(
        Device* device, VkSemaphoreCreateFlags semaphoreCreateFlags,
        VkSemaphoreType semaphoreType, uint64_t timelineSemaphoreInitialValue,
        void* next) {
    this->device = device;
    VkSemaphoreTypeCreateInfo timelineSemaphoreCreateInfo{};
    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCreateInfo.flags = semaphoreCreateFlags;
    if (semaphoreType == VK_SEMAPHORE_TYPE_TIMELINE) {
        timelineSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        timelineSemaphoreCreateInfo.pNext = next;
        timelineSemaphoreCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        timelineSemaphoreCreateInfo.initialValue = timelineSemaphoreInitialValue;
        semaphoreCreateInfo.pNext = &timelineSemaphoreCreateInfo;
    } else {
        semaphoreCreateInfo.pNext = next;
    }

    if (vkCreateSemaphore(
            device->getVkDevice(), &semaphoreCreateInfo, nullptr, &semaphoreVk) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in Semaphore::_initialize: Failed to create a Vulkan semaphore!");
    }
}

void Semaphore::signalSemaphoreVk(uint64_t timelineValue) {
    VkSemaphoreSignalInfo semaphoreSignalInfo = {};
    semaphoreSignalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
    semaphoreSignalInfo.semaphore = semaphoreVk;
    semaphoreSignalInfo.value = timelineValue;
    vkSignalSemaphore(device->getVkDevice(), &semaphoreSignalInfo);
}

void Semaphore::waitSemaphoreVk(uint64_t timelineValue) {
    VkSemaphoreWaitInfo semaphoreWaitInfo = {};
    semaphoreWaitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    semaphoreWaitInfo.semaphoreCount = 1;
    semaphoreWaitInfo.pSemaphores = &semaphoreVk;
    semaphoreWaitInfo.pValues = &timelineValue;
    vkWaitSemaphores(device->getVkDevice(), &semaphoreWaitInfo, 0);
}


Fence::Fence(Device* device, VkFenceCreateFlags fenceCreateFlags) : device(device) {
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = fenceCreateFlags;

    if (vkCreateFence(
            device->getVkDevice(), &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
        sgl::Logfile::get()->throwError("Error in Fence::Fence: Could not create a Vulkan fence.");
    }
}

Fence::~Fence() {
    vkDestroyFence(device->getVkDevice(), fence, nullptr);
}

bool Fence::wait(uint64_t timeoutNanoseconds) {
    VkResult result = vkWaitForFences(
            device->getVkDevice(), 1, &fence, VK_TRUE, timeoutNanoseconds);
    if (result == VK_SUCCESS) {
        return true;
    } else if (result == VK_TIMEOUT) {
        return false;
    } else {
        Logfile::get()->throwError("Error in Fence::wait: vkWaitForFences exited with an error code.");
        return false;
    }
}

void Fence::reset() {
    VkResult result = vkResetFences(device->getVkDevice(), 1, &fence);
    if (result != VK_SUCCESS) {
        Logfile::get()->throwError("Error in Fence::reset: vkResetFences exited with an error code.");
    }
}

}}
