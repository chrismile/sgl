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

#include <algorithm>
#include <cstring>
#include <memory>

#include <Utils/Convert.hpp>
#include <Utils/File/Logfile.hpp>
#include <Graphics/Vulkan/Utils/Device.hpp>
#include <Graphics/Vulkan/Buffers/Buffer.hpp>

#include "Helpers.hpp"
#include "AccelerationStructure.hpp"

namespace sgl { namespace vk {

BottomLevelAccelerationStructureInput::BottomLevelAccelerationStructureInput(
        Device* device, VkGeometryFlagsKHR geometryFlags) : device(device) {
    asGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    asGeometry.flags = geometryFlags;
}

std::vector<BottomLevelAccelerationStructurePtr> buildBottomLevelAccelerationStructuresFromInputsLists(
        const std::vector<BottomLevelAccelerationStructureInputList>& blasInputsList,
        VkBuildAccelerationStructureFlagsKHR flags, bool debugOutput) {
    Device* device = blasInputsList.front().front()->getDevice();

    size_t numBlases = blasInputsList.size();
    std::vector<std::vector<VkAccelerationStructureGeometryKHR>> asGeometriesList(numBlases);
    std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildInfos(numBlases);
    std::vector<BottomLevelAccelerationStructurePtr> blases(numBlases);
    std::vector<size_t> uncompactedSizes(numBlases);

    for (size_t blasIdx = 0; blasIdx < numBlases; blasIdx++) {
        const BottomLevelAccelerationStructureInputList& blasInputs = blasInputsList.at(blasIdx);
        std::vector<VkAccelerationStructureGeometryKHR>& asGeometries = asGeometriesList.at(blasIdx);
        asGeometries.reserve(blasInputs.size());
        for (const BottomLevelAccelerationStructureInputPtr& blasInput : blasInputs) {
            asGeometries.push_back(blasInput->getAccelerationStructureGeometry());
        }

        buildInfos.at(blasIdx).sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildInfos.at(blasIdx).flags = flags;
        buildInfos.at(blasIdx).geometryCount = uint32_t(blasInputs.size());
        buildInfos.at(blasIdx).pGeometries = asGeometries.data();
        buildInfos.at(blasIdx).mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfos.at(blasIdx).type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfos.at(blasIdx).srcAccelerationStructure = VK_NULL_HANDLE;
    }

    VkDeviceSize minAccelerationStructureScratchOffsetAlignment =
            device->getPhysicalDeviceAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment;

    VkDeviceSize maxScratchSize = 0;
    for (size_t blasIdx = 0; blasIdx < numBlases; blasIdx++) {
        const BottomLevelAccelerationStructureInputList& blasInputs = blasInputsList.at(blasIdx);
        std::vector<uint32_t> numPrimitivesList(blasInputs.size());
        for (size_t inputIdx = 0; inputIdx < blasInputs.size(); inputIdx++) {
            numPrimitivesList.at(inputIdx) = uint32_t(blasInputs.at(inputIdx)->getNumPrimitives());
        }

        // Query the memory requirements for the bottom-level acceleration structure.
        VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
        buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(
                device->getVkDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                &buildInfos.at(blasIdx), numPrimitivesList.data(), &buildSizesInfo);

        if (debugOutput) {
            sgl::Logfile::get()->writeInfo(
                    "Acceleration structure build scratch size: "
                    + sgl::toString(double(buildSizesInfo.buildScratchSize) / 1024.0 / 1024.0) + "MiB");
            sgl::Logfile::get()->writeInfo(
                    "Acceleration structure size: "
                    + sgl::toString(double(buildSizesInfo.accelerationStructureSize) / 1024.0 / 1024.0) + "MiB");
        }

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
        maxScratchSize = std::max(
                maxScratchSize, buildSizesInfo.buildScratchSize + minAccelerationStructureScratchOffsetAlignment);
        uncompactedSizes.at(blasIdx) = buildSizesInfo.accelerationStructureSize;

        BottomLevelAccelerationStructurePtr blas(new BottomLevelAccelerationStructure(
                device, accelerationStructure, accelerationStructureBuffer,
                buildSizesInfo.accelerationStructureSize));
        blases.at(blasIdx) = blas;
    }

    // Allocate a scratch buffer for holding the temporary memory needed by the AS builder.
    BufferPtr scratchBuffer(new Buffer(
            device, maxScratchSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY));
    VkDeviceAddress scratchBufferDeviceAddress = scratchBuffer->getVkDeviceAddress();
    VkDeviceSize alignmentOffset = scratchBufferDeviceAddress % minAccelerationStructureScratchOffsetAlignment;
    if (alignmentOffset != 0) {
        // This was necessary on AMD hardware.
        scratchBufferDeviceAddress += minAccelerationStructureScratchOffsetAlignment - alignmentOffset;
    }

    bool shallDoCompaction =
            (flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR)
            == VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;

    VkQueryPoolCreateInfo queryPoolCreateInfo{};
    queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    queryPoolCreateInfo.queryCount = uint32_t(numBlases);
    queryPoolCreateInfo.queryType  = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
    VkQueryPool queryPool = VK_NULL_HANDLE;
    vkCreateQueryPool(device->getVkDevice(), &queryPoolCreateInfo, nullptr, &queryPool);
    //vkResetQueryPool(device->getVkDevice(), queryPool, 0, uint32_t(numBlases));
    if (shallDoCompaction) {
        VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();
        vkCmdResetQueryPool(commandBuffer, queryPool, 0, uint32_t(numBlases));
        device->endSingleTimeCommands(commandBuffer);
    }

    std::vector<VkCommandBuffer> commandBuffers = device->beginSingleTimeMultipleCommands(
            uint32_t(numBlases));
    for (size_t blasIdx = 0; blasIdx < numBlases; blasIdx++) {
        VkCommandBuffer commandBuffer = commandBuffers.at(blasIdx);
        const BottomLevelAccelerationStructureInputList& blasInputs = blasInputsList.at(blasIdx);
        const BottomLevelAccelerationStructurePtr& blas = blases.at(blasIdx);

        buildInfos.at(blasIdx).scratchData.deviceAddress = scratchBufferDeviceAddress;

        std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> buildRangeInfos;
        buildRangeInfos.reserve(blasInputs.size());
        for (const BottomLevelAccelerationStructureInputPtr& blasInput : blasInputs) {
            buildRangeInfos.push_back(&blasInput->getBuildRangeInfo());
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
                    VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPool, uint32_t(blasIdx));
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
        size_t totalUncompactedSize = 0, totalCompactedSize = 0;
        for (size_t blasIdx = 0; blasIdx < numBlases; blasIdx++) {
            totalUncompactedSize += uncompactedSizes.at(blasIdx);
            totalCompactedSize += compactSizes.at(blasIdx);

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
                    device, accelerationStructure, accelerationStructureBuffer,
                    compactSizes.at(blasIdx)));
            blasesOld.at(blasIdx) = blases.at(blasIdx);
            blases.at(blasIdx) = blas;
        }
        device->endSingleTimeCommands(commandBuffer);

