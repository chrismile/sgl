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

#ifndef SGL_WEBGPU_COMPUTEPIPELINE_HPP
#define SGL_WEBGPU_COMPUTEPIPELINE_HPP

#include <memory>
#include <webgpu/webgpu.h>

namespace sgl { namespace webgpu {

class ShaderStages;
typedef std::shared_ptr<ShaderStages> ShaderStagesPtr;

class DLL_OBJECT ComputePipelineInfo {
    friend class ComputePipeline;

public:
    explicit ComputePipelineInfo(ShaderStagesPtr shaderStages);
    void addConstantEntry(const std::string& key, double value);
    void removeConstantEntry(const std::string& key);

protected:
    ShaderStagesPtr shaderStages;
    std::map<std::string, double> constantEntriesMap;
};

class DLL_OBJECT ComputePipeline {
public:
    ComputePipeline(Device* device, const ComputePipelineInfo& pipelineInfo);
    ~ComputePipeline();

    [[nodiscard]] inline Device* getDevice() { return device; }
    [[nodiscard]] inline ShaderStagesPtr& getShaderStages() { return shaderStages; }
    [[nodiscard]] inline const ShaderStagesPtr& getShaderStages() const { return shaderStages; }

    [[nodiscard]] inline WGPUPipelineLayout getWGPUPipelineLayout() const { return pipelineLayout; }
    [[nodiscard]] inline WGPUComputePipeline getWGPUPipeline() { return pipeline; }

protected:
    Device* device;
    ShaderStagesPtr shaderStages;
    WGPUPipelineLayout pipelineLayout{};
    WGPUComputePipeline pipeline{};
};

typedef std::shared_ptr<ComputePipeline> ComputePipelinePtr;

}}

#endif //SGL_WEBGPU_COMPUTEPIPELINE_HPP
