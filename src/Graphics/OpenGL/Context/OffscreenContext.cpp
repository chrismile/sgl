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

#ifndef _WIN32
#include "OffscreenContextEGL.hpp"
#endif
#include "OffscreenContextGlfw.hpp"

namespace sgl {

OffscreenContext* createOffscreenContext(sgl::vk::Device* vulkanDevice, bool verbose) {
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
#ifndef _WIN32
    sgl::OffscreenContextEGLParams paramsEgl{};
    paramsEgl.device = vulkanDevice;
    offscreenContext = new sgl::OffscreenContextEGL(paramsEgl);
    offscreenContext->initialize();

    if (!offscreenContext->getIsInitialized()) {
        delete offscreenContext;
#endif
        offscreenContext = new sgl::OffscreenContextGlfw();
        offscreenContext->initialize();
        if (!offscreenContext->getIsInitialized()) {
            delete offscreenContext;
            offscreenContext = nullptr;
        }
#ifndef _WIN32
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
