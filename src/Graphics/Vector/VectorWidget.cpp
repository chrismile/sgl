/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2023, Christoph Neuhauser
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

#include <Utils/AppSettings.hpp>
#include <Utils/File/Logfile.hpp>
#include <ImGui/ImGuiWrapper.hpp>
#include <Graphics/Window.hpp>
#include <Input/Mouse.hpp>

#ifdef SUPPORT_OPENGL
#include <GL/glew.h>
#include <Graphics/Renderer.hpp>
#include <Graphics/Texture/Texture.hpp>
#include <Graphics/Texture/TextureManager.hpp>
#include <Graphics/OpenGL/RendererGL.hpp>
#include <Graphics/OpenGL/ShaderManager.hpp>
#include <Graphics/OpenGL/Texture.hpp>
#endif

#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/Render/Renderer.hpp>
#include <Graphics/Vulkan/Render/Passes/BlitRenderPass.hpp>
#endif

#include "VectorWidget.hpp"

namespace sgl {

#ifdef SUPPORT_VULKAN
class BlindRenderPassAffine : public sgl::vk::BlitRenderPass {
public:
    BlindRenderPassAffine(
            sgl::vk::Renderer* renderer, std::vector<std::string> customShaderIds,
            sgl::vk::BufferPtr blitMatrixBuffer)
            : BlitRenderPass(renderer, std::move(customShaderIds)), blitMatrixBuffer(std::move(blitMatrixBuffer)) {}

protected:
    void createRasterData(sgl::vk::Renderer* renderer, sgl::vk::GraphicsPipelinePtr& graphicsPipeline) override {
        BlitRenderPass::createRasterData(renderer, graphicsPipeline);
        rasterData->setStaticBuffer(blitMatrixBuffer, "BlitMatrixBuffer");
    }

private:
    sgl::vk::BufferPtr blitMatrixBuffer;
};
#endif

VectorWidget::VectorWidget() {
}

VectorWidget::~VectorWidget() {
    if (vectorBackend) {
        vectorBackend->destroy();
        delete vectorBackend;
        vectorBackend = nullptr;
    }
}

void VectorWidget::setDefaultBackendId(const std::string& defaultId) {
    defaultBackendId = defaultId;
}

void VectorWidget::_initialize() {
    if (initialized) {
        return;
    }
    initialized = true;

    if (customScaleFactor <= 0.0f) {
        scaleFactor = sgl::ImGuiWrapper::get()->getScaleFactor();
    } else {
        scaleFactor = customScaleFactor;
    }

    if (!vectorBackend) {
        createDefaultBackend();
    }

#if defined(SUPPORT_OPENGL) || defined(SUPPORT_VULKAN)
    RenderSystem renderSystem = sgl::AppSettings::get()->getRenderSystem();
#endif

#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL) {
        blitShader = sgl::ShaderManager->getShaderProgram(
                { "BlitPremulAlpha.Vertex", "BlitPremulAlpha.FragmentBlit" });
        blitMsaaShader = sgl::ShaderManager->getShaderProgram(
                { "BlitPremulAlpha.Vertex", "BlitPremulAlpha.FragmentBlitMS" });
        blitDownscaleShader = sgl::ShaderManager->getShaderProgram(
                { "BlitPremulAlpha.Vertex", "BlitPremulAlpha.FragmentBlitDownscale" });
        blitDownscaleMsaaShader = sgl::ShaderManager->getShaderProgram(
                { "BlitPremulAlpha.Vertex", "BlitPremulAlpha.FragmentBlitDownscaleMS" });
    }
#endif

#if defined(SUPPORT_OPENGL) && defined(SUPPORT_VULKAN)
    if (renderSystem == RenderSystem::VULKAN) {
        vk::Device* device = AppSettings::get()->getPrimaryDevice();
        blitMatrixBuffer = std::make_shared<sgl::vk::Buffer>(
                device, sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VMA_MEMORY_USAGE_GPU_ONLY);
    }
#endif
}

void VectorWidget::onWindowSizeChanged() {
    fboWidthDisplay = int(std::ceil(float(windowWidth) * scaleFactor));
    fboHeightDisplay = int(std::ceil(float(windowHeight) * scaleFactor));
    fboWidthInternal = fboWidthDisplay * supersamplingFactor;
    fboHeightInternal = fboHeightDisplay * supersamplingFactor;

    if (!initialized) {
        _initialize();
    }

#if defined(SUPPORT_OPENGL) || defined(SUPPORT_VULKAN)
    RenderSystem renderSystem = sgl::AppSettings::get()->getRenderSystem();
    RenderSystem renderBackend = vectorBackend->getRenderBackend();
#endif

    vectorBackend->setWidgetSize(
            scaleFactor, supersamplingFactor, windowWidth, windowHeight,
            fboWidthInternal, fboHeightInternal, fboWidthDisplay, fboHeightDisplay);
    vectorBackend->onResize();

#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL || renderBackend == RenderSystem::OPENGL) {
        renderTargetGl = vectorBackend->getRenderTargetTextureGl();
    }
