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

#include <Math/Math.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/AppSettings.hpp>
#include <Graphics/Vulkan/Image/Image.hpp>
#include <Graphics/Vulkan/Shader/ShaderManager.hpp>
#include <Graphics/Vulkan/Render/Renderer.hpp>
#include <memory>
#include "BlitComputePass.hpp"

namespace sgl { namespace vk {

BlitComputePass::BlitComputePass(sgl::vk::Renderer* renderer) : BlitComputePass(renderer, {"Blit.Compute"}) {
}

BlitComputePass::BlitComputePass(sgl::vk::Renderer* renderer, std::vector<std::string> customShaderIds)
        : ComputePass(renderer), shaderIds(std::move(customShaderIds)) {
}

void BlitComputePass::setInputTexture(sgl::vk::TexturePtr& texture) {
    if (inputTexture != texture) {
        inputTexture = texture;
        if (computeData) {
            computeData->setStaticTexture(inputTexture, "inputTexture");
        }
    }
}

void BlitComputePass::setOutputImage(sgl::vk::ImageViewPtr& imageView) {
    if (outputImageView != imageView) {
        bool formatMatches = true;
        if (outputImageView) {
            formatMatches =
                    getImageFormatGlslString(outputImageView->getImage()->getImageSettings().format)
                    == getImageFormatGlslString(imageView->getImage()->getImageSettings().format);
        }
        outputImageView = imageView;
        if (formatMatches && computeData) {
            computeData->setStaticImageView(outputImageView, "outputImage");
        }
        if (!formatMatches) {
            setShaderDirty();
        }
    }
}

void BlitComputePass::_render() {
    const auto& imageSettings = outputImageView->getImage()->getImageSettings();
    renderer->dispatch(
            computeData,
            sgl::uiceil(imageSettings.width, LOCAL_SIZE_X), sgl::uiceil(imageSettings.height, LOCAL_SIZE_Y), 1);
}

void BlitComputePass::loadShader() {
    sgl::vk::ShaderManager->invalidateShaderCache();
    std::map<std::string, std::string> preprocessorDefines;
    preprocessorDefines.insert(std::make_pair("LOCAL_SIZE_X", std::to_string(LOCAL_SIZE_X)));
    preprocessorDefines.insert(std::make_pair("LOCAL_SIZE_Y", std::to_string(LOCAL_SIZE_Y)));
    preprocessorDefines.insert(std::make_pair(
            "OUTPUT_IMAGE_FORMAT", getImageFormatGlslString(outputImageView->getImage()->getImageSettings().format)));
    shaderStages = sgl::vk::ShaderManager->getShaderStages(shaderIds);
}

void BlitComputePass::createComputeData(sgl::vk::Renderer* renderer, sgl::vk::ComputePipelinePtr& computePipeline) {
    computeData = std::make_shared<sgl::vk::ComputeData>(renderer, computePipeline);
    computeData->setStaticTexture(inputTexture, "inputTexture");
    computeData->setStaticImageView(outputImageView, "outputImage");
}

}}
