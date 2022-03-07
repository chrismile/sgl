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

#include <memory>
#include <set>
#include <iostream>

#include <Utils/File/Logfile.hpp>
#include <Utils/Events/EventManager.hpp>

#include <Graphics/Window.hpp>
#include <SDL/SDLWindow.hpp>

#include "../Render/CommandBuffer.hpp"
#include "Device.hpp"
#include "Swapchain.hpp"

namespace sgl { namespace vk {

Swapchain::Swapchain(Device* device, bool useClipping) : device(device), useClipping(useClipping) {
}

Swapchain::~Swapchain() {
    cleanup();
}

void Swapchain::create(Window* window) {
    this->window = window;
    VkSurfaceKHR surface = window->getVkSurface();
    WindowSettings windowSettings = window->getWindowSettings();

    SwapchainSupportInfo swapchainSupportInfo = querySwapchainSupportInfo(device->getVkPhysicalDevice(), surface);
    std::set<VkPresentModeKHR> presentModesSet(
            swapchainSupportInfo.presentModes.begin(), swapchainSupportInfo.presentModes.end());
    VkSurfaceFormatKHR surfaceFormat = getSwapSurfaceFormat(swapchainSupportInfo.formats);
    VkPresentModeKHR presentMode;
    if (!windowSettings.vSync || windowSettings.vSyncMode == VSYNC_MODE_IMMEDIATE) {
        presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    } else if (!windowSettings.vSync || windowSettings.vSyncMode == VSYNC_MODE_FIFO) {
        presentMode = VK_PRESENT_MODE_FIFO_KHR;
    } else if (!windowSettings.vSync || windowSettings.vSyncMode == VSYNC_MODE_FIFO_RELAXED) {
        presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    } else {
        presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    }
    if (presentModesSet.find(presentMode) == presentModesSet.end()) {
        presentMode = VK_PRESENT_MODE_FIFO_KHR;
    }
    swapchainExtent = swapchainSupportInfo.capabilities.currentExtent;
    swapchainImageFormat = surfaceFormat.format;

    // vulkan-tutorial.com recommends to use min + 1 (usually triple buffering).
    uint32_t imageCount = swapchainSupportInfo.capabilities.minImageCount + 1;
    const uint32_t maxImageCount = swapchainSupportInfo.capabilities.maxImageCount;
    if (maxImageCount > 0 && imageCount > maxImageCount) {
        imageCount = swapchainSupportInfo.capabilities.maxImageCount;
    }
    minImageCount = swapchainSupportInfo.capabilities.minImageCount;

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = swapchainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
    createInfo.preTransform = swapchainSupportInfo.capabilities.currentTransform;
    // VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = useClipping ? VK_TRUE : VK_FALSE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device->getVkDevice(), &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
        sgl::Logfile::get()->writeError("Error in Swapchain::create: Could not create a swapchain.");
        exit(1);
    }

    createSwapchainImages();
    createSwapchainImageViews();
    if (createFirstTime) {
        createSyncObjects();
        createFirstTime = false;
    }
}

void Swapchain::createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(swapchainImages.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(
                device->getVkDevice(), &semaphoreInfo, nullptr,
                &imageAvailableSemaphores[i]) != VK_SUCCESS
            || vkCreateSemaphore(
                    device->getVkDevice(), &semaphoreInfo, nullptr,
                    &renderFinishedSemaphores[i]) != VK_SUCCESS) {
            sgl::Logfile::get()->throwError("Error in Swapchain::createSyncObjects: Could not create semaphores.");
        }
        if (vkCreateFence(
                device->getVkDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            sgl::Logfile::get()->throwError("Error in Swapchain::createSyncObjects: Could not create fence.");
        }
    }
}

void Swapchain::createSwapchainImages() {
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(device->getVkDevice(), swapchain, &imageCount, nullptr);
    std::vector<VkImage> swapchainVkImages;
    swapchainVkImages.resize(imageCount);
    swapchainImages.reserve(imageCount);
    vkGetSwapchainImagesKHR(device->getVkDevice(), swapchain, &imageCount, swapchainVkImages.data());

    ImageSettings imageSettings;
    imageSettings.width = swapchainExtent.width;
    imageSettings.height = swapchainExtent.height;
    imageSettings.format = swapchainImageFormat;
    for (VkImage image : swapchainVkImages) {
        swapchainImages.push_back(std::make_shared<Image>(device, imageSettings, image, false));
    }
}

void Swapchain::createSwapchainImageViews() {
    swapchainImageViews.reserve(swapchainImages.size());
    for (size_t i = 0; i < swapchainImages.size(); i++) {
        swapchainImageViews.push_back(std::make_shared<ImageView>(
                swapchainImages.at(i), VK_IMAGE_ASPECT_COLOR_BIT));
    }
}

void Swapchain::recreateSwapchain() {
    int windowWidth = 0;
    int windowHeight = 0;
    SDLWindow* sdlWindow = static_cast<SDLWindow*>(window);
    SDL_Window* sdlWindowData = sdlWindow->getSDLWindow();
    isWaitingForResizeEnd = true;
    while (windowWidth == 0 || windowHeight == 0) {
        SDL_Vulkan_GetDrawableSize(sdlWindowData, &windowWidth, &windowHeight);
        window->processEvents();
    }
    isWaitingForResizeEnd = false;

    cleanupRecreate();
    create(window);

    // Recreate framebuffer, pipeline, ...
    // For the moment, a resolution changed event is additionally triggered to be compatible with OpenGL.
    EventManager::get()->triggerEvent(std::make_shared<Event>(RESOLUTION_CHANGED_EVENT));
}

