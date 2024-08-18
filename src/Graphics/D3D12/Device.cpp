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

#include "Device.hpp"

namespace sgl { namespace d3d12 {

Device::Device(const ComPtr<IDXGIAdapter1> &dxgiAdapter1, D3D_FEATURE_LEVEL featureLevel)
        : dxgiAdapter1(dxgiAdapter1), featureLevel(featureLevel) {
    ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
    ThrowIfFailed(D3D12CreateDevice(dxgiAdapter4.Get(), featureLevel, IID_PPV_ARGS(&d3d12Device2)));
}

D3D_FEATURE_LEVEL Device::getFeatureLevel() {
    return featureLevel;
}

bool Device::getSupportsROVs() {
    if (featureLevel >= D3D_FEATURE_LEVEL_12_1) {
        return true;
    }
    D3D12_FEATURE_DATA_D3D12_OPTIONS featureDataOptions;
    ThrowIfFailed(d3d12Device2->CheckFeatureSupport(
            D3D12_FEATURE_D3D12_OPTIONS, &featureDataOptions, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS)));
    return featureDataOptions.ROVsSupported;
}

}}
