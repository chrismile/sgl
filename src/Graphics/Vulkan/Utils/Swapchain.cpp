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

#include <Utils/StringUtils.hpp>
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
    VkSurfaceKHR surface = {};
    if (window) {
        surface = window->getVkSurface();
    }
    WindowSettings windowSettings = window->getWindowSettings();

    auto* sdlWindow = static_cast<SDLWindow*>(window);
    useDownloadSwapchain = sdlWindow->getUseDownloadSwapchain();
    if (useDownloadSwapchain) {
        SDL_Window* sdlWindowData = sdlWindow->getSDLWindow();
        window->errorCheck();
        cpuSurface = (void*)SDL_GetWindowSurface(sdlWindowData);
        if (createFirstTime) {
            /*
             * For some reason, this triggers SDL_Unsupported when called for the first time on a system using xrdp.
             * I tried not calling SDL_GetWindowSurface after initial window creation, but then no resize events are triggered.
             * SDL_HasWindowSurface in case of SDL_VERSION_ATLEAST(2, 28, 0) should thus also not help in this use-case.
             */
            while (SDL_GetError()[0] != '\0') {
                std::string errorString = SDL_GetError();
                bool openMessageBox = true;
                if (sgl::stringContains(errorString, "That operation is not supported")) {
                    openMessageBox = false;
                }
                Logfile::get()->writeError(std::string() + "SDL error: " + errorString, openMessageBox);
                SDL_ClearError();
            }
        }
        swapchainExtent = { uint32_t(sdlWindow->getWidth()), uint32_t(sdlWindow->getHeight()) };
        swapchainImageFormat = VK_FORMAT_R8G8B8A8_UNORM;
        minImageCount = 1;

        // Create images.
        swapchainImages.reserve(1);
        swapchainImageViews.reserve(1);
        ImageSettings imageSettings;
        imageSettings.width = swapchainExtent.width;
        imageSettings.height = swapchainExtent.height;
        imageSettings.format = swapchainImageFormat;
        imageSettings.usage =
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageSettings.memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
        swapchainImages.push_back(std::make_shared<Image>(device, imageSettings));
        swapchainImageViews.push_back(std::make_shared<ImageView>(
                swapchainImages.at(0), VK_IMAGE_ASPECT_COLOR_BIT));
        imageSettings.tiling = VK_IMAGE_TILING_LINEAR;
        imageSettings.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageSettings.memoryUsage = VMA_MEMORY_USAGE_GPU_TO_CPU;
        swapchainImageCpu = std::make_shared<Image>(device, imageSettings);
        swapchainImageCpu->transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // Create auxiliary data.
        if (!frameDownloadCommandBuffer) {
            vk::CommandPoolType commandPoolType;
            commandPoolType.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            frameDownloadCommandBuffer = std::make_shared<CommandBuffer>(device, commandPoolType);
            frameRenderedSemaphore = std::make_shared<Semaphore>(device);
            frameDownloadedFence = std::make_shared<Fence>(device);
        }

        if (createFirstTime) {
            createSyncObjects();
            createFirstTime = false;
        }
        return;
    }

    SwapchainSupportInfo swapchainSupportInfo = querySwapchainSupportInfo(
            device->getVkPhysicalDevice(), surface, window);
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

    // On Wayland, the special value 0xFFFFFFFFu is used, as the window will adapt to the arbitrarily used size.
    if (window && (swapchainExtent.width == 0xFFFFFFFFu || swapchainExtent.height == 0xFFFFFFFFu)) {
        swapchainExtent.width = uint32_t(window->getWidth());
        swapchainExtent.height = uint32_t(window->getHeight());
    }

    // vulkan-tutorial.com recommends to use min + 1 (usually triple buffering).
    const uint32_t maxImageCount = swapchainSupportInfo.capabilities.maxImageCount;
    uint32_t imageCount = swapchainSupportInfo.capabilities.minImageCount + 1;
    if (maxImageCount > 0 && window->getUsesAnyWaylandBackend()) {
        imageCount = std::min(imageCount, uint32_t(3));
        imageCount = std::clamp(imageCount, swapchainSupportInfo.capabilities.minImageCount, maxImageCount);
    }
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

    VkResult retVal = vkCreateSwapchainKHR(device->getVkDevice(), &createInfo, nullptr, &swapchain);
    if (retVal != VK_SUCCESS) {
        std::string errorMessage = "Error in Swapchain::create: Could not create a swapchain.";
        if (retVal == VK_ERROR_OUT_OF_HOST_MEMORY) {
            errorMessage += " Error: VK_ERROR_OUT_OF_HOST_MEMORY.";
        } else if (retVal == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
            errorMessage += " Error: VK_ERROR_OUT_OF_DEVICE_MEMORY.";
        } else if (retVal == VK_ERROR_DEVICE_LOST) {
            errorMessage += " Error: VK_ERROR_DEVICE_LOST.";
        } else if (retVal == VK_ERROR_SURFACE_LOST_KHR) {
            errorMessage += " Error: VK_ERROR_SURFACE_LOST_KHR.";
        } else if (retVal == VK_ERROR_NATIVE_WINDOW_IN_USE_KHR) {
            errorMessage += " Error: VK_ERROR_NATIVE_WINDOW_IN_USE_KHR.";
        } else if (retVal == VK_ERROR_INITIALIZATION_FAILED) {
            errorMessage += " Error: VK_ERROR_INITIALIZATION_FAILED.";
        } else if (retVal == VK_ERROR_COMPRESSION_EXHAUSTED_EXT) {
            errorMessage += " Error: VK_ERROR_COMPRESSION_EXHAUSTED_EXT.";
        }
        sgl::Logfile::get()->writeError(errorMessage);
        exit(1);
    }

    createSwapchainImages();
    createSwapchainImageViews();
    if (createFirstTime) {
        createSyncObjects();
        createFirstTime = false;
    } else {
        imagesInFlight.clear();
        imagesInFlight.resize(swapchainImages.size(), VK_NULL_HANDLE);
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
    auto* sdlWindow = static_cast<SDLWindow*>(window);
    SDL_Window* sdlWindowData = sdlWindow->getSDLWindow();
    isWaitingForResizeEnd = true;
    useDownloadSwapchain = sdlWindow->getUseDownloadSwapchain();
    if (!useDownloadSwapchain) {
        while (windowWidth == 0 || windowHeight == 0) {
            SDL_Vulkan_GetDrawableSize(sdlWindowData, &windowWidth, &windowHeight);
            window->processEvents();
        }
    } else {
        while (windowWidth == 0 || windowHeight == 0) {
            windowWidth = window->getWidth();
            windowHeight = window->processEvents();
            window->processEvents();
        }
    }
    isWaitingForResizeEnd = false;

    cleanupRecreate();
    create(window);

    // Recreate framebuffer, pipeline, ...
    // For the moment, a resolution changed event is additionally triggered to be compatible with OpenGL.
    EventManager::get()->triggerEvent(std::make_shared<Event>(RESOLUTION_CHANGED_EVENT));
}

