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

#include "Instance.hpp"

namespace sgl { namespace webgpu {

Instance::Instance() {
}

Instance::~Instance() {
    isInDestructor = true;
    wgpuInstanceRelease(instance);
}

void Instance::onPreDeviceDestroy() {
    // TODO: Should already be implemented in WGPU via: https://github.com/gfx-rs/wgpu-native/pull/430
#ifdef WEBGPU_IMPL_SUPPORTS_WAIT_ON_FUTURE
    wgpuInstanceProcessEvents(instance);
#endif
}

void Instance::createInstance() {
#ifdef WEBGPU_BACKEND_EMSCRIPTEN

    instance = wgpuCreateInstance(nullptr);

#else

    WGPUInstanceDescriptor instanceDescriptor{};
#ifdef WEBGPU_BACKEND_DAWN
    // To simplify debugging, this makes sure error callbacks are called as soon as an error occurs.
    WGPUDawnTogglesDescriptor dawnTogglesDescriptor{};
    dawnTogglesDescriptor.chain.sType = WGPUSType_DawnTogglesDescriptor;
    dawnTogglesDescriptor.enabledToggleCount = 1;
    const char* toggleName = "enable_immediate_error_handling";
    dawnTogglesDescriptor.enabledToggles = &toggleName;
    instanceDescriptor.nextInChain = &dawnTogglesDescriptor.chain;
#endif
    instance = wgpuCreateInstance(&instanceDescriptor);

#endif
}

}}
