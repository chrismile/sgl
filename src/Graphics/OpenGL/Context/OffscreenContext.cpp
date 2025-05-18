/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2023, Christoph Neuhauser
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

#include <Utils/AppSettings.hpp>
#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/Utils/Interop.hpp>
#endif

#ifdef __linux__
#include "OffscreenContextGLX.hpp"
#endif
#ifndef _WIN32
#include "OffscreenContextEGL.hpp"
#endif
#ifdef _WIN32
#include "OffscreenContextWGL.hpp"
#include "DeviceSelectionWGL.hpp"
#endif
#include "OffscreenContextGlfw.hpp"

namespace sgl {

DLL_OBJECT OffscreenContext* createOffscreenContext(
        sgl::vk::Device* vulkanDevice, bool verbose) {
    OffscreenContextParams params{};
    params.tryUseZinkIfAvailable = false;
    return createOffscreenContext(vulkanDevice, params, verbose);
}

OffscreenContext* createOffscreenContext(
        sgl::vk::Device* vulkanDevice, OffscreenContextParams params, bool verbose) {
#ifdef SUPPORT_VULKAN
    // Check whether the Vulkan instance and device support OpenGL interop.
    if (vulkanDevice) {
        if (!sgl::AppSettings::get()->getInstanceSupportsVulkanOpenGLInterop()) {
            return nullptr;
        }
        if (!sgl::AppSettings::get()->checkVulkanOpenGLInteropDeviceExtensionsSupported(vulkanDevice)) {
            return nullptr;
        }
    }
#endif

    sgl::OffscreenContext* offscreenContext = nullptr;
#ifdef __linux__
    if (params.tryUseZinkIfAvailable) {
        sgl::OffscreenContextGLXParams paramsGlx{};
        paramsGlx.device = vulkanDevice;
        offscreenContext = new sgl::OffscreenContextGLX(paramsGlx);
        offscreenContext->initialize();
    }

    if (!offscreenContext || !offscreenContext->getIsInitialized()) {
        if (offscreenContext) {
            delete offscreenContext;
            offscreenContext = nullptr;
        }

#endif
#ifndef _WIN32
        sgl::OffscreenContextEGLParams paramsEgl{};
        paramsEgl.device = vulkanDevice;
        offscreenContext = new sgl::OffscreenContextEGL(paramsEgl);
        offscreenContext->initialize();
#else
#ifdef SUPPORT_VULKAN
        attemptForceWglContextForVulkanDevice(vulkanDevice);
#endif
        sgl::OffscreenContextWGLParams paramsWgl{};
        paramsWgl.device = vulkanDevice;
        offscreenContext = new sgl::OffscreenContextWGL(paramsWgl);
        offscreenContext->initialize();
#endif

        if (!offscreenContext->getIsInitialized()) {
            delete offscreenContext;
            offscreenContext = new sgl::OffscreenContextGlfw();
            offscreenContext->initialize();
            if (!offscreenContext->getIsInitialized()) {
                delete offscreenContext;
                offscreenContext = nullptr;
            }
        }
#ifdef __linux__
    }
#endif

#ifdef SUPPORT_VULKAN
    // Check whether the OpenGL context supports Vulkan interop.
    if (offscreenContext && vulkanDevice) {
        offscreenContext->makeCurrent();

        sgl::AppSettings::get()->initializeOffscreenContextFunctionPointers();
        if (!sgl::AppSettings::get()->checkOpenGLVulkanInteropExtensionsSupported()) {
            delete offscreenContext;
            offscreenContext = nullptr;
        }

        // Check if the device is actually compatible. If not, destroy the context.
        if (!sgl::isDeviceCompatibleWithOpenGl(vulkanDevice->getVkPhysicalDevice())) {
            sgl::Logfile::get()->writeError(
                "Disabling OpenGL interop due to mismatch in selected Vulkan device and OpenGL context.", false);
            delete offscreenContext;
            offscreenContext = nullptr;
        }
    }
#endif

    return offscreenContext;
}

void destroyOffscreenContext(OffscreenContext* offscreenContext) {
    if (offscreenContext) {
        delete offscreenContext;
    }
}

}
