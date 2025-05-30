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

#ifndef DEVICESELECTIONWGL_HPP
#define DEVICESELECTIONWGL_HPP

#ifndef __LP64__
typedef unsigned long DWORD;
#else
typedef unsigned int DWORD;
#endif

#include <Graphics/Utils/DeviceSelection.hpp>

namespace sgl {

// The user needs to include DeviceSelectionWGLGlobals.hpp in some module.

class DLL_OBJECT DeviceSelectorWGL : public DeviceSelector {
public:
    DeviceSelectorWGL(DWORD* _NvOptimusEnablement, DWORD* _AmdPowerXpressRequestHighPerformance);
    void serializeSettings(JsonValue& settings) override;
    void deserializeSettings(const JsonValue& settings) override;
    void renderGui() override;
    void renderGuiMenu() override;

private:
    void checkUsedVendor();
    bool isFirstFrame = true;

    // System configuration.
    bool isHybridNvidia = false;
    bool isHybridAmd = false;

    // Current selection.
    bool useNvidiaDiscrete = false;
    bool useAmdDiscrete = false;

    // User selection.
    bool forceUseNvidiaDiscrete = false;
    bool forceUseAmdDiscrete = false;

    DWORD* _NvOptimusEnablement = nullptr;
    DWORD* _AmdPowerXpressRequestHighPerformance = nullptr;
};

#ifdef SUPPORT_VULKAN
namespace vk {
class Device;
}
DLL_OBJECT void attemptForceWglContextForVulkanDevice(
    sgl::vk::Device* device, DWORD* _NvOptimusEnablement, DWORD* _AmdPowerXpressRequestHighPerformance);
#endif

}

#endif //DEVICESELECTIONWGL_HPP
