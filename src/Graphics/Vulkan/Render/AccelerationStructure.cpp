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

#include <cstring>
#include <Utils/File/Logfile.hpp>
#include <Graphics/Vulkan/Utils/Device.hpp>
#include <Graphics/Vulkan/Buffers/Buffer.hpp>
#include <memory>
#include "Helpers.hpp"
#include "AccelerationStructure.hpp"

namespace sgl { namespace vk {

BottomLevelAccelerationStructureInput::BottomLevelAccelerationStructureInput(
        Device* device, VkGeometryFlagsKHR geometryFlags) : device(device) {
    asGeometry.flags = geometryFlags;
}

std::vector<BottomLevelAccelerationStructurePtr> buildBottomLevelAccelerationStructuresFromInputsLists(
        const std::vector<BottomLevelAccelerationStructureInputList>& blasInputsList,
        VkBuildAccelerationStructureFlagsKHR flags) {
    Device* device = blasInputsList.front().front().getDevice();

    size_t numBlases = blasInputsList.size();
    std::vector<std::vector<VkAccelerationStructureGeometryKHR>> asGeometriesList(numBlases);
    std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildInfos(numBlases);
    std::vector<BottomLevelAccelerationStructurePtr> blases(numBlases);
    std::vector<size_t> uncompactedSizes(numBlases);

    for(size_t blasIdx = 0; blasIdx < numBlases; blasIdx++) {
        const BottomLevelAccelerationStructureInputList& blasInputs = blasInputsList.at(blasIdx);
        std::vector<VkAccelerationStructureGeometryKHR>& asGeometries = asGeometriesList.at(blasIdx);
        asGeometries.reserve(blasInputs.size());
        for (const BottomLevelAccelerationStructureInput& blasInput : blasInputs) {
            asGeometries.push_back(blasInput.getAccelerationStructureGeometry());
        }

        buildInfos.at(blasIdx).sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildInfos.at(blasIdx).flags = flags;
        buildInfos.at(blasIdx).geometryCount = uint32_t(blasInputs.size());
        buildInfos.at(blasIdx).pGeometries = asGeometries.data();
        buildInfos.at(blasIdx).mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfos.at(blasIdx).type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfos.at(blasIdx).srcAccelerationStructure = VK_NULL_HANDLE;
    }

    VkDeviceSize maxScratchSize = 0;
    for(size_t blasIdx = 0; blasIdx < numBlases; blasIdx++) {
        const BottomLevelAccelerationStructureInputList& blasInputs = blasInputsList.at(blasIdx);
        std::vector<uint32_t> numPrimitivesList(blasInputs.size());
        for (size_t inputIdx = 0; inputIdx < blasInputs.size(); inputIdx++) {
            numPrimitivesList.at(inputIdx) = blasInputs.at(inputIdx).getNumPrimitives();
        }

        // Query the memory requirements for the bottom-level acceleration structure.
        VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
        buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(
                device->getVkDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                &buildInfos.at(blasIdx), numPrimitivesList.data(), &buildSizesInfo);

        // Create the acceleration structure and the underlying memory.
        BufferPtr accelerationStructureBuffer(new Buffer(
                device, buildSizesInfo.accelerationStructureSize,
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
                | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY));

        VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
        accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        accelerationStructureCreateInfo.size = buildSizesInfo.accelerationStructureSize;
        accelerationStructureCreateInfo.buffer = accelerationStructureBuffer->getVkBuffer();

        VkAccelerationStructureKHR accelerationStructure = VK_NULL_HANDLE;
        vkCreateAccelerationStructureKHR(
                device->getVkDevice(), &accelerationStructureCreateInfo, nullptr,
                &accelerationStructure);
        buildInfos.at(blasIdx).dstAccelerationStructure = accelerationStructure;
        maxScratchSize = std::max(maxScratchSize, buildSizesInfo.buildScratchSize);
        uncompactedSizes.at(blasIdx) = buildSizesInfo.accelerationStructureSize;

        BottomLevelAccelerationStructurePtr blas(new BottomLevelAccelerationStructure(
                device, accelerationStructure, accelerationStructureBuffer));
        blases.at(blasIdx) = blas;
    }

    // Allocate a scratch buffer for holding the temporary memory needed by the AS builder.
    BufferPtr scratchBuffer(new Buffer(
            device, maxScratchSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY));
    VkDeviceAddress scratchBufferDeviceAddress = scratchBuffer->getVkDeviceAddress();

    bool shallDoCompaction =
            (flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR)
            == VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;

    VkQueryPoolCreateInfo queryPoolCreateInfo{};
    queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    queryPoolCreateInfo.queryCount = numBlases;
    queryPoolCreateInfo.queryType  = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;

    VkQueryPool queryPool = VK_NULL_HANDLE;
    vkCreateQueryPool(device->getVkDevice(), &queryPoolCreateInfo, nullptr, &queryPool);

    std::vector<VkCommandBuffer> commandBuffers = device->beginSingleTimeMultipleCommands(uint32_t(numBlases));
    for (size_t blasIdx = 0; blasIdx < numBlases; blasIdx++) {
        VkCommandBuffer commandBuffer = commandBuffers.at(blasIdx);
        const BottomLevelAccelerationStructureInputList& blasInputs = blasInputsList.at(blasIdx);
        const BottomLevelAccelerationStructurePtr& blas = blases.at(blasIdx);

        buildInfos.at(blasIdx).scratchData.deviceAddress = scratchBufferDeviceAddress;

        std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> buildRangeInfos;
        buildRangeInfos.reserve(blasInputs.size());
        for (const BottomLevelAccelerationStructureInput& blasInput : blasInputs) {
            buildRangeInfos.push_back(&blasInput.getBuildRangeInfo());
        }

        vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildInfos.at(blasIdx), buildRangeInfos.data());

        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                0, 1, &barrier,
                0, nullptr,
                0, nullptr);

