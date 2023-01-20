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

#ifndef SGL_OFFSCREENCONTEXTEGL_HPP
#define SGL_OFFSCREENCONTEXTEGL_HPP

#include "OffscreenContext.hpp"

typedef void* EGLDisplay;
typedef void* EGLContext;
typedef void* EGLSurface;
#if defined(_WIN32) && !defined(_WINDEF_)
class HINSTANCE__;
typedef HINSTANCE__* HINSTANCE;
typedef HINSTANCE HMODULE;
#endif

namespace sgl {

namespace vk {
class Device;
}

/*
 * Notes:
 * - Interestingly, on the NVIDIA 525.78.01 driver, the combination useDefaultDisplay = false and createPBuffer = false
 *   failed during tests. This is weird, as createPBuffer = false does not fail when using the default display.
 * - A Vulkan device can be passed. If EGL_EXT_device_persistent_id is available, the device UUID will be used for
 *   initializing the correct context. If it is not available, the Vulkan device will be ignored.
 */
struct DLL_OBJECT OffscreenContextEGLParams {
    bool useDefaultDisplay = false;
    bool createPBuffer = true;
    int pbufferWidth = 32;
    int pbufferHeight = 32;
    // If EGL_EXT_device_persistent_id is supported,
    sgl::vk::Device* device = nullptr;
};

struct OffscreenContextEGLFunctionTable;

/**
 * Initializes an offscreen context with EGL. EGL is loaded dynamically at runtime.
 * For more details see:
 * - https://github.com/KhronosGroup/Vulkan-Samples/blob/master/samples/extensions/open_gl_interop/offscreen_context.cpp
 * - https://developer.nvidia.com/blog/egl-eye-opengl-visualization-without-x-server/
 */
class DLL_OBJECT OffscreenContextEGL : public OffscreenContext {
public:
    explicit OffscreenContextEGL(OffscreenContextEGLParams params = {});
    bool initialize() override;
    ~OffscreenContextEGL() override;
    void makeCurrent() override;
    [[nodiscard]] bool getIsInitialized() const override { return isInitialized; }

private:
    bool isInitialized = false;
#ifdef _WIN32
    HMODULE eglHandle = nullptr;
#else
    void* eglHandle = nullptr;
#endif
    OffscreenContextEGLParams params = {};
    EGLDisplay eglDisplay = {};
    EGLContext eglContext = {};
    EGLSurface eglSurface = {};

    // Function table.
    bool loadFunctionTable();
    OffscreenContextEGLFunctionTable* f = nullptr;
};

}

#endif //SGL_OFFSCREENCONTEXTEGL_HPP
