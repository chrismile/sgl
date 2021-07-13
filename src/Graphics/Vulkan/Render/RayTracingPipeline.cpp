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

RayTracingPipeline::RayTracingPipeline(Device* device, const RayTracingPipelineInfo& pipelineInfo)
        : Pipeline(device, pipelineInfo.shaderStages) {
    const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts =
            pipelineInfo.shaderStages->getVkDescriptorSetLayouts();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(
            device->getVkDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        Logfile::get()->throwError(
                "Error in RayTracingPipeline::RayTracingPipeline: Could not create a pipeline layout.");
    }

    auto& shaderModules = pipelineInfo.shaderStages->getShaderModules();
    if (shaderModules.size() != 1 || shaderModules.front()->getShaderModuleType() != ShaderModuleType::COMPUTE) {
        Logfile::get()->throwError(
                "Error in RayTracingPipeline::RayTracingPipeline: Expected exactly one compute shader module.");
    }

    const std::vector<VkPipelineShaderStageCreateInfo>& shaderStages = pipelineInfo.shaderStages->getVkShaderStages();

    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;

    VkRayTracingPipelineCreateInfoKHR pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipelineCreateInfo.flags = 0;
    pipelineCreateInfo.stageCount = uint32_t(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();
    pipelineCreateInfo.groupCount = uint32_t(shaderGroups.size());
    pipelineCreateInfo.pGroups = shaderGroups.data();
    pipelineCreateInfo.maxPipelineRayRecursionDepth;
    pipelineCreateInfo.pLibraryInfo;
    pipelineCreateInfo.pLibraryInterface;
    pipelineCreateInfo.pDynamicState;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = -1;

    //if (vkCreateRayTracingPipelinesKHR(
    //        device->getVkDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo,
    //        nullptr, &pipeline) != VK_SUCCESS) {
    //    Logfile::get()->throwError(
    //            "Error in RayTracingPipeline::RayTracingPipeline: Could not create a RayTracing pipeline.");
    //}

    VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProperties = {};
    accelerationStructureProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties = {};
    rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    rayTracingPipelineProperties.pNext = &accelerationStructureProperties;
    VkPhysicalDeviceProperties2 deviceProperties2 = {};
    deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProperties2.pNext = &rayTracingPipelineProperties;
    vkGetPhysicalDeviceProperties2(device->getVkPhysicalDevice(), &deviceProperties2);

    //accelerationStructureProperties.maxInstanceCount;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {};
    accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &accelerationStructureFeatures;
    vkGetPhysicalDeviceFeatures2(device->getVkPhysicalDevice(), &deviceFeatures2);

    const uint32_t handleSize = rayTracingPipelineProperties.shaderGroupHandleSize;
    const uint32_t handleSizeAligned = uint32_t(sgl::iceil(
            int(rayTracingPipelineProperties.shaderGroupHandleSize),
            int(rayTracingPipelineProperties.shaderGroupHandleAlignment)))
                    * rayTracingPipelineProperties.shaderGroupHandleAlignment;

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

    BufferPtr buffer(new Buffer(
            device, rayTracingPipelineProperties.shaderGroupHandleSize,
            VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR
            | VK_BUFFER_USAGE_TRANSFER_SRC_BIT
            | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU));


    // Build acceleration structure.
    VkBufferDeviceAddressInfo bufferDeviceAddressInfo;
    bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAddressInfo.buffer = VK_NULL_HANDLE; // TODO
    VkDeviceAddress vertexBufferDeviceAddress = vkGetBufferDeviceAddress(device->getVkDevice(), &bufferDeviceAddressInfo);

    // TODO: Do the same for the index buffer.
    /*VkAccelerationStructureGeometryTrianglesDataKHR trianglesData = {};
    trianglesData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    trianglesData.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT; // vec3 data
    trianglesData.vertexData = { .deviceAddress = vertexBufferDeviceAddress };
    trianglesData.vertexStride = 3 * sizeof(float);
    trianglesData.indexType = VK_INDEX_TYPE_UINT32;
    trianglesData.indexData = TODO;
    trianglesData.transformData = {};
    trianglesData.maxVertex = numVertices;

    VkAccelerationStructureGeometryKHR asGeometry = {};
    asGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR; // TODO: or VK_GEOMETRY_TYPE_AABBS_KHR
    asGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR; // VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR: Call any-hit once.
    asGeometry.geometry.triangles = trianglesData;

    VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo = {};
    buildRangeInfo.firstVertex = 0;
    buildRangeInfo.primitiveCount = numPrimitives; // numIndices / 3
    buildRangeInfo.primitiveOffset = 0;
    buildRangeInfo.transformOffset = 0;

    VkRayTracingBuilder;*/
}

RayTracingPipeline::~RayTracingPipeline() {
}

}}