        if(shallDoCompaction) {
            VkAccelerationStructureKHR accelerationStructure = blas->getAccelerationStructure();
            vkCmdWriteAccelerationStructuresPropertiesKHR(
                    commandBuffer, 1, &accelerationStructure,
                    VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPool, blasIdx);
        }
    }
    device->endSingleTimeMultipleCommands(commandBuffers);

    if(shallDoCompaction) {
        VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();

        std::vector<VkDeviceSize> compactSizes(numBlases);
        vkGetQueryPoolResults(
                device->getVkDevice(), queryPool, 0,
                uint32_t(compactSizes.size()), compactSizes.size() * sizeof(VkDeviceSize),
                compactSizes.data(), sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT);

        std::vector<BottomLevelAccelerationStructurePtr> blasesOld(numBlases);
        uint32_t totalUncompactedSize = 0, totalCompactedSize = 0;
        for (size_t blasIdx = 0; blasIdx < numBlases; blasIdx++) {
            totalUncompactedSize += uint32_t(uncompactedSizes.at(blasIdx));
            totalCompactedSize += uint32_t(compactSizes.at(blasIdx));

            // Create the compacted acceleration structure and the underlying memory.
            BufferPtr accelerationStructureBuffer(new Buffer(
                    device, compactSizes.at(blasIdx),
                    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
                    | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                    VMA_MEMORY_USAGE_GPU_ONLY));

            VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
            accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
            accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            accelerationStructureCreateInfo.size = compactSizes.at(blasIdx);
            accelerationStructureCreateInfo.buffer = accelerationStructureBuffer->getVkBuffer();

            VkAccelerationStructureKHR accelerationStructure = VK_NULL_HANDLE;
            vkCreateAccelerationStructureKHR(
                    device->getVkDevice(), &accelerationStructureCreateInfo, nullptr,
                    &accelerationStructure);

            // Copy the original BLAS to a compact version
            VkCopyAccelerationStructureInfoKHR copyAccelerationStructureInfo{};
            copyAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
            copyAccelerationStructureInfo.src = blases.at(blasIdx)->getAccelerationStructure();
            copyAccelerationStructureInfo.dst = accelerationStructure;
            copyAccelerationStructureInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
            vkCmdCopyAccelerationStructureKHR(commandBuffer, &copyAccelerationStructureInfo);

            BottomLevelAccelerationStructurePtr blas(new BottomLevelAccelerationStructure(
                    device, accelerationStructure, accelerationStructureBuffer));
            blasesOld.at(blasIdx) = blases.at(blasIdx);
            blases.at(blasIdx) = blas;
        }
        device->endSingleTimeCommands(commandBuffer);

        blasesOld.clear();

        Logfile::get()->writeInfo(
                "BLAS: Reducing from " + std::to_string(totalUncompactedSize) + " to "
                + std::to_string(totalCompactedSize) + ".");
    }

    vkDestroyQueryPool(device->getVkDevice(), queryPool, nullptr);

    return blases;
}

std::vector<BottomLevelAccelerationStructurePtr> buildBottomLevelAccelerationStructuresFromInputList(
        const std::vector<BottomLevelAccelerationStructureInput>& blasInputsList,
        VkBuildAccelerationStructureFlagsKHR flags) {
    return buildBottomLevelAccelerationStructuresFromInputsLists({ blasInputsList }, flags);
}