#endif

#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN || renderBackend == RenderSystem::VULKAN) {
        renderTargetTextureVk = vectorBackend->getRenderTargetTextureVk();
        renderTargetImageViewVk = renderTargetTextureVk->getImageView();
    }
#endif

#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN || renderBackend == RenderSystem::VULKAN) {
        if (blitTargetVk) {
            _createBlitRenderPass();
        }
    }
#endif
}

bool VectorWidget::isMouseOverDiagram() const {
    glm::vec2 mousePosition(sgl::Mouse->getX(), sgl::Mouse->getY());
    mousePosition.y = float(sgl::AppSettings::get()->getMainWindow()->getHeight()) - mousePosition.y - 1.0f;

    sgl::AABB2 aabb;
    aabb.min = glm::vec2(windowOffsetX, windowOffsetY);
    aabb.max = glm::vec2(windowOffsetX + float(fboWidthDisplay), windowOffsetY + float(fboHeightDisplay));

    return aabb.contains(mousePosition);
}

bool VectorWidget::isMouseOverDiagram(int parentX, int parentY, int parentWidth, int parentHeight) const {
    glm::vec2 mousePosition(sgl::Mouse->getX(), sgl::Mouse->getY());
    mousePosition.y = float(sgl::AppSettings::get()->getMainWindow()->getHeight()) - mousePosition.y - 1.0f;
    mousePosition.x -= float(parentX);
    mousePosition.y -= float(sgl::AppSettings::get()->getMainWindow()->getHeight() - parentY - 1 + parentHeight);

    sgl::AABB2 aabb;
    aabb.min = glm::vec2(windowOffsetX, windowOffsetY);
    aabb.max = glm::vec2(windowOffsetX + float(fboWidthDisplay), windowOffsetY + float(fboHeightDisplay));

    return aabb.contains(mousePosition);
}

void VectorWidget::createDefaultBackend() {
    if (!vectorBackend) {
        if (factories.empty()) {
            sgl::Logfile::get()->throwError("Error in VectorWidget::render: No backend available to create!");
        }
        if (defaultBackendId.empty()) {
            defaultBackendId = factories.begin()->first;
        }
        auto it = factories.find(defaultBackendId);
        if (it == factories.end()) {
            sgl::Logfile::get()->throwError("Error in VectorWidget::render: Could not create default backend!");
        }
        vectorBackend = it->second.createBackendFunctor();
#ifdef SUPPORT_VULKAN
        vectorBackend->setRendererVk(rendererVk);
#endif
        vectorBackend->initialize();
    }
}

void VectorWidget::render() {
    createDefaultBackend();
    vectorBackend->renderStart();
    auto it = factories.find(vectorBackend->getID());
    if (it != factories.end()) {
        it->second.renderFunctor();
    } else {
        sgl::Logfile::get()->throwError("");
    }
    vectorBackend->renderEnd();
}

#ifdef SUPPORT_OPENGL
void VectorWidget::blitToTargetGl(sgl::FramebufferObjectPtr& sceneFramebuffer) {
    RenderSystem renderSystem = sgl::AppSettings::get()->getRenderSystem();
    if (renderSystem == RenderSystem::OPENGL) {
        glDisable(GL_CULL_FACE);
        sgl::ShaderManager->invalidateBindings();
        static_cast<sgl::RendererGL*>(sgl::Renderer)->resetShaderProgram();
        sgl::Renderer->bindFBO(sceneFramebuffer);
        glViewport(0, 0, sceneFramebuffer->getWidth(), sceneFramebuffer->getHeight());
        sgl::Renderer->setProjectionMatrix(sgl::matrixOrthogonalProjection(
                0.0f, float(sceneFramebuffer->getWidth()),
                0.0f, float(sceneFramebuffer->getHeight()),
                -1.0f, 1.0f));
        sgl::Renderer->setViewMatrix(sgl::matrixIdentity());
        sgl::Renderer->setModelMatrix(sgl::matrixIdentity());
        sgl::AABB2 aabb;
        aabb.min = glm::vec2(windowOffsetX, windowOffsetY);
        aabb.max = glm::vec2(windowOffsetX + float(fboWidthDisplay), windowOffsetY + float(fboHeightDisplay));
        // Normal alpha blending
        //glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
        // Premultiplied alpha.
        glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        if (supersamplingFactor <= 1) {
            sgl::Renderer->blitTexture(renderTargetGl, aabb);
        } else {
            sgl::ShaderProgramPtr blitShader;
            if (useMsaa) {
                blitShader = blitDownscaleMsaaShader;
            } else {
                blitShader = blitDownscaleShader;
            }
            blitShader->setUniform("supersamplingFactor", supersamplingFactor);
            sgl::Renderer->blitTexture(renderTargetGl, aabb, blitShader);
        }
    }
}
#endif

