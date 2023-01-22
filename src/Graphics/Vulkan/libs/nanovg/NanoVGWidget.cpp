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

#include "nanovg.h"

#ifdef SUPPORT_OPENGL
#include <GL/glew.h>
#include <Graphics/Renderer.hpp>
#include <Graphics/Buffers/FBO.hpp>
#include <Graphics/Texture/Texture.hpp>
#include <Graphics/Texture/TextureManager.hpp>
#include <Graphics/OpenGL/RendererGL.hpp>
#include <Graphics/OpenGL/ShaderManager.hpp>
#include <Graphics/OpenGL/Texture.hpp>
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"
#endif

#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/Utils/Swapchain.hpp>
#include <Graphics/Vulkan/Buffers/Framebuffer.hpp>
#include <Graphics/Vulkan/Render/Renderer.hpp>
#include <Graphics/Vulkan/Render/CommandBuffer.hpp>
#include <Graphics/Vulkan/Render/Passes/BlitRenderPass.hpp>
#include <memory>
#define NANOVG_VULKAN_IMPLEMENTATION
#include "nanovg_vk.h"
#endif

#if defined(SUPPORT_OPENGL) && defined(SUPPORT_VULKAN)
#include <Graphics/Vulkan/Utils/Interop.hpp>
#include <utility>
#endif

#include "NanoVGWidget.hpp"

