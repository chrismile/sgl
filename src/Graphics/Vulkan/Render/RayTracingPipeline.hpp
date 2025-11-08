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

#ifndef SGL_RAYTRACINGPIPELINE_HPP
#define SGL_RAYTRACINGPIPELINE_HPP

#include <string>
#include <array>
#include <memory>
#include <utility>

#include "ShaderGroupSettings.hpp"
#include "Pipeline.hpp"

namespace sgl { namespace vk {

class Buffer;
typedef std::shared_ptr<Buffer> BufferPtr;

/**
 * A shader group entry in @see ShaderBindingTable.
 */
class DLL_OBJECT RayTracingShaderGroup {
public:
    /**
     * @param shaderStages The global list of all shader stages used in the ray tracing pipeline.
     * HINT: Use "#extension GL_EXT_ray_tracing : require" in the shaders.
     */
    explicit RayTracingShaderGroup(ShaderStagesPtr shaderStages);

    /**
     * Sets shader record data stored in the ray tracing shader group.
     * In GLSL shaders, this data can be accessed via "shaderRecordEXT" values.
     * This is often paired with one of the following extensions:
     * - #extension GL_EXT_buffer_reference2 : require
     * - #extension GL_EXT_scalar_block_layout : enable
     * - #extension GL_EXT_nonuniform_qualifier : enable
     */
    void setShaderRecordData(void* data, uint32_t size);

    uint32_t getSize() const;

    inline bool hasRecordData() const { return !recordData.empty(); }
    inline const uint8_t* getRecordData() const { return recordData.data(); }
    inline uint32_t getRecordDataSize() const { return uint32_t(recordData.size()); }

    inline const VkRayTracingShaderGroupCreateInfoKHR& getShaderGroupCreateInfo() const { return shaderGroupCreateInfo; }

public:
    ShaderStagesPtr shaderStages;
    Device* device = nullptr;
    VkRayTracingShaderGroupCreateInfoKHR shaderGroupCreateInfo{};
    std::vector<uint8_t> recordData;
};

typedef std::shared_ptr<RayTracingShaderGroup> RayTracingShaderGroupPtr;

class DLL_OBJECT RayGenShaderGroup : public RayTracingShaderGroup {
public:
    /**
     * @param shaderStages The global list of all shader stages used in the ray tracing pipeline.
     */
    explicit RayGenShaderGroup(ShaderStagesPtr shaderStages);

    void setRayGenShader(uint32_t shaderModuleIdx);
    void setRayGenShader(const std::string& shaderModuleId);
};

class DLL_OBJECT MissShaderGroup : public RayTracingShaderGroup {
public:
    /**
     * @param shaderStages The global list of all shader stages used in the ray tracing pipeline.
     */
    explicit MissShaderGroup(ShaderStagesPtr shaderStages);

    void setMissShader(uint32_t shaderModuleIdx);
    void setMissShader(const std::string& shaderModuleId);
};

class DLL_OBJECT HitShaderGroup : public RayTracingShaderGroup {
public:
    /**
     * @param shaderStages The global list of all shader stages used in the ray tracing pipeline.
     * @param shaderGroupType VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR or
     * VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR.
     */
    explicit HitShaderGroup(ShaderStagesPtr shaderStages, VkRayTracingShaderGroupTypeKHR shaderGroupType);

    void setClosestHitShader(uint32_t shaderModuleIdx);
    void setClosestHitShader(const std::string& shaderModuleId);

    void setAnyHitShader(uint32_t shaderModuleIdx);
    void setAnyHitShader(const std::string& shaderModuleId);

    void setIntersectionShader(uint32_t shaderModuleIdx);
    void setIntersectionShader(const std::string& shaderModuleId);
};

class DLL_OBJECT CallableShaderGroup : public RayTracingShaderGroup {
public:
    /**
     * @param shaderStages The global list of all shader stages used in the ray tracing pipeline.
     */
    explicit CallableShaderGroup(ShaderStagesPtr shaderStages);