        blasesOld.clear();

        Logfile::get()->writeInfo(
                "BLAS: Reducing from " + sgl::toString(double(totalUncompactedSize) / 1024.0 / 1024.0)
                + "MiB to " + sgl::toString(double(totalCompactedSize) / 1024.0 / 1024.0) + "MiB.");
    }

    vkDestroyQueryPool(device->getVkDevice(), queryPool, nullptr);

    return blases;
}

std::vector<BottomLevelAccelerationStructurePtr> buildBottomLevelAccelerationStructuresFromInputList(
        const std::vector<BottomLevelAccelerationStructureInputPtr>& blasInputs,
        VkBuildAccelerationStructureFlagsKHR flags, bool debugOutput) {
    std::vector<BottomLevelAccelerationStructureInputList> blasInputsList;
    for (const BottomLevelAccelerationStructureInputPtr& blasInput : blasInputs) {
        blasInputsList.push_back({ blasInput });
    }
    return buildBottomLevelAccelerationStructuresFromInputsLists(
            { blasInputsList }, flags, debugOutput);
}

BottomLevelAccelerationStructurePtr buildBottomLevelAccelerationStructureFromInputs(
        const BottomLevelAccelerationStructureInputList& blasInputList,
        VkBuildAccelerationStructureFlagsKHR flags, bool debugOutput) {
    return buildBottomLevelAccelerationStructuresFromInputsLists(
            { blasInputList }, flags, debugOutput).front();
}

