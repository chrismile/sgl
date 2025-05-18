/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2025, Christoph Neuhauser
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

#ifndef OFFSCREENCONTEXTWGL_HPP
#define OFFSCREENCONTEXTWGL_HPP

#include <optional>
#include "OffscreenContext.hpp"

#if defined(_WIN32) && !defined(_WINDEF_)
struct HINSTANCE__;
typedef HINSTANCE__* HINSTANCE;
typedef HINSTANCE HMODULE;
struct HGLRC__;
typedef struct HGLRC__* HGLRC;
struct HDC__;
typedef struct HDC__* HDC;
struct HWND__;
typedef struct HWND__* HWND;
#endif

namespace sgl {

namespace vk {
class Device;
}

/**
 * Notes:
 * A Vulkan device can be passed. While on Linux with EGL EGL_EXT_device_persistent_id can be used for selecting a
 * suitable OpenGL context, this is not straight-forward on Windows with WGL.
 *
 * For, hybrid GPU configurations with an NVIDIA or AMD dGPU, we make use of the exported symbols "NvOptimusEnablement"
 * and "AmdPowerXpressRequestHighPerformance" in DeviceSelectionWGL.cpp using the function
 * @see attemptForceWglContextForVulkanDevice.
 *
 * For configurations where multiple GPUs from the same vendor are available, the WGL extensions WGL_NV_gpu_affinity
 * and WGL_AMD_gpu_association can be used. The former is unfortunately currently only available on non-gaming GPUs,
 * so we don't bother support it.
 *
 * It seems like CreateDCA could be used in the past for selecting GPUs from different vendors:
 * - https://community.khronos.org/t/how-to-use-opengl-with-a-device-chosen-by-you/63017/6
 * - https://community.khronos.org/t/how-to-create-wgl-context-for-specific-device/111852
 * - https://stackoverflow.com/questions/62372029/can-i-use-different-multigpu-in-opengl
 * However, it seems like I cannot get CreateDCA to return a non-nullptr value for anything than "\\\\.\\DISPLAY1".
 * Consequently, useDefaultDisplay is by default set to true.
 *
 */
struct DLL_OBJECT OffscreenContextWGLParams {
    bool useDefaultDisplay = true;
    int virtualWindowWidth = 640;
    int virtualWindowHeight = 480;
    sgl::vk::Device* device = nullptr;
};

struct OffscreenContextWGLFunctionTable;

class DLL_OBJECT OffscreenContextWGL : public OffscreenContext {
public:
    explicit OffscreenContextWGL(OffscreenContextWGLParams params = {});
    bool initialize() override;
    ~OffscreenContextWGL() override;
    void makeCurrent() override;
    void* getFunctionPointer(const char* functionName) override;
    [[nodiscard]] bool getIsInitialized() const override { return isInitialized; }

private:
    bool initializeFromWindow();
    bool initializeFromDeviceContextExperimental();
#ifdef SUPPORT_VULKAN
    /*
     * Returns the name of the display adapter associated with the GPU (e.g., \\.\DISPLAY1).
     * On Windows, multiple display adapters may exist for the same GPU.
     * Each display adapter may have multiple display monitors attached (e.g., \\.\DISPLAY1\Monitor0).
     * The name of the display adapter can be used with CreateDCA and the above patching code to create a suitable
     * OpenGL context.
     */
    std::string selectDisplayNameForVulkanDevice();
#endif
    bool isInitialized = false;
    HMODULE user32Module = nullptr;
    HMODULE opengl32Module = nullptr;
    HDC deviceContext = {};
    HGLRC glrc = {};
    HWND hWnd = {};
    OffscreenContextWGLParams params = {};

    // Function table.
    bool loadFunctionTable();
    OffscreenContextWGLFunctionTable* f = nullptr;
};

}

#endif //OFFSCREENCONTEXTWGL_HPP