BottomLevelAccelerationStructurePtr buildBottomLevelAccelerationStructureFromInputs(
        const BottomLevelAccelerationStructureInputList& blasInputs,
        VkBuildAccelerationStructureFlagsKHR flags) {
    return buildBottomLevelAccelerationStructuresFromInputsLists({ blasInputs }, flags).front();
}

BottomLevelAccelerationStructurePtr buildBottomLevelAccelerationStructureFromInput(
        const BottomLevelAccelerationStructureInput& blasInput,
        VkBuildAccelerationStructureFlagsKHR flags) {
    return buildBottomLevelAccelerationStructuresFromInputsLists({ { blasInput } }, flags).front();
}


TrianglesAccelerationStructureInput::TrianglesAccelerationStructureInput(
        Device* device, VkGeometryFlagsKHR geometryFlags) : BottomLevelAccelerationStructureInput(device, geometryFlags) {
    asGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;

    VkAccelerationStructureGeometryTrianglesDataKHR& trianglesData = asGeometry.geometry.triangles;
    trianglesData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    trianglesData.transformData = {}; //< nullptr -> identity transform.
}

void TrianglesAccelerationStructureInput::setIndexBuffer(
        BufferPtr& buffer, VkIndexType indexType) {
    this->indexBuffer = buffer;
    this->indexType = indexType;
    this->numIndices = buffer->getSizeInBytes() / getIndexTypeByteSize(indexType);

    buildRangeInfo.firstVertex = 0;
    buildRangeInfo.primitiveCount = numIndices / 3;
    buildRangeInfo.primitiveOffset = 0;
    buildRangeInfo.transformOffset = 0;

    VkAccelerationStructureGeometryTrianglesDataKHR& trianglesData = asGeometry.geometry.triangles;
    trianglesData.indexType = indexType;
    trianglesData.indexData = { .deviceAddress = indexBuffer->getVkDeviceAddress() };
}

void TrianglesAccelerationStructureInput::setVertexBuffer(
        BufferPtr& buffer, VkFormat vertexFormat, VkDeviceSize vertexStride) {
    this->vertexBuffer = buffer;
    this->vertexFormat = vertexFormat;
    this->numVertices = buffer->getSizeInBytes() / vertexStride;
    if (vertexStride == 0) {
        if (vertexFormat == VK_FORMAT_R32G32B32_SFLOAT) {
            this->vertexStride = 3 * sizeof(float);
        } else if (vertexFormat == VK_FORMAT_R32G32B32A32_SFLOAT) {
            this->vertexStride = 4 * sizeof(float);
        } else {
            Logfile::get()->throwError(
                    "Error in TrianglesAccelerationStructure::setVertexBuffer: vertexStride == 0, "
                    "but unhandled vertex format is used.");
        }
    } else {
        this->vertexStride = vertexStride;
    }

    VkAccelerationStructureGeometryTrianglesDataKHR& trianglesData = asGeometry.geometry.triangles;
    trianglesData.vertexFormat = this->vertexFormat;
    trianglesData.vertexData = { .deviceAddress = vertexBuffer->getVkDeviceAddress() };
    trianglesData.vertexStride = this->vertexStride;
    trianglesData.maxVertex = this->numVertices;
}


AabbsAccelerationStructureInput::AabbsAccelerationStructureInput(
        Device* device, VkGeometryFlagsKHR geometryFlags) : BottomLevelAccelerationStructureInput(device, geometryFlags) {
    asGeometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;

    VkAccelerationStructureGeometryAabbsDataKHR& aabbsData = asGeometry.geometry.aabbs;
    aabbsData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
}

void AabbsAccelerationStructureInput::setAabbsBuffer(BufferPtr& buffer, VkDeviceSize stride) {
    aabbsBuffer = buffer;
    aabbsBufferStride = stride;
    numAabbs = buffer->getSizeInBytes() / stride;

    VkAccelerationStructureGeometryAabbsDataKHR& aabbsData = asGeometry.geometry.aabbs;
    aabbsData.stride = aabbsBufferStride;
    aabbsData.data = { .deviceAddress = aabbsBuffer->getVkDeviceAddress() };
    buildRangeInfo.primitiveCount = numAabbs;
}


BottomLevelAccelerationStructure::~BottomLevelAccelerationStructure() {
    vkDestroyAccelerationStructureKHR(device->getVkDevice(), accelerationStructure, nullptr);
}

VkDeviceAddress BottomLevelAccelerationStructure::getAccelerationStructureDeviceAddress() {
    VkAccelerationStructureDeviceAddressInfoKHR addressInfo{};
    addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    addressInfo.accelerationStructure = accelerationStructure;
    VkDeviceAddress blasAddress = vkGetAccelerationStructureDeviceAddressKHR(device->getVkDevice(), &addressInfo);
    return blasAddress;
}