BottomLevelAccelerationStructurePtr buildBottomLevelAccelerationStructureFromInput(
        const BottomLevelAccelerationStructureInputPtr& blasInput,
        VkBuildAccelerationStructureFlagsKHR flags, bool debugOutput) {
    return buildBottomLevelAccelerationStructuresFromInputsLists(
            { { blasInput } }, flags, debugOutput).front();
}


std::vector<BottomLevelAccelerationStructurePtr> buildBottomLevelAccelerationStructuresFromInputsListsBatched(
        const std::vector<BottomLevelAccelerationStructureInputList>& blasInputsList,
        VkBuildAccelerationStructureFlagsKHR flags, bool debugOutput) {
    Device* device = blasInputsList.front().front()->getDevice();

    size_t numBlases = blasInputsList.size();
    std::vector<std::vector<VkAccelerationStructureGeometryKHR>> asGeometriesList(numBlases);
    std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildInfos(numBlases);
    std::vector<BottomLevelAccelerationStructurePtr> blases(numBlases);
    std::vector<size_t> uncompactedSizes(numBlases);
    std::vector<VkAccelerationStructureBuildSizesInfoKHR> buildSizesInfoList(numBlases);

    VkDeviceSize maxScratchSize = 0;
    VkDeviceSize minAccelerationStructureScratchOffsetAlignment =
            device->getPhysicalDeviceAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment;

    for (size_t blasIdx = 0; blasIdx < numBlases; blasIdx++) {
        const BottomLevelAccelerationStructureInputList& blasInputs = blasInputsList.at(blasIdx);
        std::vector<VkAccelerationStructureGeometryKHR>& asGeometries = asGeometriesList.at(blasIdx);
        asGeometries.reserve(blasInputs.size());
        for (const BottomLevelAccelerationStructureInputPtr& blasInput : blasInputs) {
            asGeometries.push_back(blasInput->getAccelerationStructureGeometry());
        }

        buildInfos.at(blasIdx).sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildInfos.at(blasIdx).flags = flags;
        buildInfos.at(blasIdx).geometryCount = uint32_t(blasInputs.size());
        buildInfos.at(blasIdx).pGeometries = asGeometries.data();
        buildInfos.at(blasIdx).mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfos.at(blasIdx).type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfos.at(blasIdx).srcAccelerationStructure = VK_NULL_HANDLE;

        std::vector<uint32_t> numPrimitivesList(blasInputs.size());
        for (size_t inputIdx = 0; inputIdx < blasInputs.size(); inputIdx++) {
            numPrimitivesList.at(inputIdx) = uint32_t(blasInputs.at(inputIdx)->getNumPrimitives());
        }

        // Query the memory requirements for the bottom-level acceleration structure.
        VkAccelerationStructureBuildSizesInfoKHR& buildSizesInfo = buildSizesInfoList.at(blasIdx);
        buildSizesInfo = {};
        buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(
                device->getVkDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                &buildInfos.at(blasIdx), numPrimitivesList.data(), &buildSizesInfo);

        maxScratchSize = std::max(
                maxScratchSize, buildSizesInfo.buildScratchSize + minAccelerationStructureScratchOffsetAlignment);
        uncompactedSizes.at(blasIdx) = buildSizesInfo.accelerationStructureSize;
    }

    // Allocate a scratch buffer for holding the temporary memory needed by the AS builder.
    BufferPtr scratchBuffer(new Buffer(
            device, maxScratchSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY));
    VkDeviceAddress scratchBufferDeviceAddress = scratchBuffer->getVkDeviceAddress();
    VkDeviceSize alignmentOffset = scratchBufferDeviceAddress % minAccelerationStructureScratchOffsetAlignment;
    if (alignmentOffset != 0) {
        // This was necessary on AMD hardware.
        scratchBufferDeviceAddress += minAccelerationStructureScratchOffsetAlignment - alignmentOffset;
    }

    bool shallDoCompaction =
            (flags & VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR)
            == VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;

    VkQueryPoolCreateInfo queryPoolCreateInfo{};
    queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    queryPoolCreateInfo.queryCount = uint32_t(numBlases);
    queryPoolCreateInfo.queryType  = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
    VkQueryPool queryPool = VK_NULL_HANDLE;
    vkCreateQueryPool(device->getVkDevice(), &queryPoolCreateInfo, nullptr, &queryPool);
    //vkResetQueryPool(device->getVkDevice(), queryPool, 0, uint32_t(numBlases));
    if (shallDoCompaction) {
        VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();
        vkCmdResetQueryPool(commandBuffer, queryPool, 0, uint32_t(numBlases));
        device->endSingleTimeCommands(commandBuffer);
    }

    size_t totalUncompactedSize = 0, totalCompactedSize = 0;

    /*
     * If possible, try creating the BLASes in smaller batches and compact immediately afterwards to ...
     * a) ... avoid driver timeout detection.
     * b) ... avoid out of memory errors.
     */
    constexpr VkDeviceSize batchBlasLimit = 256 * 1024 * 1024; //< 256MiB
    VkDeviceSize batchBlasSize = 0;
    size_t batchBlasStartIdx = 0;
    size_t batchNumBlases = 0;
    std::vector<VkDeviceSize> compactSizes(numBlases);

    for (size_t blasIdx = 0; blasIdx < numBlases; blasIdx++) {
        const VkAccelerationStructureBuildSizesInfoKHR& buildSizesInfo = buildSizesInfoList.at(blasIdx);

        if (debugOutput) {
            sgl::Logfile::get()->writeInfo(
                    "Acceleration structure build scratch size: "
                    + sgl::toString(double(buildSizesInfo.buildScratchSize) / 1024.0 / 1024.0) + "MiB");
            sgl::Logfile::get()->writeInfo(
                    "Acceleration structure size: "
                    + sgl::toString(double(buildSizesInfo.accelerationStructureSize) / 1024.0 / 1024.0) + "MiB");
        }

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
        buildInfos.at(blasIdx).scratchData.deviceAddress = scratchBufferDeviceAddress;

        BottomLevelAccelerationStructurePtr blas(new BottomLevelAccelerationStructure(
                device, accelerationStructure, accelerationStructureBuffer,
                buildSizesInfo.accelerationStructureSize));
        blases.at(blasIdx) = blas;

        batchBlasSize += buildSizesInfo.accelerationStructureSize;
        batchNumBlases += 1;

        if (batchBlasSize >= batchBlasLimit || blasIdx == numBlases - 1) {
            std::vector<VkCommandBuffer> commandBuffers = device->beginSingleTimeMultipleCommands(
                    uint32_t(batchNumBlases));
            size_t batchBlasEndIdx = batchBlasStartIdx + batchNumBlases;
            for (size_t batchBlasIdx = batchBlasStartIdx; batchBlasIdx < batchBlasEndIdx; batchBlasIdx++) {
                VkCommandBuffer commandBuffer = commandBuffers.at(batchBlasIdx - batchBlasStartIdx);
                const BottomLevelAccelerationStructureInputList& blasInputs = blasInputsList.at(batchBlasIdx);
                const BottomLevelAccelerationStructurePtr& blas = blases.at(batchBlasIdx);

                std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> buildRangeInfos;
                buildRangeInfos.reserve(blasInputs.size());
                for (const BottomLevelAccelerationStructureInputPtr& blasInput : blasInputs) {
                    buildRangeInfos.push_back(&blasInput->getBuildRangeInfo());
                }

                vkCmdBuildAccelerationStructuresKHR(
                        commandBuffer, 1, &buildInfos.at(batchBlasIdx), buildRangeInfos.data());

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
                            VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPool, uint32_t(batchBlasIdx));
                }
            }
            device->endSingleTimeMultipleCommands(commandBuffers);

            if (shallDoCompaction) {
                VkCommandBuffer commandBuffer = device->beginSingleTimeCommands();

                vkGetQueryPoolResults(
                        device->getVkDevice(), queryPool, batchBlasStartIdx,
                        uint32_t(batchNumBlases), batchNumBlases * sizeof(VkDeviceSize),
                        compactSizes.data() + batchBlasStartIdx, sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT);

                std::vector<BottomLevelAccelerationStructurePtr> blasesOld(batchNumBlases);
                size_t batchUncompactedSize = 0, batchCompactedSize = 0;
                for (size_t batchBlasIdx = batchBlasStartIdx; batchBlasIdx < batchBlasEndIdx; batchBlasIdx++) {
                    totalUncompactedSize += uncompactedSizes.at(batchBlasIdx);
                    totalCompactedSize += compactSizes.at(batchBlasIdx);
                    batchUncompactedSize += uncompactedSizes.at(batchBlasIdx);
                    batchCompactedSize += compactSizes.at(batchBlasIdx);

                    // Create the compacted acceleration structure and the underlying memory.
                    BufferPtr accelerationStructureBuffer(new Buffer(
                            device, compactSizes.at(batchBlasIdx),
                            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
                            | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                            VMA_MEMORY_USAGE_GPU_ONLY));

                    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
                    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
                    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
                    accelerationStructureCreateInfo.size = compactSizes.at(batchBlasIdx);
                    accelerationStructureCreateInfo.buffer = accelerationStructureBuffer->getVkBuffer();

                    VkAccelerationStructureKHR accelerationStructure = VK_NULL_HANDLE;
                    vkCreateAccelerationStructureKHR(
                            device->getVkDevice(), &accelerationStructureCreateInfo, nullptr,
                            &accelerationStructure);

                    // Copy the original BLAS to a compact version.
                    VkCopyAccelerationStructureInfoKHR copyAccelerationStructureInfo{};
                    copyAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR;
                    copyAccelerationStructureInfo.src = blases.at(batchBlasIdx)->getAccelerationStructure();
                    copyAccelerationStructureInfo.dst = accelerationStructure;
                    copyAccelerationStructureInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
                    vkCmdCopyAccelerationStructureKHR(commandBuffer, &copyAccelerationStructureInfo);

                    BottomLevelAccelerationStructurePtr blas(new BottomLevelAccelerationStructure(
                            device, accelerationStructure, accelerationStructureBuffer,
                            compactSizes.at(batchBlasIdx)));
                    blasesOld.at(batchBlasIdx) = blases.at(batchBlasIdx);
                    blases.at(batchBlasIdx) = blas;
                }
                if (debugOutput) {
                    Logfile::get()->writeInfo(
                            "BLAS: Reducing batch from "
                            + sgl::toString(double(batchUncompactedSize) / 1024.0 / 1024.0) + "MiB to "
                            + sgl::toString(double(batchCompactedSize) / 1024.0 / 1024.0) + "MiB.");
                }
                device->endSingleTimeCommands(commandBuffer);

                blasesOld.clear();
            } else {
                for (size_t batchBlasIdx = batchBlasStartIdx; batchBlasIdx < batchNumBlases; batchBlasIdx++) {
                    totalUncompactedSize += uncompactedSizes.at(batchBlasIdx);
                }
            }

            batchBlasSize = 0;
            batchNumBlases = 0;
            batchBlasStartIdx = blasIdx + 1;
        }
    }

    if (debugOutput && shallDoCompaction) {
        Logfile::get()->writeInfo(
                "BLAS: Reduced from "
                + sgl::toString(double(totalUncompactedSize) / 1024.0 / 1024.0) + "MiB to "
                + sgl::toString(double(totalCompactedSize) / 1024.0 / 1024.0) + "MiB.");
    }
    if (debugOutput && !shallDoCompaction) {
        Logfile::get()->writeInfo(
                "BLAS: Created acceleration structures of size "
                + sgl::toString(double(totalUncompactedSize) / 1024.0 / 1024.0) + "MiB.");
    }

    vkDestroyQueryPool(device->getVkDevice(), queryPool, nullptr);

    return blases;
}

