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

#include <Utils/File/Logfile.hpp>
#include <Graphics/Texture/Bitmap.hpp>
#include <Graphics/Vulkan/Utils/Device.hpp>
#include <Graphics/Vulkan/Utils/Swapchain.hpp>
#include <Graphics/Vulkan/Render/Renderer.hpp>

#include "ScreenshotReadbackHelper.hpp"

namespace sgl { namespace vk {

ScreenshotReadbackHelper::~ScreenshotReadbackHelper() {
    for (size_t i = 0; i < frameDataList.size(); i++) {
        saveDataIfAvailable(uint32_t(i));
    }
}

void ScreenshotReadbackHelper::onSwapchainRecreated() {
    sgl::Window *window = sgl::AppSettings::get()->getMainWindow();
    auto width = uint32_t(window->getWidth());
    auto height = uint32_t(window->getHeight());
    onSwapchainRecreated(width, height);
}

void ScreenshotReadbackHelper::onSwapchainRecreated(uint32_t width, uint32_t height) {
    for (size_t i = 0; i < frameDataList.size(); i++) {
        saveDataIfAvailable(uint32_t(i));
    }

    vk::Swapchain* swapchain = AppSettings::get()->getSwapchain();
    size_t numSwapchainImages = swapchain ? swapchain->getNumImages() : 1;

    frameDataList.clear();
    frameDataList.reserve(numSwapchainImages);

    vk::Device* device = renderer->getDevice();
    while (numSwapchainImages > frameDataList.size()) {
        vk::ImageSettings imageSettings;
        imageSettings.width = width;
        imageSettings.height = height;
        imageSettings.format = VK_FORMAT_R8G8B8A8_UINT;
        imageSettings.tiling = VK_IMAGE_TILING_LINEAR;
        imageSettings.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageSettings.memoryUsage = VMA_MEMORY_USAGE_GPU_TO_CPU;
        vk::ImagePtr readBackImage(new vk::Image(device, imageSettings));
        readBackImage->transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        FrameData frameData;
        frameData.image = readBackImage;

        frameDataList.push_back(frameData);
    }
}

void ScreenshotReadbackHelper::requestScreenshotReadback(vk::ImagePtr& image, const std::string& filename) {
    vk::Swapchain* swapchain = AppSettings::get()->getSwapchain();
    uint32_t imageIndex = swapchain ? swapchain->getImageIndex() : 0;

    FrameData& frameData = frameDataList.at(imageIndex);
    if (frameData.used) {
        Logfile::get()->throwError("Error in ScreenshotReadbackHelper::requestScreenshotReadback: frameData.used");
    }
    frameData.used = true;
    frameData.filename = filename;

    // Copy the image data to the GPU -> CPU read-back image.
    vk::ImagePtr& readBackImage = frameData.image;
    // No FORMAT_FEATURE_BLIT_DST_BIT for linearTiling on NVIDIA drivers.
    // image->blit(readBackImage, renderer->getVkCommandBuffer());
    image->copyToImage(
            readBackImage, VK_IMAGE_ASPECT_COLOR_BIT, renderer->getVkCommandBuffer());
}

void ScreenshotReadbackHelper::saveDataIfAvailable(uint32_t imageIndex) {
    FrameData& frameData = frameDataList.at(imageIndex);

    if (!frameData.used) {
        return;
    }
    frameData.used = false;

    vk::ImagePtr& readBackImage = frameData.image;
    int width = int(readBackImage->getImageSettings().width);
    int height = int(readBackImage->getImageSettings().height);
    VkSubresourceLayout subresourceLayout = readBackImage->getSubresourceLayout(VK_IMAGE_ASPECT_COLOR_BIT);

    sgl::BitmapPtr bitmap(new sgl::Bitmap(width, height, 32));
    uint8_t* bitmapPixels = bitmap->getPixels();

    uint8_t* mappedData = reinterpret_cast<uint8_t*>(readBackImage->mapMemory());
    if (screenshotTransparentBackground) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int writeLocation = (x + y * width) * 4;
                // We don't need to add "int(subresourceLayout.offset)" here, as this is automatically done by VMA.
                int readLocation = x * 4 + int(subresourceLayout.rowPitch) * y;
                for (int c = 0; c < 4; c++) {
                    bitmapPixels[writeLocation + c] = mappedData[readLocation + c];
                }
            }
        }
    } else {
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
    }
    readBackImage->unmapMemory();

    bitmap->savePNG(frameData.filename.c_str(), false);
}

void ScreenshotReadbackHelper::setScreenshotTransparentBackground(bool transparentBackground) {
    screenshotTransparentBackground = transparentBackground;
}

}}
