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
#include "GraphicsPipeline.hpp"
#include "Data.hpp"

namespace sgl { namespace vk {

inline size_t getIndexTypeByteSize(VkIndexType indexType) {
    if (indexType == VK_INDEX_TYPE_UINT32) {
        return 4;
    } else if (indexType == VK_INDEX_TYPE_UINT16) {
        return 2;
    } else if (indexType == VK_INDEX_TYPE_UINT8_EXT) {
        return 1;
    } else {
        Logfile::get()->throwError("Error in getIndexTypeByteSize: Invalid index type.");
        return 1;
    }
}

RasterData::RasterData(GraphicsPipelinePtr& graphicsPipeline) : graphicsPipeline(graphicsPipeline) {
    ;
}

void RasterData::setIndexBuffer(BufferPtr& buffer, VkIndexType indexType) {
    indexBuffer = buffer;
    this->indexType = indexType;
    numIndices = buffer->getSizeInBytes() / getIndexTypeByteSize(indexType);
}

void RasterData::setVertexBuffer(BufferPtr& buffer, uint32_t binding) {
    bool isFirstVertexBuffer = vertexBuffers.empty();

    const std::vector<VkVertexInputBindingDescription>& vertexInputBindingDescriptions =
            graphicsPipeline->getVertexInputBindingDescriptions();
    if (uint32_t(vertexInputBindingDescriptions.size()) <= binding) {
        Logfile::get()->throwError(
                "Error in RasterData::setVertexBuffer: Binding point missing in vertex input binding "
                "description list.");
    }
    const VkVertexInputBindingDescription& vertexInputBindingDescription = vertexInputBindingDescriptions.at(binding);
    size_t numVerticesNew = buffer->getSizeInBytes() / vertexInputBindingDescription.stride;

    if (!isFirstVertexBuffer && numVertices != numVerticesNew) {
        Logfile::get()->throwError("Error in RasterData::setVertexBuffer: Inconsistent number of vertices.");
    }

    if (vertexBuffers.size() <= binding) {
        vertexBuffers.resize(binding + 1);
        vulkanVertexBuffers.resize(binding + 1);
    }

    vertexBuffers.at(binding) = buffer;
    vulkanVertexBuffers.at(binding) = buffer->getVkBuffer();
    numVertices = numVerticesNew;
}

void RasterData::setVertexBuffer(BufferPtr& buffer, const std::string& name) {
    uint32_t location = graphicsPipeline->getShaderStages()->getInputVariableLocation(name);
    setVertexBuffer(buffer, location);
}

}}
