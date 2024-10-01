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

#ifndef SGL_WEBGPU_SHADER_HPP
#define SGL_WEBGPU_SHADER_HPP

#include <string>
#include <memory>
#include <webgpu/webgpu.h>
#include "Reflect/WGSLReflect.hpp"

namespace sgl { namespace webgpu {

class Device;
class ShaderModule;
typedef std::shared_ptr<ShaderModule> ShaderModulePtr;
class ShaderStages;
typedef std::shared_ptr<ShaderStages> ShaderStagesPtr;

class ShaderModule {
    friend class ShaderStages;
public:
    ShaderModule(WGPUShaderModule shaderModule, ReflectInfo reflectInfo);
    ~ShaderModule();

    [[nodiscard]] const WGPUShaderModule& getWGPUShaderModule() const { return shaderModule; }

private:
    WGPUShaderModule shaderModule{};
    ReflectInfo reflectInfo;
};

class ShaderStages {
public:
    ShaderStages(
            Device* device,
            const std::vector<ShaderModulePtr>& shaderModules, const std::vector<std::string>& entryPoints);
    ~ShaderStages();

    [[nodiscard]] inline const std::vector<WGPUBindGroupLayout>& getWGPUBindGroupLayouts() const {
        return bindGroupLayouts;
    }

    [[nodiscard]] const ShaderModulePtr& getShaderModule(ShaderType shaderType) const;
    [[nodiscard]] const std::string& getEntryPoint(ShaderType shaderType) const;

    [[nodiscard]] const std::vector<InOutEntry>& getInputVariableDescriptors() const;
    [[nodiscard]] bool getHasInputVariable(const std::string& varName) const;
    [[nodiscard]] uint32_t getInputVariableLocation(const std::string& varName) const;
    [[nodiscard]] uint32_t getInputVariableLocationIndex(const std::string& varName) const;
    const InOutEntry& getInputVariableDescriptorFromLocation(uint32_t location);
    const InOutEntry& getInputVariableDescriptorFromName(const std::string& name);

    [[nodiscard]] const std::map<uint32_t, std::vector<BindingEntry>>& getBindGroupsInfo() const;
    [[nodiscard]] bool hasBindingEntry(uint32_t groupIdx, const std::string& descName) const;
    [[nodiscard]] const BindingEntry& getBindingEntryByName(uint32_t groupIdx, const std::string& descName) const;
    [[nodiscard]] const BindingEntry& getBindingEntryByIndex(uint32_t groupIdx, uint32_t binding) const;
    [[nodiscard]] uint32_t getBindingIndexByName(uint32_t groupIdx, const std::string& descName) const;
    bool getBindingEntryByNameOptional(uint32_t groupIdx, const std::string& descName, uint32_t& bindingIndex) const;

private:
    void mergeBindGroupsInfo(const std::map<uint32_t, std::vector<BindingEntry>>& newBindGroupsInfo);
    void createBindGroupLayouts();

    Device* device;

    std::vector<ShaderModulePtr> shaderModules;
    std::vector<std::string> entryPoints;
    std::vector<ShaderType> shaderModuleTypes;

    std::map<std::string, uint32_t> inputVariableNameLocationMap; ///< input interface variable name -> location
    std::map<uint32_t, std::string> inputLocationVariableNameMap; ///< input interface variable location -> name
    std::map<std::string, uint32_t> inputVariableNameLocationIndexMap; ///< input interface variable name -> loc. index

    std::map<uint32_t, std::vector<BindingEntry>> bindGroupsInfo; ///< group index -> descriptor set info
    std::map<std::string, std::vector<BindingEntry>> bindGroupsNameMap; ///< name -> binding
    std::vector<WGPUBindGroupLayout> bindGroupLayouts;
};

}}

#endif //SGL_WEBGPU_SHADER_HPP