void Swapchain::beginFrame() {
    auto* sdlWindow = static_cast<SDLWindow*>(window);
    bool useDownloadSwapchain = sdlWindow->getUseDownloadSwapchain();
    if (useDownloadSwapchain) {
        return;
    }

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
    if (useDownloadSwapchain) {
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.pWaitSemaphores = {};
        submitInfo.pWaitDstStageMask = {};
        signalSemaphores[0] = frameRenderedSemaphore->getVkSemaphore();
    }
    VkFence fence = useDownloadSwapchain ? VK_NULL_HANDLE : inFlightFences[currentFrame];
    if (vkQueueSubmit(device->getGraphicsQueue(), 1, &submitInfo, fence) != VK_SUCCESS) {
        sgl::Logfile::get()->throwError("Error in Swapchain::renderFrame: Could not submit to the graphics queue.");
    }

    if (!useDownloadSwapchain) {
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
    } else {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(frameDownloadCommandBuffer->getVkCommandBuffer(), &beginInfo);
        swapchainImages.front()->copyToImage(
                swapchainImageCpu, VK_IMAGE_ASPECT_COLOR_BIT,
                frameDownloadCommandBuffer->getVkCommandBuffer());
        vkEndCommandBuffer(frameDownloadCommandBuffer->getVkCommandBuffer());

        VkCommandBuffer frameDownloadCommandBuffers[] = { frameDownloadCommandBuffer->getVkCommandBuffer() };
        waitSemaphores[0] = frameRenderedSemaphore->getVkSemaphore();
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        waitStages[0] = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = frameDownloadCommandBuffers;
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.pSignalSemaphores = {};
        if (vkQueueSubmit(device->getGraphicsQueue(), 1, &submitInfo, frameDownloadedFence->getVkFence()) != VK_SUCCESS) {
            sgl::Logfile::get()->throwError(
                    "Error in Swapchain::renderFrame: Could not submit to the graphics queue.");
        }
        frameDownloadedFence->wait();
        frameDownloadedFence->reset();
        downloadSwapchainRender();
    }
}