namespace sgl {

#ifdef SUPPORT_VULKAN
class BlitRenderPassNanoVG : public sgl::vk::BlitRenderPass {
public:
    BlitRenderPassNanoVG(
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

NanoVGSettings::NanoVGSettings() {
    RenderSystem renderSystem = sgl::AppSettings::get()->getRenderSystem();
    if (renderSystem == RenderSystem::OPENGL) {
        nanoVgBackend = NanoVGBackend::OPENGL;
    } else if (renderSystem == RenderSystem::VULKAN) {
        nanoVgBackend = NanoVGBackend::VULKAN;
    } else {
        sgl::Logfile::get()->throwError(
                "Error in NanoVGSettings::NanoVGSettings: Encountered unsupported render system.");
        nanoVgBackend = NanoVGBackend::VULKAN;
    }

#ifndef NDEBUG
    useDebugging = true;
#else
    useDebugging = false;
#endif
}

NanoVGWidget::NanoVGWidget(NanoVGSettings nanoVgSettings) {
    setSettings(nanoVgSettings);
}

void NanoVGWidget::setSettings(NanoVGSettings nanoVgSettings) {
    useMsaa = nanoVgSettings.useMsaa;
    numMsaaSamples = nanoVgSettings.numMsaaSamples;
    supersamplingFactor = nanoVgSettings.supersamplingFactor;
    shallClearBeforeRender = nanoVgSettings.shallClearBeforeRender;

    nanoVgBackend = nanoVgSettings.nanoVgBackend;

#ifndef SUPPORT_OPENGL
    if (nanoVgBackend == NanoVGBackend::OPENGL) {
        sgl::Logfile::get()->throwError(
                "Error in NanoVGWrapper::NanoVGWrapper: OpenGL backend selected, but OpenGL is not supported.");
    }
#endif

#ifndef SUPPORT_VULKAN
    if (nanoVgBackend == NanoVGBackend::VULKAN) {
        sgl::Logfile::get()->throwError(
                "Error in NanoVGWrapper::NanoVGWrapper: Vulkan backend selected, but Vulkan is not supported.");
    }
#endif

    flags = {};
    if (nanoVgSettings.useStencilStrokes) {
        flags |= NVG_STENCIL_STROKES;
    }
    if (!useMsaa) {
        flags |= NVG_ANTIALIAS;
    }
    if (nanoVgSettings.useDebugging) {
        flags |= NVG_DEBUG;
    }
}

NanoVGWidget::~NanoVGWidget() {
#ifdef SUPPORT_OPENGL
    if (nanoVgBackend == NanoVGBackend::OPENGL) {
        if (vg) {
            nvgDeleteGL3(vg);
            vg = nullptr;
        }
    }
#endif

#ifdef SUPPORT_VULKAN
    if (nanoVgBackend == NanoVGBackend::VULKAN) {
        vk::Device* device = AppSettings::get()->getPrimaryDevice();
        if (!nanovgCommandBuffers.empty()) {
            vkFreeCommandBuffers(
                    device->getVkDevice(), commandPool,
                    uint32_t(nanovgCommandBuffers.size()), nanovgCommandBuffers.data());
            nanovgCommandBuffers.clear();
        }
        for (auto* vgEntry : vgArray) {
            nvgDeleteVk(vgEntry);
        }
        vgArray.clear();
    }
#endif
}

void NanoVGWidget::_initializeFont(NVGcontext* vgCurrent) {
    std::string fontFilename = sgl::AppSettings::get()->getDataDirectory() + "Fonts/DroidSans.ttf";
    int font = nvgCreateFont(vgCurrent, "sans", fontFilename.c_str());
    if (font == -1) {
        sgl::Logfile::get()->throwError("Error in NanoVGWrapper::_initializeFont: Couldn't find the font file.");
    }
}

void NanoVGWidget::_initialize() {
    if (initialized) {
        return;
    }
    initialized = true;

    scaleFactor = sgl::ImGuiWrapper::get()->getScaleFactor();

#if defined(SUPPORT_OPENGL) || defined(SUPPORT_VULKAN)
    RenderSystem renderSystem = sgl::AppSettings::get()->getRenderSystem();
#endif

#ifdef SUPPORT_OPENGL
    if (nanoVgBackend == NanoVGBackend::OPENGL) {
        vg = nvgCreateGL3(flags);
        _initializeFont(vg);
    }
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

#ifdef SUPPORT_VULKAN
    if (nanoVgBackend == NanoVGBackend::VULKAN) {
        vk::Device* device = AppSettings::get()->getPrimaryDevice();
        vk::Swapchain* swapchain = AppSettings::get()->getSwapchain();

        vk::CommandPoolType commandPoolType;
        commandPoolType.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        nanovgCommandBuffers = device->allocateCommandBuffers(
                commandPoolType, &commandPool, swapchain ? uint32_t(swapchain->getMaxNumFramesInFlight()) : 1);

        /*vk::CommandPoolType commandPoolType;
        commandPoolType.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        int maxNumFramesInFlight = swapchain ? swapchain->getMaxNumFramesInFlight() : 1;
        for (int i = 0; i < maxNumFramesInFlight; i++) {
            nanovgCommandBuffers.push_back(std::make_shared<sgl::vk::CommandBuffer>(device, commandPoolType));
        }*/

        if (!framebufferVk) {
            onWindowSizeChanged();
        }

        VkQueue graphicsQueue = device->getGraphicsQueue();
        VKNVGCreateInfo vknvgCreateInfo{};
        vknvgCreateInfo.gpu = device->getVkPhysicalDevice();
        vknvgCreateInfo.device = device->getVkDevice();
        vknvgCreateInfo.renderpass = framebufferVk->getVkRenderPass();

        int maxNumFramesInFlight = swapchain ? swapchain->getMaxNumFramesInFlight() : 1;
        vgArray.resize(maxNumFramesInFlight);
        for (int i = 0; i < maxNumFramesInFlight; i++) {
            vknvgCreateInfo.cmdBuffer = nanovgCommandBuffers.at(i);
            vknvgCreateInfo.cmdBufferSingleTime = nanovgCommandBuffers.at(i);
            vgArray[i] = nvgCreateVk(vknvgCreateInfo, flags, graphicsQueue);
            _initializeFont(vgArray[i]);
        }

    }
#endif

#if defined(SUPPORT_OPENGL) && defined(SUPPORT_VULKAN)
    if (nanoVgBackend == NanoVGBackend::OPENGL && renderSystem == RenderSystem::VULKAN) {
        vk::Device* device = AppSettings::get()->getPrimaryDevice();
        vk::Swapchain* swapchain = AppSettings::get()->getSwapchain();

        vk::CommandPoolType commandPoolType;
        commandPoolType.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        int maxNumFramesInFlight = swapchain ? swapchain->getMaxNumFramesInFlight() : 1;
        for (int i = 0; i < maxNumFramesInFlight; i++) {
            commandBuffersPost.push_back(std::make_shared<sgl::vk::CommandBuffer>(device, commandPoolType));
        }
    }
#endif

#if defined(SUPPORT_OPENGL) && defined(SUPPORT_VULKAN)
    if ((nanoVgBackend == NanoVGBackend::OPENGL) != (renderSystem == RenderSystem::OPENGL)) {
        vk::Device* device = AppSettings::get()->getPrimaryDevice();
        vk::Swapchain* swapchain = AppSettings::get()->getSwapchain();
        int maxNumFramesInFlight = swapchain ? swapchain->getMaxNumFramesInFlight() : 1;
        interopSyncVkGl = std::make_shared<sgl::InteropSyncVkGl>(device, maxNumFramesInFlight);
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

void NanoVGWidget::onWindowSizeChanged() {
    fboWidthDisplay = int(std::ceil(float(windowWidth) * scaleFactor));
    fboHeightDisplay = int(std::ceil(float(windowHeight) * scaleFactor));
    fboWidthInternal = fboWidthDisplay * supersamplingFactor;
    fboHeightInternal = fboHeightDisplay * supersamplingFactor;

    if (!initialized) {
        _initialize();
    }

#if defined(SUPPORT_OPENGL) || defined(SUPPORT_VULKAN)
    RenderSystem renderSystem = sgl::AppSettings::get()->getRenderSystem();
#endif

#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL && nanoVgBackend != NanoVGBackend::VULKAN) {
        sgl::TextureSettings textureSettingsColor;
        textureSettingsColor.internalFormat = GL_RGBA8;
        if (useMsaa) {
            renderTargetGl = sgl::TextureManager->createMultisampledTexture(
                    fboWidthInternal, fboHeightInternal, numMsaaSamples, textureSettingsColor.internalFormat);
        } else {
            renderTargetGl = sgl::TextureManager->createEmptyTexture(
                    fboWidthInternal, fboHeightInternal, textureSettingsColor);
        }
    }
#endif

#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN || nanoVgBackend == NanoVGBackend::VULKAN) {
        sgl::vk::Device* device = sgl::AppSettings::get()->getPrimaryDevice();

        sgl::vk::ImageSettings imageSettings;
        imageSettings.width = uint32_t(fboWidthInternal);
        imageSettings.height = uint32_t(fboHeightInternal);
        imageSettings.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageSettings.usage =
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        if (useMsaa) {
            imageSettings.numSamples = VkSampleCountFlagBits(numMsaaSamples);
        }
#ifdef SUPPORT_OPENGL
        if (nanoVgBackend == NanoVGBackend::OPENGL) {
            imageSettings.exportMemory = true;
        }
#endif
        sgl::vk::ImageSamplerSettings samplerSettings;
        renderTargetTextureVk = std::make_shared<sgl::vk::Texture>(device, imageSettings, samplerSettings);
#ifdef SUPPORT_OPENGL
        if (nanoVgBackend == NanoVGBackend::OPENGL) {
            renderTargetGl = sgl::TexturePtr(new sgl::TextureGLExternalMemoryVk(renderTargetTextureVk));
        }
#endif
        renderTargetImageViewVk = renderTargetTextureVk->getImageView();

        vk::AttachmentState attachmentState;
        attachmentState.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachmentState.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachmentState.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        framebufferVk = std::make_shared<vk::Framebuffer>(
                device, uint32_t(fboWidthInternal), uint32_t(fboHeightInternal));
        framebufferVk->setColorAttachment(renderTargetImageViewVk, 0, attachmentState);
    }
#endif

#ifdef SUPPORT_OPENGL
    if (nanoVgBackend == NanoVGBackend::OPENGL) {
        depthStencilRbo = sgl::Renderer->createRBO(
                fboWidthInternal, fboHeightInternal, sgl::RBO_DEPTH24_STENCIL8,
                useMsaa ? numMsaaSamples : 0);

        framebufferGl = sgl::Renderer->createFBO();
        framebufferGl->bindTexture(renderTargetGl, sgl::COLOR_ATTACHMENT);
        framebufferGl->bindRenderbuffer(depthStencilRbo, sgl::DEPTH_STENCIL_ATTACHMENT);
    }
#endif

#ifdef SUPPORT_VULKAN
    if (renderSystem == RenderSystem::VULKAN || nanoVgBackend == NanoVGBackend::VULKAN) {
        if (blitTargetVk) {
            _createBlitRenderPass();
        }
    }
#endif
}

bool NanoVGWidget::isMouseOverDiagram() const {
    glm::vec2 mousePosition(sgl::Mouse->getX(), sgl::Mouse->getY());
    mousePosition.y = float(sgl::AppSettings::get()->getMainWindow()->getHeight()) - mousePosition.y - 1.0f;

    sgl::AABB2 aabb;
    aabb.min = glm::vec2(windowOffsetX, windowOffsetY);
    aabb.max = glm::vec2(windowOffsetX + float(fboWidthDisplay), windowOffsetY + float(fboHeightDisplay));

    return aabb.contains(mousePosition);
}

bool NanoVGWidget::isMouseOverDiagram(int parentX, int parentY, int parentWidth, int parentHeight) const {
    glm::vec2 mousePosition(sgl::Mouse->getX(), sgl::Mouse->getY());
    mousePosition.y = float(sgl::AppSettings::get()->getMainWindow()->getHeight()) - mousePosition.y - 1.0f;
    mousePosition.x -= float(parentX);
    mousePosition.y -= float(sgl::AppSettings::get()->getMainWindow()->getHeight() - parentY - 1 + parentHeight);

    sgl::AABB2 aabb;
    aabb.min = glm::vec2(windowOffsetX, windowOffsetY);
    aabb.max = glm::vec2(windowOffsetX + float(fboWidthDisplay), windowOffsetY + float(fboHeightDisplay));

    return aabb.contains(mousePosition);
}

void NanoVGWidget::render() {
    renderStart();
    renderBase();
    renderEnd();
}

void NanoVGWidget::renderStart() {
    if (!initialized) {
        _initialize();
    }

#if defined(SUPPORT_OPENGL) || defined(SUPPORT_VULKAN)
    RenderSystem renderSystem = sgl::AppSettings::get()->getRenderSystem();
#endif

#ifdef SUPPORT_OPENGL
    if (nanoVgBackend == NanoVGBackend::OPENGL) {
#ifdef SUPPORT_VULKAN
        if (renderSystem == RenderSystem::VULKAN) {
            GLenum srcLayout = GL_LAYOUT_COLOR_ATTACHMENT_EXT;
            if (shallClearBeforeRender) {
                rendererVk->insertImageMemoryBarrier(
                        renderTargetImageViewVk,
                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_ACCESS_NONE_KHR, VK_ACCESS_TRANSFER_WRITE_BIT);
                srcLayout = GL_LAYOUT_TRANSFER_DST_EXT;
            } else {
                if (renderTargetImageViewVk->getImage()->getVkImageLayout() == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
                    srcLayout = GL_LAYOUT_COLOR_ATTACHMENT_EXT;
                } else if (renderTargetImageViewVk->getImage()->getVkImageLayout() == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                    srcLayout = GL_LAYOUT_SHADER_READ_ONLY_EXT;
                } else if (renderTargetImageViewVk->getImage()->getVkImageLayout() == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                    srcLayout = GL_LAYOUT_TRANSFER_DST_EXT;
                }
            }

            sgl::vk::CommandBufferPtr commandBufferPre = rendererVk->getCommandBuffer();
            commandBufferPre->pushSignalSemaphore(interopSyncVkGl->getRenderReadySemaphore());
            rendererVk->endCommandBuffer();
            rendererVk->submitToQueue();
            interopSyncVkGl->getRenderReadySemaphore()->waitSemaphoreGl(renderTargetGl, srcLayout);
        }
#endif
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        sgl::Renderer->bindFBO(framebufferGl);
        glViewport(0, 0, fboWidthInternal, fboHeightInternal);
        if (shallClearBeforeRender) {
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClearDepth(0.0f);
            glClearStencil(0);
            glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        }
    }
#endif

#ifdef SUPPORT_VULKAN
    if (nanoVgBackend == NanoVGBackend::VULKAN) {
        vk::Swapchain* swapchain = AppSettings::get()->getSwapchain();
        size_t currentFrameIdx = 0;
        if (swapchain) {
            currentFrameIdx = swapchain->getCurrentFrame();
        }
        VkCommandBuffer commandBuffer = rendererVk->getVkCommandBuffer();
        vg = vgArray[currentFrameIdx];

        if (shallClearBeforeRender) {
            renderTargetImageViewVk->transitionImageLayout(
                    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, rendererVk->getVkCommandBuffer());
            renderTargetImageViewVk->clearColor(glm::vec4(0.0f), rendererVk->getVkCommandBuffer());
        }
        renderTargetImageViewVk->transitionImageLayout(
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, rendererVk->getVkCommandBuffer());

        NVGparams* vgParams = nvgInternalParams(vg);
        auto* vgVk = static_cast<VKNVGcontext*>(vgParams->userPtr);
        vgVk->createInfo.cmdBuffer = commandBuffer;
        vgVk->createInfo.renderpass = framebufferVk->getVkRenderPass();

        VkClearValue clearValues[2];
        clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
        clearValues[1].depthStencil = { 1.0f, 0 };

        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = framebufferVk->getVkRenderPass();
        renderPassBeginInfo.framebuffer = framebufferVk->getVkFramebuffer();
        renderPassBeginInfo.renderArea.extent = framebufferVk->getExtent2D();
        renderPassBeginInfo.clearValueCount = 2;
        renderPassBeginInfo.pClearValues = clearValues;
        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.width = float(framebufferVk->getWidth());
        viewport.height = float(framebufferVk->getHeight());
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = {uint32_t(framebufferVk->getWidth()), uint32_t(framebufferVk->getHeight()) };
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }
#endif

    nvgBeginFrame(vg, windowWidth, windowHeight, scaleFactor * float(supersamplingFactor));
}

void NanoVGWidget::renderEnd() {
    nvgEndFrame(vg);

#if defined(SUPPORT_OPENGL) || defined(SUPPORT_VULKAN)
    RenderSystem renderSystem = sgl::AppSettings::get()->getRenderSystem();
#endif

#if defined(SUPPORT_OPENGL) && defined(SUPPORT_VULKAN)
    if (nanoVgBackend == NanoVGBackend::OPENGL && renderSystem == RenderSystem::VULKAN) {
        sgl::vk::Device* device = sgl::AppSettings::get()->getPrimaryDevice();
        vk::Swapchain* swapchain = AppSettings::get()->getSwapchain();
        GLenum dstLayout = GL_LAYOUT_COLOR_ATTACHMENT_EXT;
        if (renderTargetImageViewVk->getImage()->getVkImageLayout() == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
            dstLayout = GL_LAYOUT_COLOR_ATTACHMENT_EXT;
        } else if (renderTargetImageViewVk->getImage()->getVkImageLayout() == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            dstLayout = GL_LAYOUT_SHADER_READ_ONLY_EXT;
        } else if (renderTargetImageViewVk->getImage()->getVkImageLayout() == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            dstLayout = GL_LAYOUT_TRANSFER_DST_EXT;
        }
        interopSyncVkGl->getRenderFinishedSemaphore()->signalSemaphoreGl(renderTargetGl, dstLayout);
        // 2023-01-22: With the Intel driver contained in Mesa 22.0.5, the synchronization didn't work as expected.
        if (device->getDeviceDriverId() == VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA) {
            glFinish();
        }
        sgl::vk::CommandBufferPtr commandBufferPost =
                commandBuffersPost.at(swapchain ? swapchain->getCurrentFrame() : 0);
        commandBufferPost->pushWaitSemaphore(
                interopSyncVkGl->getRenderFinishedSemaphore(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        rendererVk->pushCommandBuffer(commandBufferPost);
        rendererVk->beginCommandBuffer();
        rendererVk->insertImageMemoryBarrier(
                renderTargetImageViewVk,
                renderTargetImageViewVk->getImage()->getVkImageLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_ACCESS_NONE_KHR, VK_ACCESS_SHADER_READ_BIT);
        interopSyncVkGl->frameFinished();
    }
#endif

#ifdef SUPPORT_VULKAN
    if (nanoVgBackend == NanoVGBackend::VULKAN) {
        /*vk::Swapchain* swapchain = AppSettings::get()->getSwapchain();
        size_t currentFrameIdx = 0;
        if (swapchain) {
            currentFrameIdx = swapchain->getCurrentFrame();
        }
        VkCommandBuffer commandBuffer = nanovgCommandBuffers.at(currentFrameIdx)->getVkCommandBuffer();*/
        VkCommandBuffer commandBuffer = rendererVk->getVkCommandBuffer();
        vkCmdEndRenderPass(commandBuffer);
        rendererVk->clearGraphicsPipeline();

        //vkEndCommandBuffer(commandBuffer);
        //auto commandBuffer = rendererVk->getCommandBuffer();
        //rendererVk->pushCommandBuffer();
    }
#endif
}

#ifdef SUPPORT_OPENGL
void NanoVGWidget::blitToTargetGl(sgl::FramebufferObjectPtr& sceneFramebuffer) {
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
void NanoVGWidget::_createBlitRenderPass() {
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
    blitPassVk = std::make_shared<BlitRenderPassNanoVG>(rendererVk, shaderIds, blitMatrixBuffer);

    const auto& blitImageSettings = blitTargetVk->getImage()->getImageSettings();
    blitPassVk->setBlendMode(vk::BlendMode::BACK_TO_FRONT_PREMUL_ALPHA);
    blitPassVk->setOutputImageInitialLayout(blitInitialLayoutVk);
    blitPassVk->setOutputImageFinalLayout(blitFinalLayoutVk);
    blitPassVk->setAttachmentLoadOp(VK_ATTACHMENT_LOAD_OP_LOAD);
    //blitPassVk->setDepthWriteEnabled(false);
    //blitPassVk->setDepthTestEnabled(false);
    //blitPassVk->setDepthCompareOp(VK_COMPARE_OP_ALWAYS);
    blitPassVk->setCullMode(sgl::vk::CullMode::CULL_NONE);
    blitPassVk->setInputTexture(renderTargetTextureVk);
    blitPassVk->setOutputImage(blitTargetVk);
    blitPassVk->recreateSwapchain(blitImageSettings.width, blitImageSettings.height);
}

void NanoVGWidget::setBlitTargetVk(
        vk::ImageViewPtr& _blitTargetVk, VkImageLayout initialLayout, VkImageLayout finalLayout) {
    blitTargetVk = _blitTargetVk;
    blitInitialLayoutVk = initialLayout;
    blitFinalLayoutVk = finalLayout;
    _createBlitRenderPass();
}

void NanoVGWidget::blitToTargetVk() {
    glm::mat4 blitMatrix = sgl::matrixOrthogonalProjection(
            0.0f, float(blitTargetVk->getImage()->getImageSettings().width),
            0.0f, float(blitTargetVk->getImage()->getImageSettings().height),
            0.0f, 1.0f);
    sgl::AABB2 aabb;
    aabb.min = glm::vec2(windowOffsetX, windowOffsetY);
    aabb.max = glm::vec2(windowOffsetX + float(fboWidthDisplay), windowOffsetY + float(fboHeightDisplay));
    blitPassVk->setNormalizedCoordinatesAabb(aabb, nanoVgBackend == NanoVGBackend::OPENGL);
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