void TopLevelAccelerationStructure::build(
        const std::vector<BottomLevelAccelerationStructurePtr>& blases,
        const std::vector<BlasInstance>& instances,
        VkBuildAccelerationStructureFlagsKHR flags) {
    if (instances.size() > device->getPhysicalDeviceAccelerationStructureProperties().maxInstanceCount) {
        Logfile::get()->throwError(
                "Error in TopLevelAccelerationStructure::build: The maximum number of supported instances is "
                + std::to_string(device->getPhysicalDeviceAccelerationStructureProperties().maxInstanceCount)
                + ". However, the number of used instances is " + std::to_string(instances.size()) + ".");
    }

    bool update = accelerationStructure != VK_NULL_HANDLE;

    VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();

    std::vector<VkAccelerationStructureInstanceKHR> asInstances;
    asInstances.reserve(instances.size());
    for (auto& instance : instances) {
        const BottomLevelAccelerationStructurePtr& blas = blases.at(instance.blasIdx);

        // Convert from column-major to row-major order.
        glm::mat4 transposedTransform = glm::transpose(instance.transform);

        VkAccelerationStructureInstanceKHR asInstance{};
        memcpy(&asInstance.transform, &transposedTransform, sizeof(VkTransformMatrixKHR));
        asInstance.instanceCustomIndex = instance.instanceCustomIndex;
        asInstance.mask = instance.mask;
        asInstance.instanceShaderBindingTableRecordOffset = instance.shaderBindingTableRecordOffset;
        asInstance.flags = instance.flags;
        asInstance.accelerationStructureReference = blas->getAccelerationStructureDeviceAddress();
        asInstances.push_back(asInstance);
    }

    // Create a buffering that stores the AS instance data.
    BufferPtr instancesBuffer(new Buffer(
            device, asInstances.size() * sizeof(VkAccelerationStructureInstanceKHR),
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY));
    instancesBuffer->uploadData(
            asInstances.size() * sizeof(VkAccelerationStructureInstanceKHR), asInstances.data(),
            commandBuffer);

    VkMemoryBarrier memoryBarrier{};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    vkCmdPipelineBarrier(
            commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            0, 1, &memoryBarrier, 0,
            nullptr, 0, nullptr);

    VkAccelerationStructureGeometryInstancesDataKHR asGeometryInstancesData{};
    asGeometryInstancesData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    asGeometryInstancesData.arrayOfPointers = VK_FALSE;
    asGeometryInstancesData.data.deviceAddress = instancesBuffer->getVkDeviceAddress();

    VkAccelerationStructureGeometryKHR topAsGeometry{};
    topAsGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    topAsGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    topAsGeometry.geometry.instances = asGeometryInstancesData;

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildInfo.flags = flags;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &topAsGeometry;
    buildInfo.mode = update
            ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR
            : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;

    uint32_t numInstances = uint32_t(instances.size());
    VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
    buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(
            device->getVkDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &buildInfo, &numInstances, &buildSizesInfo);

    if (!update) {
        // Create the acceleration structure and the underlying memory.
        accelerationStructureBuffer = std::make_shared<Buffer>(
                device, buildSizesInfo.accelerationStructureSize,
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
                | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);

        VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
        accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        accelerationStructureCreateInfo.size = buildSizesInfo.accelerationStructureSize;
        accelerationStructureCreateInfo.buffer = accelerationStructureBuffer->getVkBuffer();

        accelerationStructure = VK_NULL_HANDLE;
        vkCreateAccelerationStructureKHR(
                device->getVkDevice(), &accelerationStructureCreateInfo, nullptr,
                &accelerationStructure);
    }

    // Allocate a scratch buffer for holding the temporary memory needed by the AS builder.
    BufferPtr scratchBuffer(new Buffer(
            device, buildSizesInfo.buildScratchSize,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY));

    buildInfo.srcAccelerationStructure = update ? accelerationStructure : VK_NULL_HANDLE;
    buildInfo.dstAccelerationStructure = accelerationStructure;
    buildInfo.scratchData.deviceAddress = scratchBuffer->getVkDeviceAddress();

    VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
    buildRangeInfo.primitiveCount = static_cast<uint32_t>(instances.size());
    buildRangeInfo.primitiveOffset = 0;
    buildRangeInfo.firstVertex = 0;
    buildRangeInfo.transformOffset = 0;
    const VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfos = &buildRangeInfo;

    vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildInfo, &buildRangeInfos);

    device->endSingleTimeCommands(commandBuffer);
}

}}
