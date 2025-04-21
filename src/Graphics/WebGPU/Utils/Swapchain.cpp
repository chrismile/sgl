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

#include <cstring>
#include <map>

#ifdef WEBGPU_BACKEND_WGPU
#include <webgpu/wgpu.h>
#endif

#include <Utils/File/Logfile.hpp>
#include <Utils/Events/EventManager.hpp>

#include "Common.hpp"
#include "Instance.hpp"
#include "Device.hpp"
#include "Swapchain.hpp"

namespace sgl { namespace webgpu {

Swapchain::Swapchain(Device* device) : device(device) {
}

Swapchain::~Swapchain() {
    cleanup();

#ifdef WEBGPU_IMPL_SUPPORTS_WAIT_ON_FUTURE
    WGPUFutureWaitInfo futureWaitInfo{};
    futureWaitInfo.future = submittedWorkFuture;
    wgpuInstanceWaitAny(device->getInstance()->getWGPUInstance(), 1, &futureWaitInfo, 0);
#endif
}

void Swapchain::create(Window* window) {
    this->window = window;
    if (window) {
        surface = window->getWebGPUSurface();
    }
    if (!surface) {
        sgl::Logfile::get()->throwError("Error in Swapchain::create: Surface is nullptr.");
    }
    WindowSettings windowSettings = window->getWindowSettings();

    if (windowSettings.pixelWidth == 0 || windowSettings.pixelHeight == 0) {
        validPixelSize = false;
        return;
    }
    validPixelSize = true;

    surfaceFormat = WGPUTextureFormat_Undefined;
    WGPUSurfaceCapabilities capabilities{};
    wgpuSurfaceGetCapabilities(surface, device->getWGPUAdapter(), &capabilities);
    for (size_t i = 0; i < capabilities.formatCount; i++) {
        if (capabilities.formats[i] == WGPUTextureFormat_BGRA8Unorm
                || capabilities.formats[i] == WGPUTextureFormat_RGBA8Unorm) {
            surfaceFormat = capabilities.formats[i];
            break;
        }
    }
    if (surfaceFormat == WGPUTextureFormat_Undefined) {
        //surfaceFormat = wgpuSurfaceGetPreferredFormat(surface, device->getWGPUAdapter()); // deprecated
        sgl::Logfile::get()->throwError("Error in Swapchain::create: Could not find matching format.");
    }

    WGPUSurfaceConfiguration config{};
    config.width = uint32_t(windowSettings.pixelWidth);
    config.height = uint32_t(windowSettings.pixelHeight);
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
    if (validPixelSize) {
        EventManager::get()->triggerEvent(std::make_shared<Event>(RESOLUTION_CHANGED_EVENT));
    }
}

void Swapchain::cleanupRecreate() {
    if (surface && validPixelSize) {
        wgpuSurfaceUnconfigure(surface);
        surface = {};
    }
}

void Swapchain::cleanup() {
    cleanupRecreate();
}

static std::map<WGPUSurfaceGetCurrentTextureStatus, std::string> surfaceTextureStatusNameMap = {
#ifdef WEBGPU_LEGACY_API
        { WGPUSurfaceGetCurrentTextureStatus_Success, "Success" },
        { WGPUSurfaceGetCurrentTextureStatus_OutOfMemory, "Out of memory" },
        { WGPUSurfaceGetCurrentTextureStatus_DeviceLost, "Device lost" },
#else
        { WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal, "Success optimal" },
        { WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal, "Success suboptimal" },
        { WGPUSurfaceGetCurrentTextureStatus_Error, "Error" },
#endif
        { WGPUSurfaceGetCurrentTextureStatus_Timeout, "Timeout" },
        { WGPUSurfaceGetCurrentTextureStatus_Outdated, "Outdated" },
        { WGPUSurfaceGetCurrentTextureStatus_Lost, "Lost" },
};

bool Swapchain::beginFrame() {
    if (!validPixelSize) {
        return false;
    }

    WGPUSurfaceTexture surfaceTexture;
    wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);
    if (surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_Outdated) {
        // TODO: Handle WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal here, too?
        if (surfaceTexture.texture != nullptr) {
            wgpuTextureRelease(surfaceTexture.texture);
        }
        recreateSwapchain();
        return false;
    } else if (
#ifdef WEBGPU_LEGACY_API
            surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success
#else
            surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal
            && surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal
#endif
            ) {
        auto it = surfaceTextureStatusNameMap.find(surfaceTexture.status);
        if (it != surfaceTextureStatusNameMap.end()) {
            sgl::Logfile::get()->throwError(
                    "Error in Swapchain::beginFrame: Failed to acquire surface texture: " + it->second);
        } else {
            sgl::Logfile::get()->throwError("Error in Swapchain::beginFrame: Failed to acquire surface texture!");
        }
        if (surfaceTexture.texture != nullptr) {
            wgpuTextureRelease(surfaceTexture.texture);
        }
        return false;
    }

