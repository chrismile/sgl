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

#ifndef SGL_ACCELERATIONSTRUCTURE_HPP
#define SGL_ACCELERATIONSTRUCTURE_HPP

#include <memory>
#include <utility>

#ifdef USE_GLM
#include <glm/glm.hpp>
#else
#include <Math/Geometry/fallback/mat.hpp>
#endif

#include "../libs/volk/volk.h"

namespace sgl { namespace vk {

class Device;
class Buffer;
typedef std::shared_ptr<Buffer> BufferPtr;

class DLL_OBJECT BottomLevelAccelerationStructureInput {
public:
    /**
     * @param device A Vulkan device.
     * @param geometryFlags The following flags are supported:
     * - VK_GEOMETRY_OPAQUE_BIT_KHR: Call only first-hit.
     * - VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR: Call any-hit once.
     */
    explicit BottomLevelAccelerationStructureInput(
            Device* device, VkGeometryFlagsKHR geometryFlags = VkGeometryFlagsKHR(0));

    [[nodiscard]] inline Device* getDevice() const { return device; }
    [[nodiscard]] inline size_t getNumPrimitives() const { return buildRangeInfo.primitiveCount; }

    inline VkAccelerationStructureGeometryKHR& getAccelerationStructureGeometry() { return asGeometry; }
    [[nodiscard]] inline const VkAccelerationStructureGeometryKHR& getAccelerationStructureGeometry() const { return asGeometry; }
    inline VkAccelerationStructureBuildRangeInfoKHR& getBuildRangeInfo() { return buildRangeInfo; }
    [[nodiscard]] inline const VkAccelerationStructureBuildRangeInfoKHR& getBuildRangeInfo() const { return buildRangeInfo; }

protected:
    Device* device = nullptr;

    VkAccelerationStructureGeometryKHR asGeometry{};
    VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
};

typedef std::shared_ptr<BottomLevelAccelerationStructureInput> BottomLevelAccelerationStructureInputPtr;

class DLL_OBJECT TrianglesAccelerationStructureInput : public BottomLevelAccelerationStructureInput {
public:
    /**
     * @param device A Vulkan device.
     * @param geometryFlags The following flags are supported:
     * - VK_GEOMETRY_OPAQUE_BIT_KHR: Call only first-hit.
     * - VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR: Call any-hit once.
     */
    explicit TrianglesAccelerationStructureInput(
            Device* device, VkGeometryFlagsKHR geometryFlags = VkGeometryFlagsKHR(0));

    void setIndexBuffer(BufferPtr& buffer, VkIndexType indexType = VK_INDEX_TYPE_UINT32);
    void setIndexBufferOffset(
            BufferPtr& buffer, uint32_t primitiveOffset, uint32_t numIndices,
            VkIndexType indexType = VK_INDEX_TYPE_UINT32);
    void setVertexBuffer(BufferPtr& buffer, VkFormat vertexFormat, VkDeviceSize vertexStride = 0);

    /// Optional. Needs to be called after @see setVertexBuffer.
    void setMaxVertex(uint32_t maxVertex);

protected:
    BufferPtr indexBuffer;
    VkIndexType indexType = VK_INDEX_TYPE_UINT32;
    size_t numIndices = 0;

    BufferPtr vertexBuffer;
    VkFormat vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    VkDeviceSize vertexStride = 0;
    size_t numVertices = 0;
};

class DLL_OBJECT AabbsAccelerationStructureInput : public BottomLevelAccelerationStructureInput {
public:
    /**
     * @param device A Vulkan device.
     * @param geometryFlags The following flags are supported:
     * - VK_GEOMETRY_OPAQUE_BIT_KHR: Call only first-hit.
     * - VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR: Call any-hit once.
     */
    explicit AabbsAccelerationStructureInput(
            Device* device, VkGeometryFlagsKHR geometryFlags = VkGeometryFlagsKHR(0));

    /**
     * @param buffer A buffer containing a list of VkAabbPositionsKHR entries.
     * (I.e.: float minX, minY, minZ, maxX, maxY, maxZ.)
     * @param vertexStride The stride between two entries in the buffer.
     */
    void setAabbsBuffer(BufferPtr& buffer, VkDeviceSize stride = 6 * sizeof(float));

protected:
    BufferPtr aabbsBuffer;
    VkDeviceSize aabbsBufferStride = 0;
    size_t numAabbs = 0;
};

class DLL_OBJECT BottomLevelAccelerationStructure {
public:
    explicit BottomLevelAccelerationStructure(
            Device* device, VkAccelerationStructureKHR accelerationStructure, BufferPtr accelerationStructureBuffer,
            size_t blasSizeInBytes)
            : device(device), accelerationStructure(accelerationStructure),
              accelerationStructureBuffer(std::move(accelerationStructureBuffer)),
              accelerationStructureSizeInBytes(blasSizeInBytes) {
    }

    ~BottomLevelAccelerationStructure();

    VkDeviceAddress getAccelerationStructureDeviceAddress();