std::vector<BottomLevelAccelerationStructurePtr> buildBottomLevelAccelerationStructuresFromInputListBatched(
        const std::vector<BottomLevelAccelerationStructureInputPtr>& blasInputs,
        VkBuildAccelerationStructureFlagsKHR flags, bool debugOutput) {
    std::vector<BottomLevelAccelerationStructureInputList> blasInputsList;
    for (const BottomLevelAccelerationStructureInputPtr& blasInput : blasInputs) {
        blasInputsList.push_back({ blasInput });
    }
    return buildBottomLevelAccelerationStructuresFromInputsListsBatched(
            { blasInputsList }, flags, debugOutput);
}

BottomLevelAccelerationStructurePtr buildBottomLevelAccelerationStructureFromInputsBatched(
        const BottomLevelAccelerationStructureInputList& blasInputList,
        VkBuildAccelerationStructureFlagsKHR flags, bool debugOutput) {
    return buildBottomLevelAccelerationStructuresFromInputsListsBatched(
            { blasInputList }, flags, debugOutput).front();
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
    buildRangeInfo.primitiveCount = uint32_t(numIndices / 3);
    buildRangeInfo.primitiveOffset = 0;
    buildRangeInfo.transformOffset = 0;

    VkAccelerationStructureGeometryTrianglesDataKHR& trianglesData = asGeometry.geometry.triangles;
    trianglesData.indexType = indexType;
    trianglesData.indexData.deviceAddress = indexBuffer->getVkDeviceAddress();
}