#ifdef SUPPORT_VULKAN
void VectorWidget::_createBlitRenderPass() {
    std::vector<std::string> shaderIds;
    shaderIds.reserve(2);
    shaderIds.emplace_back("BlitPremulAlpha.Vertex");
    if (supersamplingFactor <= 1) {
        if (!useMsaa) {
            shaderIds.emplace_back("BlitPremulAlpha.FragmentBlit");
        } else {
            shaderIds.emplace_back("BlitPremulAlpha.FragmentBlitMS");
        }
    } else {
        if (!useMsaa) {
            shaderIds.emplace_back("BlitPremulAlpha.FragmentBlitDownscale");
        } else {
            shaderIds.emplace_back("BlitPremulAlpha.FragmentBlitDownscaleMS");
        }
    }
    blitPassVk = std::make_shared<BlindRenderPassAffine>(rendererVk, shaderIds, blitMatrixBuffer);

    const auto& blitImageSettings = blitTargetVk->getImage()->getImageSettings();
    blitPassVk->setBlendMode(vk::BlendMode::BACK_TO_FRONT_PREMUL_ALPHA);
    blitPassVk->setOutputImageInitialLayout(blitInitialLayoutVk);
    blitPassVk->setOutputImageFinalLayout(blitFinalLayoutVk);
    blitPassVk->setAttachmentLoadOp(
            blitInitialLayoutVk == VK_IMAGE_LAYOUT_UNDEFINED
            ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD);
    //blitPassVk->setDepthWriteEnabled(false);
    //blitPassVk->setDepthTestEnabled(false);
    //blitPassVk->setDepthCompareOp(VK_COMPARE_OP_ALWAYS);
    blitPassVk->setCullMode(sgl::vk::CullMode::CULL_NONE);
    blitPassVk->setInputTexture(renderTargetTextureVk);
    blitPassVk->setOutputImage(blitTargetVk);
    blitPassVk->recreateSwapchain(blitImageSettings.width, blitImageSettings.height);
}

void VectorWidget::setBlitTargetVk(
        vk::ImageViewPtr& _blitTargetVk, VkImageLayout initialLayout, VkImageLayout finalLayout) {
    blitTargetVk = _blitTargetVk;
    blitInitialLayoutVk = initialLayout;
    blitFinalLayoutVk = finalLayout;
    _createBlitRenderPass();
}

void VectorWidget::blitToTargetVk() {
    RenderSystem renderBackend = vectorBackend->getRenderBackend();
    glm::mat4 blitMatrix = sgl::matrixOrthogonalProjection(
            0.0f, float(blitTargetVk->getImage()->getImageSettings().width),
            0.0f, float(blitTargetVk->getImage()->getImageSettings().height),
            0.0f, 1.0f);
    sgl::AABB2 aabb;
    aabb.min = glm::vec2(windowOffsetX, windowOffsetY);
    aabb.max = glm::vec2(windowOffsetX + float(fboWidthDisplay), windowOffsetY + float(fboHeightDisplay));
    blitPassVk->setNormalizedCoordinatesAabb(aabb, renderBackend == RenderSystem::OPENGL);
    blitMatrixBuffer->updateData(sizeof(glm::mat4), &blitMatrix, rendererVk->getVkCommandBuffer());
    rendererVk->insertBufferMemoryBarrier(
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            blitMatrixBuffer);
    renderTargetImageViewVk->transitionImageLayout(
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, rendererVk->getVkCommandBuffer());

    blitPassVk->buildIfNecessary();
    if (supersamplingFactor <= 1 && useMsaa) {
        // FragmentBlitMS
        rendererVk->pushConstants(
                blitPassVk->getGraphicsPipeline(), VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                int32_t(renderTargetImageViewVk->getImage()->getImageSettings().numSamples));
    }
    if (supersamplingFactor > 1 && !useMsaa) {
        // FragmentBlitDownscale
        rendererVk->pushConstants(
                blitPassVk->getGraphicsPipeline(), VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                int32_t(supersamplingFactor));
    }
    if (supersamplingFactor > 1 && useMsaa) {
        // FragmentBlitDownscaleMS
        rendererVk->pushConstants(
                blitPassVk->getGraphicsPipeline(), VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                int32_t(renderTargetImageViewVk->getImage()->getImageSettings().numSamples));
        rendererVk->pushConstants(
                blitPassVk->getGraphicsPipeline(), VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(int32_t),
                int32_t(supersamplingFactor));
    }
    blitPassVk->render();
}
#endif

}