    inline VkAccelerationStructureKHR getAccelerationStructure() { return accelerationStructure; }
    [[nodiscard]] inline size_t getAccelerationStructureSizeInBytes() const { return accelerationStructureSizeInBytes; }

protected:
    Device* device = nullptr;
    VkAccelerationStructureKHR accelerationStructure = VK_NULL_HANDLE;
    BufferPtr accelerationStructureBuffer;
    size_t accelerationStructureSizeInBytes = 0;
};

typedef std::shared_ptr<BottomLevelAccelerationStructure> BottomLevelAccelerationStructurePtr;
typedef std::vector<BottomLevelAccelerationStructureInputPtr> BottomLevelAccelerationStructureInputList;

DLL_OBJECT std::vector<BottomLevelAccelerationStructurePtr> buildBottomLevelAccelerationStructuresFromInputsLists(
        const std::vector<BottomLevelAccelerationStructureInputList>& blasInputsList,
        VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        bool debugOutput = false);

DLL_OBJECT std::vector<BottomLevelAccelerationStructurePtr> buildBottomLevelAccelerationStructuresFromInputList(
        const std::vector<BottomLevelAccelerationStructureInputPtr>& blasInputList,
        VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        bool debugOutput = false);

DLL_OBJECT BottomLevelAccelerationStructurePtr buildBottomLevelAccelerationStructureFromInputs(
        const BottomLevelAccelerationStructureInputList& blasInputs,
        VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        bool debugOutput = false);

DLL_OBJECT BottomLevelAccelerationStructurePtr buildBottomLevelAccelerationStructureFromInput(
        const BottomLevelAccelerationStructureInputPtr& blasInput,
        VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        bool debugOutput = false);

/**
 * Same as @see buildBottomLevelAccelerationStructuresFromInputsLists, but supports batched BLAS reduction.
 */
DLL_OBJECT std::vector<BottomLevelAccelerationStructurePtr> buildBottomLevelAccelerationStructuresFromInputsListsBatched(
        const std::vector<BottomLevelAccelerationStructureInputList>& blasInputsList,
        VkBuildAccelerationStructureFlagsKHR flags, bool debugOutput);
DLL_OBJECT std::vector<BottomLevelAccelerationStructurePtr> buildBottomLevelAccelerationStructuresFromInputListBatched(
        const std::vector<BottomLevelAccelerationStructureInputPtr>& blasInputList,
        VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        bool debugOutput = false);
DLL_OBJECT BottomLevelAccelerationStructurePtr buildBottomLevelAccelerationStructureFromInputsBatched(
        const BottomLevelAccelerationStructureInputList& blasInputs,
        VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        bool debugOutput = false);

/**
 * Bottom-level acceleration structure instance.
 */
class DLL_OBJECT BlasInstance {
public:
    BlasInstance()
            : transform(1.0f), blasIdx(0), instanceCustomIndex(0), mask(0xFF), shaderBindingTableRecordOffset(0),
            flags(VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR) {}

    glm::mat4 transform = glm::mat4();
    uint32_t blasIdx;
    uint32_t instanceCustomIndex:24;
    uint32_t mask:8;
    uint32_t shaderBindingTableRecordOffset:24;
    VkGeometryInstanceFlagsKHR flags:8;
};

class DLL_OBJECT TopLevelAccelerationStructure {
public:
    explicit TopLevelAccelerationStructure(Device* device) : device(device) {}

    ~TopLevelAccelerationStructure();

    /**
     * Builds the top-level acceleration structure from a set of bottom-level acceleration structure instances.
     * NOTE: This function can be called multiple times, e.g., when wanting to update the transforms of the bottom-level
     * acceleration structure instances.
     * @param blases The bottom-level acceleration structures.
     * @param instances The bottom-level acceleration structure instances referencing the entries in 'blases'.
     * @param flags
     */
    void build(
            const std::vector<BottomLevelAccelerationStructurePtr>& blases,
            const std::vector<BlasInstance>& instances,
            VkBuildAccelerationStructureFlagsKHR flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);

    inline VkAccelerationStructureKHR getAccelerationStructure() { return accelerationStructure; }
    [[nodiscard]] inline size_t getBlasesSizeInBytes() const { return blasesSizeInBytes; }
    [[nodiscard]] inline size_t getTlasSizeInBytes() const { return tlasSizeInBytes; }
    [[nodiscard]] inline size_t getAccelerationStructureSizeInBytes() const { return accelerationStructureSizeInBytes; }

protected:
    Device* device = nullptr;
    VkAccelerationStructureKHR accelerationStructure = VK_NULL_HANDLE;
    BufferPtr accelerationStructureBuffer;
    std::vector<BottomLevelAccelerationStructurePtr> bottomLevelAccelerationStructures;
    size_t blasesSizeInBytes = 0;
    size_t tlasSizeInBytes = 0;
    size_t accelerationStructureSizeInBytes = 0;
};

typedef std::shared_ptr<TopLevelAccelerationStructure> TopLevelAccelerationStructurePtr;

}}

#endif //SGL_ACCELERATIONSTRUCTURE_HPP