void TrianglesAccelerationStructureInput::setIndexBufferOffset(
        BufferPtr& buffer, uint32_t primitiveOffset, uint32_t numIndices, VkIndexType indexType) {
    this->indexBuffer = buffer;
    this->indexType = indexType;
    this->numIndices = numIndices;

    buildRangeInfo.firstVertex = 0;
    buildRangeInfo.primitiveCount = uint32_t(numIndices / 3);
    buildRangeInfo.primitiveOffset = primitiveOffset;
    buildRangeInfo.transformOffset = 0;

    VkAccelerationStructureGeometryTrianglesDataKHR& trianglesData = asGeometry.geometry.triangles;
    trianglesData.indexType = indexType;
    trianglesData.indexData.deviceAddress = indexBuffer->getVkDeviceAddress();
}

void TrianglesAccelerationStructureInput::setVertexBuffer(
        BufferPtr& buffer, VkFormat vertexFormat, VkDeviceSize vertexStride) {
    this->vertexBuffer = buffer;
    this->vertexFormat = vertexFormat;
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
    this->numVertices = buffer->getSizeInBytes() / this->vertexStride;

    VkAccelerationStructureGeometryTrianglesDataKHR& trianglesData = asGeometry.geometry.triangles;
    trianglesData.vertexFormat = this->vertexFormat;
    trianglesData.vertexData.deviceAddress = vertexBuffer->getVkDeviceAddress();
    trianglesData.vertexStride = this->vertexStride;
    trianglesData.maxVertex = uint32_t(this->numVertices);
}