void Swapchain::beginFrame() {
    vkWaitForFences(device->getVkDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(
            device->getVkDevice(), swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
            VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        sgl::Logfile::get()->writeError("Error in Swapchain::renderFrame: Failed to acquire swapchain image!");
    }

    // Image already in use?
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device->getVkDevice(), 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Mark as in use.
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    vkResetFences(device->getVkDevice(), 1, &inFlightFences[currentFrame]);
}

void Swapchain::renderFrame(const std::vector<VkCommandBuffer>& commandBuffers) {
    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = uint32_t(commandBuffers.size());
    submitInfo.pCommandBuffers = commandBuffers.data();
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    if (vkQueueSubmit(device->getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        sgl::Logfile::get()->throwError("Error in Swapchain::renderFrame: Could not submit to the graphics queue.");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = { swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;
    VkResult result = vkQueuePresentKHR(device->getGraphicsQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
    } else if (result != VK_SUCCESS) {
        sgl::Logfile::get()->writeError("Error in Swapchain::renderFrame: Failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Swapchain::renderFrame(const std::vector<sgl::vk::CommandBufferPtr>& commandBuffers) {
    if (commandBuffers.empty()) {
        sgl::Logfile::get()->throwError("Error in Swapchain::renderFrame: Command buffer array empty!");
    }

    sgl::vk::CommandBufferPtr commandBufferFirst = commandBuffers.front();
    sgl::vk::CommandBufferPtr commandBufferLast = commandBuffers.back();

    //VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };

    // The wait semaphore is added by sgl::vk::Renderer to ensure it GPU-CPU syncing is possible.
    //commandBufferFirst->pushWaitSemaphore(waitSemaphores[0]);
    commandBufferLast->pushSignalSemaphore(signalSemaphores[0]);

    std::vector<uint64_t> waitSemaphoreValues;
    std::vector<uint64_t> signalSemaphoreValues;

    for (size_t cmdBufIdx = 0; cmdBufIdx < commandBuffers.size(); cmdBufIdx++) {
        const sgl::vk::CommandBufferPtr& commandBuffer = commandBuffers.at(cmdBufIdx);
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkTimelineSemaphoreSubmitInfo timelineSubmitInfo{};
        timelineSubmitInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;

        if (commandBuffer->hasWaitTimelineSemaphore()) {
            waitSemaphoreValues = commandBuffer->getWaitSemaphoreValues();
            submitInfo.pNext = &timelineSubmitInfo;
            timelineSubmitInfo.waitSemaphoreValueCount = uint32_t(waitSemaphoreValues.size());
            timelineSubmitInfo.pWaitSemaphoreValues = waitSemaphoreValues.data();
        }
        if (commandBuffer->hasSignalTimelineSemaphore()) {
            signalSemaphoreValues = commandBuffer->getSignalSemaphoreValues();
            submitInfo.pNext = &timelineSubmitInfo;
            timelineSubmitInfo.signalSemaphoreValueCount = uint32_t(signalSemaphoreValues.size());
            timelineSubmitInfo.pSignalSemaphoreValues = signalSemaphoreValues.data();
        }

        submitInfo.waitSemaphoreCount = uint32_t(commandBuffer->getWaitSemaphoresVk().size());
        submitInfo.pWaitSemaphores = commandBuffer->getWaitSemaphoresVk().data();
        submitInfo.pWaitDstStageMask = commandBuffer->getWaitDstStageMasks().data();
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = commandBuffer->getVkCommandBufferPtr();
        submitInfo.signalSemaphoreCount = uint32_t(commandBuffer->getSignalSemaphoresVk().size());
        submitInfo.pSignalSemaphores = commandBuffer->getSignalSemaphoresVk().data();

        VkFence fence;
        if (cmdBufIdx == commandBuffers.size() - 1) {
            fence = inFlightFences[currentFrame];
            assert(commandBuffer->getVkFence() == VK_NULL_HANDLE);
        } else {
            fence = commandBuffer->getVkFence();
        }
        if (vkQueueSubmit(device->getGraphicsQueue(), 1, &submitInfo, fence) != VK_SUCCESS) {
            sgl::Logfile::get()->throwError(
                    "Error in Swapchain::renderFrame: Could not submit to the graphics queue.");
        }

        commandBuffer->_clearSyncObjects();
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = { swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;
    VkResult result = vkQueuePresentKHR(device->getGraphicsQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        recreateSwapchain();
    } else if (result != VK_SUCCESS) {
        sgl::Logfile::get()->writeError("Error in Swapchain::renderFrame: Failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Swapchain::cleanupRecreate() {
    vkDeviceWaitIdle(device->getVkDevice());

    swapchainFramebuffers.clear();
    swapchainImages.clear();
    swapchainImageViews.clear();
    vkDestroySwapchainKHR(device->getVkDevice(), swapchain, nullptr);
}

void Swapchain::cleanup() {
    cleanupRecreate();

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device->getVkDevice(), renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device->getVkDevice(), imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device->getVkDevice(), inFlightFences[i], nullptr);
    }
}

SwapchainSupportInfo querySwapchainSupportInfo(VkPhysicalDevice device, VkSurfaceKHR surface) {
    SwapchainSupportInfo swapchainSupportInfo;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapchainSupportInfo.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        swapchainSupportInfo.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapchainSupportInfo.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        swapchainSupportInfo.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
                device, surface, &presentModeCount, swapchainSupportInfo.presentModes.data());
    }

    return swapchainSupportInfo;
}

VkSurfaceFormatKHR Swapchain::getSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    for (const auto &availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM
                && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR Swapchain::getSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

}}
