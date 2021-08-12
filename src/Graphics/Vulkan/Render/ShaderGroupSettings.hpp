/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Christoph Neuhauser
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

#ifndef SGL_SHADERGROUPSETTINGS_HPP
#define SGL_SHADERGROUPSETTINGS_HPP

#include <limits>

namespace sgl { namespace vk {

/**
 * Used in vk::Renderer to reference certain RayTracingShaderGroup objects in the shader binding table.
 * All indices/offsets are local to the group type (RayGen, Miss, Hit), and the unit of index/offset/size is
 * one shader group (NOT bytes!).
 */
struct DLL_OBJECT ShaderGroupSettings {
    uint32_t rayGenShaderIndex = 0;
    uint32_t missShaderGroupOffset = 0;
    uint32_t missShaderGroupSize = std::numeric_limits<uint32_t>::max(); ///< Max encodes all groups.
    uint32_t hitShaderGroupOffset = 0;
    uint32_t hitShaderGroupSize = std::numeric_limits<uint32_t>::max(); ///< Max encodes all groups.
};

}}

#endif //SGL_SHADERGROUPSETTINGS_HPP
