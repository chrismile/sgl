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

#ifndef SGL_FENCE_HPP
#define SGL_FENCE_HPP

#include "../Utils/d3d12.hpp"

namespace sgl { namespace d3d12 {

class Device;

class DLL_OBJECT Fence {
public:
    explicit Fence(Device* device, uint64_t value = 0, D3D12_FENCE_FLAGS flags = D3D12_FENCE_FLAG_NONE);
    virtual ~Fence();
    void waitOnCpu(uint64_t value);
    /** Returns whether the wait succeeded (true) or a timeout or error was encountered (false). */
    bool waitOnCpu(uint64_t value, DWORD timeoutMs);

    HANDLE getSharedHandle(const std::wstring& handleName);
    /** A not thread-safe version using a static counter for handle name "Local\\D3D12FenceHandle{ctr}". */
    HANDLE getSharedHandle();

    inline ID3D12Fence* getD3D12Fence() { return fence.Get(); }

protected:
    Fence() = default;
    void _initialize(Device* device, uint64_t value, D3D12_FENCE_FLAGS flags);
    ComPtr<ID3D12Fence> fence{};

private:
    Device* device;
    HANDLE fenceEvent{};
};

typedef std::shared_ptr<Fence> FencePtr;

}}

#endif //SGL_FENCE_HPP
