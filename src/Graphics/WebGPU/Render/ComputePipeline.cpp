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

#include <Utils/File/Logfile.hpp>
#include <utility>
#include "../Utils/Device.hpp"
#include "../Shader/Shader.hpp"
#include "ComputePipeline.hpp"

namespace sgl { namespace webgpu {

ComputePipelineInfo::ComputePipelineInfo(ShaderStagesPtr shaderStages) : shaderStages(std::move(shaderStages)) {
    ;
}

void ComputePipelineInfo::addConstantEntry(const std::string& key, double value) {
    constantEntriesMap[key] = value;
}

void ComputePipelineInfo::removeConstantEntry(const std::string& key) {
    constantEntriesMap.erase(key);
}


ComputePipeline::ComputePipeline(Device* device, const ComputePipelineInfo& pipelineInfo) : device(device) {
    shaderStages = pipelineInfo.shaderStages;

    WGPUPipelineLayoutDescriptor pipelineLayoutDesc{};
    pipelineLayoutDesc.bindGroupLayoutCount = shaderStages->getWGPUBindGroupLayouts().size();
    pipelineLayoutDesc.bindGroupLayouts = shaderStages->getWGPUBindGroupLayouts().data();
    pipelineLayout = wgpuDeviceCreatePipelineLayout(device->getWGPUDevice(), &pipelineLayoutDesc);

    std::vector<WGPUConstantEntry> constantEntries;
    constantEntries.reserve(pipelineInfo.constantEntriesMap.size());
    for (const auto& constantEntryPair : pipelineInfo.constantEntriesMap) {
        WGPUConstantEntry constantEntry{};
        constantEntry.key = constantEntryPair.first.c_str();
        constantEntry.value = constantEntryPair.second;
        constantEntries.push_back(constantEntry);
    }

    WGPUComputePipelineDescriptor pipelineDesc{};
    pipelineDesc.compute.constantCount = constantEntries.size();
    pipelineDesc.compute.constants = constantEntries.empty() ? nullptr : constantEntries.data();
    pipelineDesc.compute.module = shaderStages->getShaderModule(ShaderType::COMPUTE)->getWGPUShaderModule();
    pipelineDesc.compute.entryPoint = shaderStages->getEntryPoint(ShaderType::COMPUTE).c_str();
    pipelineDesc.layout = pipelineLayout;

    pipeline = wgpuDeviceCreateComputePipeline(device->getWGPUDevice(), &pipelineDesc);
    if (!pipeline) {
        Logfile::get()->throwError(
                "Error in ComputePipeline::ComputePipeline: wgpuDeviceCreateComputePipeline failed.");
    }
}

ComputePipeline::~ComputePipeline() {
    if (pipeline) {
        wgpuComputePipelineRelease(pipeline);
        pipeline = {};
    }
    if (pipelineLayout) {
        wgpuPipelineLayoutRelease(pipelineLayout);
        pipelineLayout = {};
    }
}

}}
