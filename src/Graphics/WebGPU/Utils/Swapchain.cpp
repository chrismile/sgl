/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2024, Christoph Neuhauser
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

#include <map>

#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h>
#endif

#include <Utils/File/Logfile.hpp>
#include <Utils/Events/EventManager.hpp>
#include <SDL/SDLWindow.hpp>

#include "Device.hpp"
#include "Swapchain.hpp"

namespace sgl { namespace webgpu {

Swapchain::Swapchain(Device* device) : device(device) {
}

Swapchain::~Swapchain() {
    cleanup();
}

void Swapchain::create(Window* window) {
    this->window = window;
    auto* sdlWindow = static_cast<SDLWindow*>(window);
    if (sdlWindow) {
        surface = sdlWindow->getWebGPUSurface();
    }
    if (!surface) {
        sgl::Logfile::get()->throwError("Error in Swapchain::create: Surface is nullptr.");
    }
    WindowSettings windowSettings = window->getWindowSettings();

    WGPUSurfaceConfiguration config{};
    config.width = uint32_t(windowSettings.pixelWidth);
    config.height = uint32_t(windowSettings.pixelHeight);
//#ifdef WEBGPU_BACKEND_DAWN
    // TODO: Check if compatible with Emscripten, wgpu
    // TODO: Fails at the moment with Dawn.
    /*WGPUSurfaceCapabilities capabilities{};
    capabilities.usages = WGPUTextureUsage_RenderAttachment;
    bool success = wgpuSurfaceGetCapabilities(surface, device->getWGPUAdapter(), &capabilities) == WGPUStatus_Success;
    if (!success) {
        sgl::Logfile::get()->throwError("Error in Swapchain::create: wgpuSurfaceGetCapabilities failed.");
    }
    surfaceFormat = capabilities.formats[0];*/
//#else
    surfaceFormat = wgpuSurfaceGetPreferredFormat(surface, device->getWGPUAdapter());
//#endif
    config.format = surfaceFormat;
    config.viewFormatCount = 0;
    config.viewFormats = nullptr;
    config.usage = WGPUTextureUsage_RenderAttachment;
    config.device = device->getWGPUDevice();
    config.presentMode = WGPUPresentMode_Fifo; // TODO: WGPUPresentMode_Immediate, WGPUPresentMode_Mailbox, WGPUPresentMode_Fifo
    config.alphaMode = WGPUCompositeAlphaMode_Auto;
    wgpuSurfaceConfigure(surface, &config);
}

void Swapchain::recreateSwapchain() {
    cleanupRecreate();
    create(window);

    // Recreate framebuffer, pipeline, ...
    // For the moment, a resolution changed event is additionally triggered to be compatible with OpenGL.
    EventManager::get()->triggerEvent(std::make_shared<Event>(RESOLUTION_CHANGED_EVENT));
}

void Swapchain::cleanupRecreate() {
    if (surface) {
        wgpuSurfaceUnconfigure(surface);
        surface = {};
    }
}

void Swapchain::cleanup() {
    cleanupRecreate();
}

static std::map<WGPUSurfaceGetCurrentTextureStatus, std::string> surfaceTextureStatusNameMap = {
        { WGPUSurfaceGetCurrentTextureStatus_Success, "Success" },
        { WGPUSurfaceGetCurrentTextureStatus_Timeout, "Timeout" },
        { WGPUSurfaceGetCurrentTextureStatus_Outdated, "Outdated" },
        { WGPUSurfaceGetCurrentTextureStatus_Lost, "Lost" },
        { WGPUSurfaceGetCurrentTextureStatus_OutOfMemory, "Out of memory" },
        { WGPUSurfaceGetCurrentTextureStatus_DeviceLost, "Device lost" },
#ifdef WEBGPU_BACKEND_DAWN
        { WGPUSurfaceGetCurrentTextureStatus_Error, "Error" },
#endif
};

void Swapchain::beginFrame() {
    WGPUSurfaceTexture surfaceTexture;
    wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);
    if (surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_Outdated) {
        recreateSwapchain();
        return;
    } else if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
        auto it = surfaceTextureStatusNameMap.find(surfaceTexture.status);
        if (it != surfaceTextureStatusNameMap.end()) {
            sgl::Logfile::get()->throwError(
                    "Error in Swapchain::beginFrame: Failed to acquire surface texture: " + it->second);
        } else {
            sgl::Logfile::get()->throwError("Error in Swapchain::beginFrame: Failed to acquire surface texture!");
        }
    }

    WGPUTextureViewDescriptor viewDescriptor{};
    viewDescriptor.label = "Surface texture view";
    viewDescriptor.format = wgpuTextureGetFormat(surfaceTexture.texture);
    viewDescriptor.dimension = WGPUTextureViewDimension_2D;
    viewDescriptor.baseMipLevel = 0;
    viewDescriptor.mipLevelCount = 1;
    viewDescriptor.baseArrayLayer = 0;
    viewDescriptor.arrayLayerCount = 1;
    viewDescriptor.aspect = WGPUTextureAspect_All;
    currentTextureView = wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);
    if (!currentTextureView) {
        sgl::Logfile::get()->throwError("Error in Swapchain::beginFrame: wgpuTextureCreateView failed!");
    }
}

static std::map<WGPUQueueWorkDoneStatus, std::string> queueWorkDoneStatusNameMap = {
        { WGPUQueueWorkDoneStatus_Success, "Success" },
        { WGPUQueueWorkDoneStatus_Error, "Error" },
        { WGPUQueueWorkDoneStatus_Unknown, "Unknown" },
        { WGPUQueueWorkDoneStatus_DeviceLost, "Device lost" },
#ifdef WEBGPU_BACKEND_DAWN
        { WGPUQueueWorkDoneStatus_InstanceDropped, "Instance dropped" },
#endif
};

void Swapchain::renderFrame(const std::vector<WGPUCommandBuffer>& commandBuffers) {
    wgpuQueueSubmit(device->getWGPUQueue(), commandBuffers.size(), commandBuffers.data());

    auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void* userdata) {
        if (status != WGPUQueueWorkDoneStatus_Success) {
            auto it = queueWorkDoneStatusNameMap.find(status);
            if (it != queueWorkDoneStatusNameMap.end()) {
                sgl::Logfile::get()->throwError(
                        "Error in wgpuQueueOnSubmittedWorkDone: " + it->second);
            } else {
                sgl::Logfile::get()->throwError("Error in wgpuQueueOnSubmittedWorkDone: Queue work failed!");
            }
        }
    };
    wgpuQueueOnSubmittedWorkDone(device->getWGPUQueue(), onQueueWorkDone, nullptr);

    wgpuTextureViewRelease(currentTextureView);
#ifndef __EMSCRIPTEN__
    wgpuSurfacePresent(surface);
#endif

#if defined(WEBGPU_BACKEND_DAWN)
    wgpuDeviceTick(device->getWGPUDevice());
#elif defined(WEBGPU_BACKEND_WGPU)
    wgpuDevicePoll(device->getWGPUDevice(), false, nullptr);
#endif
}

}}
