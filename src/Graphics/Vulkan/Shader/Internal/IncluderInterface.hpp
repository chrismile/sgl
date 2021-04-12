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

#ifndef SGL_INCLUDERINTERFACE_HPP
#define SGL_INCLUDERINTERFACE_HPP

#include <string>
#include <shaderc/shaderc.hpp>

namespace sgl { namespace vk {

class ShaderManager;

class IncluderInterface : public shaderc::CompileOptions::IncluderInterface {
public:
    // Called by ShaderManager.
    inline void setShaderManager(ShaderManager* shaderManager) { this->shaderManager = shaderManager; }

    virtual shaderc_include_result *GetInclude(
            const char *requestedSource, shaderc_include_type type,
            const char *requestingSource, size_t includeDepth) override;
    virtual void ReleaseInclude(shaderc_include_result *data) override;

private:
    std::string getDirectoryFromFilename(const std::string &filename);
    ShaderManager* shaderManager = nullptr;
};

}}

#endif //SGL_INCLUDERINTERFACE_HPP
