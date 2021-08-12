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

#include <Utils/File/Logfile.hpp>
#include <Math/Math.hpp>
#include "../Utils/Instance.hpp"
#include "../Utils/Device.hpp"
#include "../Buffers/Buffer.hpp"
#include "../Shader/Shader.hpp"
#include "RayTracingPipeline.hpp"

namespace sgl { namespace vk {

RayTracingShaderGroup::RayTracingShaderGroup(ShaderStagesPtr shaderStages)
        : shaderStages(std::move(shaderStages)), device(shaderStages->getDevice()) {
    shaderGroupCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    shaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    shaderGroupCreateInfo.generalShader = VK_SHADER_UNUSED_KHR;
    shaderGroupCreateInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroupCreateInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroupCreateInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
}

void RayTracingShaderGroup::setShaderRecordData(void* data, uint32_t size) {
    recordData.resize(size);
    memcpy(recordData.data(), data, size);
}

uint32_t RayTracingShaderGroup::getSize() const {
    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rayTracingPipelineProperties =
            device->getPhysicalDeviceRayTracingPipelineProperties();
    const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
    const uint32_t handleAlignment = rayTracingPipelineProperties.shaderGroupHandleAlignment;
    const uint32_t baseAlignment = rayTracingPipelineProperties.shaderGroupBaseAlignment;
    const uint32_t handleSizeAligned = uint32_t(sgl::iceil(int(handleSize), int(handleAlignment))) * handleAlignment;
    uint32_t sizeUnaligned = handleSizeAligned + getRecordDataSize();
    uint32_t sizeAligned = uint32_t(sgl::iceil(int(sizeUnaligned), int(baseAlignment))) * baseAlignment;
    return sizeAligned;
}


RayGenShaderGroup::RayGenShaderGroup(ShaderStagesPtr shaderStages)
        : RayTracingShaderGroup(std::move(shaderStages)) {
    shaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
}

void RayGenShaderGroup::setRayGenShader(uint32_t shaderModuleIdx) {
    shaderGroupCreateInfo.generalShader = shaderModuleIdx;
    ShaderModuleType shaderModuleType = shaderStages->getShaderModules().at(shaderModuleIdx)->getShaderModuleType();
    if (shaderModuleType != ShaderModuleType::RAYGEN) {
        Logfile::get()->throwError(
                "Error in HitShaderGroup::setRayGenShader: Shader module type is not RAYGEN.");
    }
}
void RayGenShaderGroup::setRayGenShader(const std::string& shaderModuleId) {
    setRayGenShader(uint32_t(shaderStages->findModuleIndexFromId(shaderModuleId)));
}


MissShaderGroup::MissShaderGroup(ShaderStagesPtr shaderStages)
        : RayTracingShaderGroup(std::move(shaderStages)) {
    shaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
}

void MissShaderGroup::setMissShader(uint32_t shaderModuleIdx) {
    shaderGroupCreateInfo.generalShader = shaderModuleIdx;
    ShaderModuleType shaderModuleType = shaderStages->getShaderModules().at(shaderModuleIdx)->getShaderModuleType();
    if (shaderModuleType != ShaderModuleType::MISS) {
        Logfile::get()->throwError(
                "Error in MissShaderGroup::setRayGenShader: Shader module type is not MISS.");
    }
}
void MissShaderGroup::setMissShader(const std::string& shaderModuleId) {
    setMissShader(uint32_t(shaderStages->findModuleIndexFromId(shaderModuleId)));
}


HitShaderGroup::HitShaderGroup(ShaderStagesPtr shaderStages, VkRayTracingShaderGroupTypeKHR shaderGroupType)
        : RayTracingShaderGroup(std::move(shaderStages)) {
    if (shaderGroupType != VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR
    && shaderGroupType != VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR) {
        Logfile::get()->throwError(
                "Error in HitShaderGroup::HitShaderGroup: shaderGroupType must be either "
                "VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR or "
                "VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR.");
    }
    shaderGroupCreateInfo.type = shaderGroupType;
}

void HitShaderGroup::setClosestHitShader(uint32_t shaderModuleIdx) {
    shaderGroupCreateInfo.closestHitShader = shaderModuleIdx;
    ShaderModuleType shaderModuleType = shaderStages->getShaderModules().at(shaderModuleIdx)->getShaderModuleType();
    if (shaderModuleType != ShaderModuleType::CLOSEST_HIT) {
        Logfile::get()->throwError(
                "Error in HitShaderGroup::setClosestHitShader: Shader module type is not CLOSEST_HIT.");
    }
}
void HitShaderGroup::setClosestHitShader(const std::string& shaderModuleId) {
    setClosestHitShader(uint32_t(shaderStages->findModuleIndexFromId(shaderModuleId)));
}

void HitShaderGroup::setAnyHitShader(uint32_t shaderModuleIdx) {
    shaderGroupCreateInfo.anyHitShader = shaderModuleIdx;
    ShaderModuleType shaderModuleType = shaderStages->getShaderModules().at(shaderModuleIdx)->getShaderModuleType();
    if (shaderModuleType != ShaderModuleType::ANY_HIT) {
        Logfile::get()->throwError(
                "Error in HitShaderGroup::setAnyHitShader: Shader module type is not ANY_HIT.");
    }
}
void HitShaderGroup::setAnyHitShader(const std::string& shaderModuleId) {
    setAnyHitShader(uint32_t(shaderStages->findModuleIndexFromId(shaderModuleId)));
}

void HitShaderGroup::setIntersectionShader(uint32_t shaderModuleIdx) {
    shaderGroupCreateInfo.intersectionShader = shaderModuleIdx;
    ShaderModuleType shaderModuleType = shaderStages->getShaderModules().at(shaderModuleIdx)->getShaderModuleType();
    if (shaderModuleType != ShaderModuleType::INTERSECTION) {
        Logfile::get()->throwError(
                "Error in HitShaderGroup::setIntersectionShader: Shader module type is not INTERSECTION.");
    }
}
void HitShaderGroup::setIntersectionShader(const std::string& shaderModuleId) {
    setIntersectionShader(uint32_t(shaderStages->findModuleIndexFromId(shaderModuleId)));
}


CallableShaderGroup::CallableShaderGroup(ShaderStagesPtr shaderStages)
        : RayTracingShaderGroup(std::move(shaderStages)) {
    shaderGroupCreateInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
}

void CallableShaderGroup::setCallable(uint32_t shaderModuleIdx) {
    shaderGroupCreateInfo.generalShader = shaderModuleIdx;
    ShaderModuleType shaderModuleType = shaderStages->getShaderModules().at(shaderModuleIdx)->getShaderModuleType();
    if (shaderModuleType != ShaderModuleType::CALLABLE) {
        Logfile::get()->throwError(
                "Error in CallableShaderGroup::setRayGenShader: Shader module type is not CALLABLE.");
    }
}
void CallableShaderGroup::setCallable(const std::string& shaderModuleId) {
    setCallable(uint32_t(shaderStages->findModuleIndexFromId(shaderModuleId)));
}


RayGenShaderGroup* ShaderBindingTable::addRayGenShaderGroup() {
    auto shaderGroup = new RayGenShaderGroup(shaderStages);
    rayGenShaderGroups.push_back(RayTracingShaderGroupPtr(shaderGroup));
    return shaderGroup;
}
MissShaderGroup* ShaderBindingTable::addMissShaderGroup() {
    auto shaderGroup = new MissShaderGroup(shaderStages);
    missShaderGroups.push_back(RayTracingShaderGroupPtr(shaderGroup));
    return shaderGroup;
}
HitShaderGroup* ShaderBindingTable::addHitShaderGroup(VkRayTracingShaderGroupTypeKHR shaderGroupType) {
    auto shaderGroup = new HitShaderGroup(shaderStages, shaderGroupType);
    hitShaderGroups.push_back(RayTracingShaderGroupPtr(shaderGroup));
    return shaderGroup;
}
CallableShaderGroup* ShaderBindingTable::addCallableShaderGroup() {
    Logfile::get()->throwError(
            "Error in ShaderBindingTable::addCallableShaderGroup: Callable shaders are currently not supported.");
    auto shaderGroup = new CallableShaderGroup(shaderStages);
    callableShaderGroups.push_back(RayTracingShaderGroupPtr(shaderGroup));
    return shaderGroup;
}

void ShaderBindingTable::buildShaderGroups() {
    Device* device = shaderStages->getDevice();
    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rayTracingPipelineProperties =
            device->getPhysicalDeviceRayTracingPipelineProperties();
    const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
    const uint32_t baseAlignment = rayTracingPipelineProperties.shaderGroupBaseAlignment;
    const uint32_t groupSizeAligned = uint32_t(sgl::iceil(int(handleSize), int(baseAlignment))) * baseAlignment;

    rayGenGroupStride = groupSizeAligned;
    for (RayTracingShaderGroupPtr& shaderGroup : rayGenShaderGroups) {
        rayGenGroupStride = std::max(rayGenGroupStride, shaderGroup->getSize());
    }
    missGroupStride = groupSizeAligned;
    for (RayTracingShaderGroupPtr& shaderGroup : missShaderGroups) {
        missGroupStride = std::max(missGroupStride, shaderGroup->getSize());
    }
    hitGroupStride = groupSizeAligned;
    for (RayTracingShaderGroupPtr& shaderGroup : hitShaderGroups) {
        hitGroupStride = std::max(hitGroupStride, shaderGroup->getSize());
    }
    missGroupsOffset = rayGenGroupStride * uint32_t(rayGenShaderGroups.size());
    hitGroupsOffset = missGroupsOffset + missGroupStride * uint32_t(missShaderGroups.size());

    for (RayTracingShaderGroupPtr& shaderGroup : rayGenShaderGroups) {
        shaderGroups.push_back(shaderGroup->getShaderGroupCreateInfo());
    }
    for (RayTracingShaderGroupPtr& shaderGroup : missShaderGroups) {
        shaderGroups.push_back(shaderGroup->getShaderGroupCreateInfo());
  }
    for (RayTracingShaderGroupPtr& shaderGroup : hitShaderGroups) {
        shaderGroups.push_back(shaderGroup->getShaderGroupCreateInfo());
    }
    for (RayTracingShaderGroupPtr& shaderGroup : callableShaderGroups) {
        shaderGroups.push_back(shaderGroup->getShaderGroupCreateInfo());
    }
}

void ShaderBindingTable::buildShaderBindingTable(VkPipeline pipeline) {
    Device* device = shaderStages->getDevice();
    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rayTracingPipelineProperties =
            device->getPhysicalDeviceRayTracingPipelineProperties();
    const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
    const uint32_t handleAlignment = rayTracingPipelineProperties.shaderGroupHandleAlignment;
    const uint32_t handleSizeAligned = uint32_t(sgl::iceil(int(handleSize), int(handleAlignment))) * handleAlignment;

    const uint32_t shaderBindingTableSize = uint32_t(shaderGroups.size()) * handleSizeAligned;
    std::vector<uint8_t> shaderGroupHandleData(shaderBindingTableSize);
    auto _vkGetRayTracingShaderGroupHandlesKHR =
            (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetInstanceProcAddr(
                    device->getInstance()->getVkInstance(), "vkGetRayTracingShaderGroupHandlesKHR");
    if (_vkGetRayTracingShaderGroupHandlesKHR != nullptr) {
        if (_vkGetRayTracingShaderGroupHandlesKHR(
                device->getVkDevice(), pipeline, 0, uint32_t(shaderGroups.size()),
                shaderBindingTableSize, shaderGroupHandleData.data()) != VK_SUCCESS) {
            sgl::Logfile::get()->writeError(
                    "Error in RayTracingPipeline::RayTracingPipeline: Failed to retrieve shader group handles.");
        }
    } else {
        sgl::Logfile::get()->writeError(
                "Error in RayTracingPipeline::RayTracingPipeline: Missing vkGetRayTracingShaderGroupHandlesKHR.");
    }

    size_t sbtBufferSize =
            rayGenGroupStride * rayGenShaderGroups.size()
            + missGroupStride * missShaderGroups.size()
            + hitGroupStride * hitShaderGroups.size();
    sbtBuffer = std::make_shared<Buffer>(
            device, sbtBufferSize,
            VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR
            | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU);
    uint8_t* sbtPtr = reinterpret_cast<uint8_t*>(sbtBuffer->mapMemory());
    uint8_t* shaderGroupHandlePtr = shaderGroupHandleData.data();
    memset(sbtPtr, 0, sbtBuffer->getSizeInBytes());
    uint8_t* sbtDataOffset = sbtPtr;
    for (auto& shaderGroup : rayGenShaderGroups) {
        memcpy(sbtDataOffset, shaderGroupHandlePtr, handleSize);
        if (shaderGroup->hasRecordData()) {
            memcpy(sbtDataOffset + handleSize, shaderGroup->getRecordData(), shaderGroup->getRecordDataSize());
        }
        shaderGroupHandlePtr += handleSize;
        sbtDataOffset += rayGenGroupStride;
    }
    for (auto& shaderGroup : missShaderGroups) {
        memcpy(sbtDataOffset, shaderGroupHandlePtr, handleSize);
        if (shaderGroup->hasRecordData()) {
            memcpy(sbtDataOffset + handleSize, shaderGroup->getRecordData(), shaderGroup->getRecordDataSize());
        }
        shaderGroupHandlePtr += handleSize;
        sbtDataOffset += missGroupStride;
    }
    for (auto& shaderGroup : hitShaderGroups) {
        memcpy(sbtDataOffset, shaderGroupHandlePtr, handleSize);
        if (shaderGroup->hasRecordData()) {
            memcpy(sbtDataOffset + handleSize, shaderGroup->getRecordData(), shaderGroup->getRecordDataSize());
        }
        shaderGroupHandlePtr += handleSize;
        sbtDataOffset += hitGroupStride;
    }
    sbtBuffer->unmapMemory();

    sbtAddress = sbtBuffer->getVkDeviceAddress();
}

std::array<VkStridedDeviceAddressRegionKHR, 4> ShaderBindingTable::getStridedDeviceAddressRegions(
        const ShaderGroupSettings& shaderGroupSettings) const {
    std::array<VkStridedDeviceAddressRegionKHR, 4> stridedDeviceAddressRegions{};

    uint32_t missShaderGroupSize = shaderGroupSettings.missShaderGroupSize;
    if (missShaderGroupSize == std::numeric_limits<uint32_t>::max()) {
        missShaderGroupSize = uint32_t(missShaderGroups.size());
    }
    uint32_t hitShaderGroupSize = shaderGroupSettings.hitShaderGroupSize;
    if (hitShaderGroupSize == std::numeric_limits<uint32_t>::max()) {
        hitShaderGroupSize = uint32_t(hitShaderGroups.size());
    }

    // RayGen
    VkStridedDeviceAddressRegionKHR region;
    region.deviceAddress = sbtAddress + shaderGroupSettings.rayGenShaderIndex * rayGenGroupStride;
    region.stride = rayGenGroupStride;
    region.size = rayGenGroupStride;
    stridedDeviceAddressRegions.at(0) = region;

    // Miss
    region.deviceAddress = sbtAddress + missGroupsOffset + shaderGroupSettings.missShaderGroupOffset * missGroupStride;
    region.stride = missGroupStride;
    region.size = missGroupStride * missShaderGroupSize;
    stridedDeviceAddressRegions.at(1) = region;

    // Hit
    region.deviceAddress = sbtAddress + hitGroupsOffset + shaderGroupSettings.hitShaderGroupOffset * hitGroupStride;
    region.stride = hitGroupStride;
    region.size = hitGroupStride * hitShaderGroupSize;
    stridedDeviceAddressRegions.at(2) = region;

    // Callable (not supported so far).
    stridedDeviceAddressRegions.at(3) = VkStridedDeviceAddressRegionKHR{0u, 0u, 0u};

    return stridedDeviceAddressRegions;
}

ShaderBindingTable ShaderBindingTable::generateSimpleShaderBindingTable(
        const ShaderStagesPtr& shaderStages, VkRayTracingShaderGroupTypeKHR shaderGroupType) {
    ShaderBindingTable sbt(shaderStages);
    sbt.missGroupStride = 0;

    bool hasHitGroup = false;
    bool hasClosestHit = false;
    bool hasAnyHit = false;
    bool hasIntersection = false;

    uint32_t shaderModuleIdx = 0;
    for (const ShaderModulePtr& shaderModule : shaderStages->getShaderModules()) {
        ShaderModuleType shaderModuleType = shaderModule->getShaderModuleType();
        if (shaderModuleType == ShaderModuleType::RAYGEN) {
            sbt.addRayGenShaderGroup()->setRayGenShader(shaderModuleIdx);
        } else if (shaderModuleType == ShaderModuleType::MISS) {
            sbt.addMissShaderGroup()->setMissShader(shaderModuleIdx);
        } else if (shaderModuleType == ShaderModuleType::CALLABLE) {
            sbt.addCallableShaderGroup()->setCallable(shaderModuleIdx);
        } else if (shaderModuleType == ShaderModuleType::CLOSEST_HIT) {
            if (hasClosestHit) {
                Logfile::get()->throwError(
                        "Error in ShaderBindingTable::generateSimpleShaderBindingTable: Simple shader binding "
                        "table generation does not support more than one CLOSEST_HIT shader!");
            }
            hasHitGroup = true;
            hasClosestHit = true;
        } else if (shaderModuleType == ShaderModuleType::ANY_HIT) {
            if (hasAnyHit) {
                Logfile::get()->throwError(
                        "Error in ShaderBindingTable::generateSimpleShaderBindingTable: Simple shader binding "
                        "table generation does not support more than one ANY_HIT shader!");
            }
            hasHitGroup = true;
            hasAnyHit = true;
        } else if (shaderModuleType == ShaderModuleType::INTERSECTION) {
            if (hasIntersection) {
                Logfile::get()->throwError(
                        "Error in ShaderBindingTable::generateSimpleShaderBindingTable: Simple shader binding "
                        "table generation does not support more than one INTERSECTION shader!");
            }
            hasHitGroup = true;
            hasIntersection = true;
        } else {
            Logfile::get()->throwError(
                    "Error in ShaderBindingTable::generateSimpleShaderBindingTable: Only ray tracing shader "
                    "modules are supported!");
        }
        shaderModuleIdx++;
    }

    if (hasHitGroup) {
        shaderModuleIdx = 0;
        HitShaderGroup* hitShaderGroup = sbt.addHitShaderGroup(shaderGroupType);
        for (const ShaderModulePtr& shaderModule : shaderStages->getShaderModules()) {
            ShaderModuleType shaderModuleType = shaderModule->getShaderModuleType();
            if (shaderModuleType == ShaderModuleType::CLOSEST_HIT) {
                hitShaderGroup->setClosestHitShader(shaderModuleIdx);
            } else if (shaderModuleType == ShaderModuleType::ANY_HIT) {
                hitShaderGroup->setAnyHitShader(shaderModuleIdx);
            } else if (shaderModuleType == ShaderModuleType::INTERSECTION) {
                hitShaderGroup->setIntersectionShader(shaderModuleIdx);
            }
            shaderModuleIdx++;
        }
    }

    return sbt;
}


RayTracingPipelineInfo::RayTracingPipelineInfo(const ShaderBindingTable& table)
        : sbt(table), shaderStages(table.getShaderStages()) {
    assert(shaderStages.get() != nullptr);
    sbt.buildShaderGroups();
    reset();
}

void RayTracingPipelineInfo::reset() {
    maxPipelineRayRecursionDepth = 1;
}

RayTracingPipeline::RayTracingPipeline(Device* device, const RayTracingPipelineInfo& pipelineInfo)
        : Pipeline(device, pipelineInfo.shaderStages), sbt(pipelineInfo.sbt) {
    createPipelineLayout();

    const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages = pipelineInfo.shaderStages->getVkShaderStages();

#if VK_VERSION_1_2 && VK_HEADER_VERSION >= 162
    const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& rayTracingPipelineProperties =
            device->getPhysicalDeviceRayTracingPipelineProperties();

    if (pipelineInfo.maxPipelineRayRecursionDepth > rayTracingPipelineProperties.maxRayRecursionDepth) {
        Logfile::get()->throwError(
                "Error in RayTracingPipelineInfo::RayTracingPipelineInfo: The maximum pipeline ray recursion depth"
                "is larger than the maximum ray recursion depth supported.");
    }

    const std::vector<VkRayTracingShaderGroupCreateInfoKHR>& shaderGroups = sbt.getShaderGroupCreateInfoList();

    VkRayTracingPipelineCreateInfoKHR pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipelineCreateInfo.flags = 0;
    pipelineCreateInfo.stageCount = uint32_t(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();
    pipelineCreateInfo.groupCount = uint32_t(shaderGroups.size());
    pipelineCreateInfo.pGroups = shaderGroups.data();
    pipelineCreateInfo.maxPipelineRayRecursionDepth = pipelineInfo.maxPipelineRayRecursionDepth;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    if (vkCreateRayTracingPipelinesKHR(
            device->getVkDevice(), VK_NULL_HANDLE, VK_NULL_HANDLE,
            1, &pipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in RayTracingPipeline::RayTracingPipeline: Could not create a RayTracing pipeline.");
    }

    sbt.buildShaderBindingTable(pipeline);
#endif
}

}}
