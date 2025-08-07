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

#ifndef SGL_RESOURCE_HPP
#define SGL_RESOURCE_HPP

#include "../Utils/d3d12.hpp"

namespace sgl { namespace d3d12 {

class Device;

struct ResourceSettings {

};

class DLL_OBJECT Resource {
public:
    explicit Resource(Device* device, const ResourceSettings& resourceSettings);
    ~Resource();

    void uploadData(size_t sizeInBytesData, const void* dataPtr);

    HANDLE getSharedHandle(const std::wstring& handleName);
    /** A not thread-safe version using a static counter for handle name "Local\\D3D12ResourceHandle{ctr}". */
    HANDLE getSharedHandle();

    inline ID3D12Resource* getD3D12Resource() { return resource.Get(); }

private:
    Device* device;

    ComPtr<ID3D12Resource> resource{};
};

}}

#endif //SGL_RESOURCE_HPP
