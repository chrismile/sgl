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

#ifndef SGL_SWAPCHAIN_HPP
#define SGL_SWAPCHAIN_HPP

#include <vector>
#include <vulkan/vulkan.h>
#include <Graphics/Window.hpp>
#include "../Image/Image.hpp"
#include "../Buffers/Framebuffer.hpp"

namespace sgl { namespace vk {

class Device;

struct DLL_OBJECT SwapchainSupportInfo {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};
SwapchainSupportInfo querySwapchainSupportInfo(VkPhysicalDevice device, VkSurfaceKHR surface);

class DLL_OBJECT Swapchain {
public:
    /**
     * @param device The device object.
     * @param useClipping The user may disable clipping to be able to read back pixels obscured by another window.
     */
    Swapchain(Device* device, bool useClipping = true);
    ~Swapchain();
    void create(Window* window);
    void recreate();

    /**
     * Updates of buffers etc. can be performed between beginFrame and renderFrame.
     */
    void beginFrame();
    void renderFrame(std::vector<VkCommandBuffer>& commandBuffers);

    inline size_t getNumImages() { return swapchainImageViews.size(); }
    inline std::vector<ImageViewPtr>& getSwapchainImageViews() { return swapchainImageViews; }

private:
    void recreateSwapchain();
    void createSyncObjects();
    void createSwapchainImages();
    void createSwapchainImageViews();

    /// Only cleans up resources that are reallocated by @see recreate.
    void cleanupRecreate();
    /// Cleans up all resources.
    void cleanup();

    VkSurfaceFormatKHR getSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR getSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    VkSwapchainKHR swapchain;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;

    Device* device = nullptr;
    Window* window = nullptr;
    bool useClipping = true;
    std::vector<ImagePtr> swapchainImages;
    std::vector<ImageViewPtr> swapchainImageViews;
    std::vector<FramebufferPtr> swapchainFramebuffers;

    const int MAX_FRAMES_IN_FLIGHT = 2;
    size_t currentFrame = 0;
    uint32_t imageIndex = 0;
    bool framebufferResized = false;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
};

}}

#endif //SGL_SWAPCHAIN_HPP