    WGPUTextureViewDescriptor viewDescriptor{};
    cStringToWgpuView(viewDescriptor.label, "Surface texture view");
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
    currentTexture = surfaceTexture.texture;

    return true;
}

static std::map<WGPUQueueWorkDoneStatus, std::string> queueWorkDoneStatusNameMap = {
        { WGPUQueueWorkDoneStatus_Success, "Success" },
        { WGPUQueueWorkDoneStatus_Error, "Error" },
#ifdef WEBGPU_BACKEND_EMDAWNWEBGPU
        { WGPUQueueWorkDoneStatus_CallbackCancelled, "Callback cancelled" },
#endif
#ifdef WEBGPU_LEGACY_API
        { WGPUQueueWorkDoneStatus_DeviceLost, "Device lost" },
#elif !defined(WEBGPU_BACKEND_EMDAWNWEBGPU)
        { WGPUQueueWorkDoneStatus_InstanceDropped, "Instance dropped" },
#endif
#ifdef WEBGPU_BACKEND_WGPU
        { WGPUQueueWorkDoneStatus_Unknown, "Unknown" },
#endif
};

void Swapchain::renderFrame(const std::vector<WGPUCommandBuffer>& commandBuffers) {
    wgpuQueueSubmit(device->getWGPUQueue(), commandBuffers.size(), commandBuffers.data());

#ifdef WEBGPU_LEGACY_API
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
#else
    auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void* userdata1, void* userdata2) {
        if (status != WGPUQueueWorkDoneStatus_Success) {
#ifdef WEBGPU_BACKEND_DAWN
            // TODO: Check if this is necessary for WGPU or Emscripten.
            if (status == WGPUQueueWorkDoneStatus_InstanceDropped) {
                auto* instance = reinterpret_cast<sgl::webgpu::Instance*>(userdata1);
                if (instance->getIsInDestructor()) {
                    return;
                }
            }
#endif
            auto it = queueWorkDoneStatusNameMap.find(status);
            if (it != queueWorkDoneStatusNameMap.end()) {
                sgl::Logfile::get()->throwError(
                        "Error in wgpuQueueOnSubmittedWorkDone: " + it->second);
            } else {
                sgl::Logfile::get()->throwError("Error in wgpuQueueOnSubmittedWorkDone2: Queue work failed!");
            }
        }
    };
    WGPUQueueWorkDoneCallbackInfo queueWorkDoneCallbackInfo{};
    queueWorkDoneCallbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
    queueWorkDoneCallbackInfo.callback = onQueueWorkDone;
    queueWorkDoneCallbackInfo.userdata1 = device->getInstance();
    submittedWorkFuture = wgpuQueueOnSubmittedWorkDone(device->getWGPUQueue(), queueWorkDoneCallbackInfo);
#endif

#ifndef __EMSCRIPTEN__
    wgpuSurfacePresent(surface);
#endif

    wgpuTextureViewRelease(currentTextureView);
    wgpuTextureRelease(currentTexture);
    currentTextureView = {};
    currentTexture = {};

#if defined(WEBGPU_BACKEND_DAWN)
    wgpuDeviceTick(device->getWGPUDevice());
#elif defined(WEBGPU_BACKEND_WGPU)
    wgpuDevicePoll(device->getWGPUDevice(), false, nullptr);
#endif
}

}}