    void setCallable(uint32_t shaderModuleIdx);
    void setCallable(const std::string& shaderModuleId);
};


/**
 * A shader binding table for the ray tracing pipeline.
 * For more information please refer to one of the following resources.
 * - https://vulkan.lunarg.com/doc/view/1.2.135.0/windows/chunked_spec/chap35.html
 * - https://www.willusher.io/graphics/2019/11/20/the-sbt-three-ways
 *
 * (A) Hit shaders
 * pHitShaderBindingTable::offset + pHitShaderBindingTable::stride *
 * (instanceShaderBindingTableRecordOffset + geometryIndex * sbtRecordStride + sbtRecordOffset)
 * 'sbtRecordStride' and 'sbtRecordOffset' are used in traceRayEXT. 'geometryIndex' is the location of the geometry
 * within the instance and is available to shaders as 'RayGeometryIndexKHR'.
 *
 * (B) Miss shaders
 * pMissShaderBindingTable::offset + pMissShaderBindingTable::stride * missIndex
 * 'missIndex' is used in traceRayEXT calls.
 *
 * (C) Callable shaders
 * pCallableShaderBindingTable::offset + pCallableShaderBindingTable::stride * sbtRecordIndex
 *
 */
class DLL_OBJECT ShaderBindingTable {
public:
    explicit ShaderBindingTable(ShaderStagesPtr shaderStages);

    /*
     * Adds a shader group to the binding table. The groups can then be referenced in ShaderGroupSettings in the order
     * in which they were added.
     */
    RayGenShaderGroup* addRayGenShaderGroup();
    MissShaderGroup* addMissShaderGroup();
    HitShaderGroup* addHitShaderGroup(VkRayTracingShaderGroupTypeKHR shaderGroupType);
    CallableShaderGroup* addCallableShaderGroup();

    std::array<VkStridedDeviceAddressRegionKHR, 4> getStridedDeviceAddressRegions(
            const ShaderGroupSettings& shaderGroupSettings) const;
    inline const ShaderStagesPtr& getShaderStages() const { return shaderStages; }

    /// Expects max. one shader of each kind (RayGen, Miss, AnyHit, ClosestHit).
    static ShaderBindingTable generateSimpleShaderBindingTable(
            const ShaderStagesPtr& shaderStages,
            VkRayTracingShaderGroupTypeKHR shaderGroupType = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR);


    // For building the SBT data structure internally (called by RayTracingPipelineInfo, RayTracingPipeline).
    void buildShaderGroups();
    const std::vector<VkRayTracingShaderGroupCreateInfoKHR>& getShaderGroupCreateInfoList() { return shaderGroups; }
    void buildShaderBindingTable(VkPipeline pipeline);

private:
    ShaderStagesPtr shaderStages;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups;

    std::vector<RayTracingShaderGroupPtr> rayGenShaderGroups;
    std::vector<RayTracingShaderGroupPtr> missShaderGroups;
    std::vector<RayTracingShaderGroupPtr> hitShaderGroups;
    std::vector<RayTracingShaderGroupPtr> callableShaderGroups;

    BufferPtr sbtBuffer;
    VkDeviceAddress sbtAddress{};
    uint32_t rayGenGroupStride = 0, missGroupStride = 0, hitGroupStride = 0, callableGroupStride = 0;
    uint32_t missGroupsOffset = 0, hitGroupsOffset = 0, callableGroupsOffset = 0;
};

class DLL_OBJECT RayTracingPipelineInfo {
    friend class RayTracingPipeline;

public:
    explicit RayTracingPipelineInfo(const ShaderBindingTable& table);

    /// Resets to standard settings.
    void reset();

    /// Sets the maximum ray recursion depth. A value of one means no recursion.
    inline void setMaxRayRecursionDepth(uint32_t depth) { maxPipelineRayRecursionDepth = depth; }

    /// Can be used to enable 64-bit indexing if device extension VK_EXT_shader_64bit_indexing is enabled.
    void setUse64BitIndexing(bool _useShader64BitIndexing);

protected:
    ShaderBindingTable sbt;
    ShaderStagesPtr shaderStages;
    uint32_t maxPipelineRayRecursionDepth = 1;

    // Extensions.
    bool useShader64BitIndexing = false;
};

class DLL_OBJECT RayTracingPipeline : public Pipeline {
public:
    RayTracingPipeline(Device* device, const RayTracingPipelineInfo& pipelineInfo);
    ~RayTracingPipeline() override = default;

    /// Called in vk::Renderer.
    inline std::array<VkStridedDeviceAddressRegionKHR, 4> getStridedDeviceAddressRegions(
            const ShaderGroupSettings& shaderGroupSettings) {
        return sbt.getStridedDeviceAddressRegions(shaderGroupSettings);
    }

protected:
    ShaderBindingTable sbt;
};

typedef std::shared_ptr<RayTracingPipeline> RayTracingPipelinePtr;

}}

#endif //SGL_RAYTRACINGPIPELINE_HPP