void TrianglesAccelerationStructureInput::setMaxVertex(uint32_t maxVertex) {
    VkAccelerationStructureGeometryTrianglesDataKHR& trianglesData = asGeometry.geometry.triangles;
    trianglesData.maxVertex = maxVertex;
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
    aabbsData.data.deviceAddress = aabbsBuffer->getVkDeviceAddress();
    buildRangeInfo.primitiveCount = uint32_t(numAabbs);
}


BottomLevelAccelerationStructure::~BottomLevelAccelerationStructure() {
    if (accelerationStructure != VK_NULL_HANDLE) {
        vkDestroyAccelerationStructureKHR(device->getVkDevice(), accelerationStructure, nullptr);
    }
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
    bottomLevelAccelerationStructures = blases;

    blasesSizeInBytes = 0;
    tlasSizeInBytes = 0;
    for (auto& blas : blases) {
        blasesSizeInBytes += blas->getAccelerationStructureSizeInBytes();
    }

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

    // Create a buffering that stores the AS instance data. 16 is added for possibly needed alignment padding.
    size_t instancesSizeInBytes = asInstances.size() * sizeof(VkAccelerationStructureInstanceKHR) + 16;
    BufferPtr instancesBuffer(new Buffer(
            device, instancesSizeInBytes,
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
            | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
            VMA_MEMORY_USAGE_GPU_ONLY));
    VkDeviceAddress instancesBufferDeviceAddress = instancesBuffer->getVkDeviceAddress();
    VkDeviceSize instancesAlignmentOffset = instancesBufferDeviceAddress % VkDeviceAddress(16);
    VkDeviceSize instancesWriteOffset = 0;
    if (instancesAlignmentOffset != 0) {
        // This was necessary on AMD hardware.
        instancesWriteOffset = VkDeviceSize(16) - instancesAlignmentOffset;
        instancesBufferDeviceAddress += instancesWriteOffset;
    }
    BufferPtr instancesStagingBuffer;
    instancesBuffer->uploadDataOffset(
            instancesWriteOffset,
            asInstances.size() * sizeof(VkAccelerationStructureInstanceKHR), asInstances.data(),
            commandBuffer, instancesStagingBuffer);

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
    asGeometryInstancesData.data.deviceAddress = instancesBufferDeviceAddress;

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

    auto numInstances = uint32_t(instances.size());
    VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo{};
    buildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(
            device->getVkDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &buildInfo, &numInstances, &buildSizesInfo);

    tlasSizeInBytes = buildSizesInfo.accelerationStructureSize;
    accelerationStructureSizeInBytes = tlasSizeInBytes + blasesSizeInBytes;

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
    VkDeviceSize minAccelerationStructureScratchOffsetAlignment =
            device->getPhysicalDeviceAccelerationStructureProperties().minAccelerationStructureScratchOffsetAlignment;
    BufferPtr scratchBuffer(new Buffer(
            device, buildSizesInfo.buildScratchSize + minAccelerationStructureScratchOffsetAlignment,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
            | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY));
    VkDeviceAddress scratchBufferDeviceAddress = scratchBuffer->getVkDeviceAddress();
    VkDeviceSize alignmentOffset = scratchBufferDeviceAddress % minAccelerationStructureScratchOffsetAlignment;
    if (alignmentOffset != 0) {
        // This was necessary on AMD hardware.
        scratchBufferDeviceAddress += minAccelerationStructureScratchOffsetAlignment - alignmentOffset;
    }

    buildInfo.srcAccelerationStructure = update ? accelerationStructure : VK_NULL_HANDLE;
    buildInfo.dstAccelerationStructure = accelerationStructure;
    buildInfo.scratchData.deviceAddress = scratchBufferDeviceAddress;

    VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
    buildRangeInfo.primitiveCount = static_cast<uint32_t>(instances.size());
    buildRangeInfo.primitiveOffset = 0;
    buildRangeInfo.firstVertex = 0;
    buildRangeInfo.transformOffset = 0;
    const VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfos = &buildRangeInfo;

    vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildInfo, &buildRangeInfos);

    device->endSingleTimeCommands(commandBuffer);
}

TopLevelAccelerationStructure::~TopLevelAccelerationStructure() {
    if (accelerationStructure != VK_NULL_HANDLE) {
        vkDestroyAccelerationStructureKHR(device->getVkDevice(), accelerationStructure, nullptr);
    }
}

}}
