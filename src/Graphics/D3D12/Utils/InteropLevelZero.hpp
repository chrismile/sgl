/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2026, Christoph Neuhauser
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

#ifndef SGL_D3D12_INTEROPLEVELZERO_HPP
#define SGL_D3D12_INTEROPLEVELZERO_HPP

#include <Graphics/Utils/InteropLevelZero.hpp>

namespace sgl { namespace d3d12 {

class Device;

/**
 * Calls zeInit or zeInitDrivers and selects the closest matching Level Zero device for the passed Vulkan device.
 * @param device The Vulkan device.
 * @param zeDriver The Level Zero driver (if true is returned).
 * @param zeDevice The Level Zero device (if true is returned).
 * @return Whether a matching Level Zero device was found.
 */
DLL_OBJECT bool initializeLevelZeroAndFindMatchingDevice(
        sgl::d3d12::Device* device, ze_driver_handle_t* zeDriver, ze_device_handle_t* zeDevice);

}}

#endif //SGL_D3D12_INTEROPLEVELZERO_HPP