void Swapchain::renderFrame(const std::vector<sgl::vk::CommandBufferPtr>& commandBuffers) {
    if (commandBuffers.empty()) {
        sgl::Logfile::get()->throwError("Error in Swapchain::renderFrame: Command buffer array empty!");
    }

    //const sgl::vk::CommandBufferPtr& commandBufferFirst = commandBuffers.front();
    const sgl::vk::CommandBufferPtr& commandBufferLast = commandBuffers.back();

    //VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    if (useDownloadSwapchain) {
        signalSemaphores[0] = frameRenderedSemaphore->getVkSemaphore();
    }
    // The wait semaphore is added by sgl::vk::Renderer to ensure GPU-CPU syncing is possible.
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

        VkFence fence = VK_NULL_HANDLE;
        if (cmdBufIdx == commandBuffers.size() - 1) {
            if (!useDownloadSwapchain) {
                fence = inFlightFences[currentFrame];
            }
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

    if (useDownloadSwapchain) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(frameDownloadCommandBuffer->getVkCommandBuffer(), &beginInfo);
        swapchainImages.front()->copyToImage(
                swapchainImageCpu, VK_IMAGE_ASPECT_COLOR_BIT,
                frameDownloadCommandBuffer->getVkCommandBuffer());
        vkEndCommandBuffer(frameDownloadCommandBuffer->getVkCommandBuffer());

        VkCommandBuffer frameDownloadCommandBuffers[] = { frameDownloadCommandBuffer->getVkCommandBuffer() };
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        VkSemaphore waitSemaphores[] = { frameRenderedSemaphore->getVkSemaphore() };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = frameDownloadCommandBuffers;
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.pSignalSemaphores = {};
        if (vkQueueSubmit(device->getGraphicsQueue(), 1, &submitInfo, frameDownloadedFence->getVkFence()) != VK_SUCCESS) {
            sgl::Logfile::get()->throwError(
                    "Error in Swapchain::renderFrame: Could not submit to the graphics queue.");
        }
        frameDownloadedFence->wait();
        frameDownloadedFence->reset();
        downloadSwapchainRender();
        return;
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

void Swapchain::downloadSwapchainRender() {
    auto* sdlWindow = static_cast<SDLWindow*>(window);
    auto* cpuSurfaceSdl = reinterpret_cast<SDL_Surface*>(cpuSurface);
    //auto* cpuSurfaceWriteableSdl = reinterpret_cast<SDL_Surface*>(cpuSurfaceWriteable);

    int width = int(swapchainImageCpu->getImageSettings().width);
    int height = int(swapchainImageCpu->getImageSettings().height);
    VkSubresourceLayout subresourceLayout =
            swapchainImageCpu->getSubresourceLayout(VK_IMAGE_ASPECT_COLOR_BIT);

    auto* mappedData = reinterpret_cast<uint8_t*>(swapchainImageCpu->mapMemory());
    auto* cpuSurfaceWriteableSdl = SDL_CreateRGBSurfaceFrom(
            mappedData, width, height, 32, int(subresourceLayout.rowPitch),
            0x000000FFu, 0x0000FF00u, 0x00FF0000u, 0xFF000000u);
            //0x000000FFu, 0x0000FF00u, 0x00F0000u, 0xFF000000u);
    /*SDL_LockSurface(cpuSurfaceWriteableSdl);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int writeLocation = (x + y * width) * 4;
            int readLocation = int(subresourceLayout.offset) + x * 4 + int(subresourceLayout.rowPitch) * y;
            for (int c = 0; c < 3; c++) {
                bitmapPixels[writeLocation + c] = mappedData[readLocation + c];
            }
            bitmapPixels[writeLocation + 3] = 255;
        }
    }
    SDL_UnlockSurface(cpuSurfaceWriteableSdl);*/
    SDL_BlitSurface(cpuSurfaceWriteableSdl, NULL, cpuSurfaceSdl, NULL);
    SDL_FreeSurface(cpuSurfaceWriteableSdl);
    swapchainImageCpu->unmapMemory();
    SDL_UpdateWindowSurface(sdlWindow->getSDLWindow());
}

void Swapchain::cleanupRecreate() {
    vkDeviceWaitIdle(device->getVkDevice());

    swapchainFramebuffers.clear();
    swapchainImages.clear();
    swapchainImageViews.clear();
    if (swapchain) {
        vkDestroySwapchainKHR(device->getVkDevice(), swapchain, nullptr);
        swapchain = {};
    }
}

void Swapchain::cleanup() {
    cleanupRecreate();

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device->getVkDevice(), renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device->getVkDevice(), imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device->getVkDevice(), inFlightFences[i], nullptr);
    }
}

SwapchainSupportInfo querySwapchainSupportInfo(VkPhysicalDevice device, VkSurfaceKHR surface, Window* window) {
    SwapchainSupportInfo swapchainSupportInfo;
    if (!surface) {
        sgl::Logfile::get()->throwError("Error in querySwapchainSupportInfo: VkSurfaceKHR object is null.");
    }

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapchainSupportInfo.capabilities);
	if (window) {
        while (swapchainSupportInfo.capabilities.currentExtent.width == 0
                || swapchainSupportInfo.capabilities.currentExtent.height == 0) {
            VkResult errorCode = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                    device, surface, &swapchainSupportInfo.capabilities);
            if (errorCode != VK_SUCCESS) {
                sgl::Logfile::get()->writeError("Error in vkGetPhysicalDeviceSurfaceCapabilitiesKHR.");
            }
            window->processEvents();
        }
    }

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
