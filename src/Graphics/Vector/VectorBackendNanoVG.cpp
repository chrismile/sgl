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

#include "nanovg/nanovg.h"

#include <Utils/File/Logfile.hpp>

#ifdef SUPPORT_OPENGL
#include <GL/glew.h>
#include <Graphics/Renderer.hpp>
#include <Graphics/Buffers/FBO.hpp>
#include <Graphics/Texture/Texture.hpp>
#include <Graphics/Texture/TextureManager.hpp>
#include <Graphics/OpenGL/ShaderManager.hpp>
#include <Graphics/OpenGL/Texture.hpp>
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg/nanovg_gl.h"
#endif

#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/Utils/Swapchain.hpp>
#include <Graphics/Vulkan/Buffers/Framebuffer.hpp>
#include <Graphics/Vulkan/Render/Renderer.hpp>
#include <Graphics/Vulkan/Render/CommandBuffer.hpp>
#include <Graphics/Vulkan/Render/Passes/BlitRenderPass.hpp>
#include <memory>
#define NANOVG_VULKAN_IMPLEMENTATION
#include "nanovg/nanovg_vk.h"
#endif

#if defined(SUPPORT_OPENGL) && defined(SUPPORT_VULKAN)
#include <Graphics/Vulkan/Utils/Interop.hpp>
#endif

#include "VectorWidget.hpp"
#include "VectorBackendNanoVG.hpp"

namespace sgl {

NanoVGSettings::NanoVGSettings() {
    RenderSystem renderSystem = sgl::AppSettings::get()->getRenderSystem();
    if (renderSystem == RenderSystem::OPENGL) {
        renderBackend = RenderSystem::OPENGL;
    } else if (renderSystem == RenderSystem::VULKAN) {
        renderBackend = RenderSystem::VULKAN;
    } else {
        sgl::Logfile::get()->throwError(
                "Error in NanoVGSettings::NanoVGSettings: Encountered unsupported render system.");
        renderBackend = RenderSystem::VULKAN;
    }

#ifndef NDEBUG
    useDebugging = true;
#else
    useDebugging = false;
#endif
}

bool VectorBackendNanoVG::checkIsSupported() {
    return true;
}

VectorBackendNanoVG::VectorBackendNanoVG(VectorWidget* vectorWidget, const NanoVGSettings& nanoVgSettings)
        : VectorBackend(vectorWidget) {
    useMsaa = nanoVgSettings.useMsaa;
    numMsaaSamples = nanoVgSettings.numMsaaSamples;
    supersamplingFactor = nanoVgSettings.supersamplingFactor;

    renderBackend = nanoVgSettings.renderBackend;

#ifndef SUPPORT_OPENGL
    if (renderBackend == RenderSystem::OPENGL) {
        sgl::Logfile::get()->throwError(
                "Error in NanoVGWrapper::NanoVGWrapper: OpenGL backend selected, but OpenGL is not supported.");
    }
#endif

#ifndef SUPPORT_VULKAN
    if (renderBackend == RenderSystem::VULKAN) {
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

void VectorBackendNanoVG::initialize() {
    if (initialized) {
        return;
    }
    initialized = true;

#if defined(SUPPORT_OPENGL) && defined(SUPPORT_VULKAN)
    RenderSystem renderSystem = sgl::AppSettings::get()->getRenderSystem();
#endif

#ifdef SUPPORT_OPENGL
    if (renderBackend == RenderSystem::OPENGL) {
        vg = nvgCreateGL3(flags);
        _initializeFont(vg);
    }
#endif

#ifdef SUPPORT_VULKAN
    if (renderBackend == RenderSystem::VULKAN) {
        vk::Device* device = AppSettings::get()->getPrimaryDevice();
        vk::Swapchain* swapchain = AppSettings::get()->getSwapchain();

        vk::CommandPoolType commandPoolType;
        commandPoolType.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        nanovgCommandBuffers = device->allocateCommandBuffers(
                commandPoolType, &commandPool, swapchain ? uint32_t(swapchain->getMaxNumFramesInFlight()) : 1);

        if (!framebufferVk) {
            vectorWidget->onWindowSizeChanged();
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
        vg = vgArray[0];
    }
#endif

#if defined(SUPPORT_OPENGL) && defined(SUPPORT_VULKAN)
    if (renderBackend == RenderSystem::OPENGL && renderSystem == RenderSystem::VULKAN) {
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
    if ((renderBackend == RenderSystem::OPENGL) != (renderSystem == RenderSystem::OPENGL)) {
        vk::Device* device = AppSettings::get()->getPrimaryDevice();
        vk::Swapchain* swapchain = AppSettings::get()->getSwapchain();
        int maxNumFramesInFlight = swapchain ? swapchain->getMaxNumFramesInFlight() : 1;
        interopSyncVkGl = std::make_shared<sgl::InteropSyncVkGl>(device, maxNumFramesInFlight);
    }
#endif
}

void VectorBackendNanoVG::destroy() {
    if (!initialized) {
        return;
    }

#ifdef SUPPORT_OPENGL
    if (renderBackend == RenderSystem::OPENGL) {
        if (vg) {
            nvgDeleteGL3(vg);
            vg = nullptr;
        }
    }
#endif

#ifdef SUPPORT_VULKAN
    if (renderBackend == RenderSystem::VULKAN) {
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

    initialized = false;
}

void VectorBackendNanoVG::_initializeFont(NVGcontext* vgCurrent) {
    std::string fontFilename = sgl::AppSettings::get()->getDataDirectory() + "Fonts/DroidSans.ttf";
    int font = nvgCreateFont(vgCurrent, "sans", fontFilename.c_str());
    if (font == -1) {
        sgl::Logfile::get()->throwError("Error in NanoVGWrapper::_initializeFont: Couldn't find the font file.");
    }
}

void VectorBackendNanoVG::onResize() {
#if defined(SUPPORT_OPENGL) || defined(SUPPORT_VULKAN)
    RenderSystem renderSystem = sgl::AppSettings::get()->getRenderSystem();
#endif

#ifdef SUPPORT_OPENGL
    if (renderSystem == RenderSystem::OPENGL && renderBackend != RenderSystem::VULKAN) {
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
    if (renderSystem == RenderSystem::VULKAN || renderBackend == RenderSystem::VULKAN) {
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
        if (renderBackend == RenderSystem::OPENGL) {
            imageSettings.exportMemory = true;
        }
#endif
        sgl::vk::ImageSamplerSettings samplerSettings;
        renderTargetTextureVk = std::make_shared<sgl::vk::Texture>(device, imageSettings, samplerSettings);
#ifdef SUPPORT_OPENGL
        if (renderBackend == RenderSystem::OPENGL) {
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
    if (renderBackend == RenderSystem::OPENGL) {
        depthStencilRbo = sgl::Renderer->createRBO(
                fboWidthInternal, fboHeightInternal, sgl::RBO_DEPTH24_STENCIL8,
                useMsaa ? numMsaaSamples : 0);

        framebufferGl = sgl::Renderer->createFBO();
        framebufferGl->bindTexture(renderTargetGl, sgl::COLOR_ATTACHMENT);
        framebufferGl->bindRenderbuffer(depthStencilRbo, sgl::DEPTH_STENCIL_ATTACHMENT);
    }
#endif
}

void VectorBackendNanoVG::renderStart() {
    if (!initialized) {
        initialize();
    }

#ifdef SUPPORT_VULKAN
    RenderSystem renderSystem = sgl::AppSettings::get()->getRenderSystem();
#endif

#ifdef SUPPORT_OPENGL
    if (renderBackend == RenderSystem::OPENGL) {
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
            glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
            glClearDepth(0.0f);
            glClearStencil(0);
            glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        }
    }
#endif

#ifdef SUPPORT_VULKAN
    if (renderBackend == RenderSystem::VULKAN) {
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
            renderTargetImageViewVk->clearColor(clearColor, rendererVk->getVkCommandBuffer());
        }
        renderTargetImageViewVk->transitionImageLayout(
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, rendererVk->getVkCommandBuffer());

        NVGparams* vgParams = nvgInternalParams(vg);
        auto* vgVk = static_cast<VKNVGcontext*>(vgParams->userPtr);
        vgVk->createInfo.cmdBuffer = commandBuffer;
        vgVk->createInfo.renderpass = framebufferVk->getVkRenderPass();

        VkClearValue clearValues[2];
        clearValues[0].color = { { clearColor.r, clearColor.g, clearColor.b, clearColor.a } };
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

void VectorBackendNanoVG::renderEnd() {
    nvgEndFrame(vg);

#if defined(SUPPORT_OPENGL) && defined(SUPPORT_VULKAN)
    RenderSystem renderSystem = sgl::AppSettings::get()->getRenderSystem();
#endif

#if defined(SUPPORT_OPENGL) && defined(SUPPORT_VULKAN)
    if (renderBackend == RenderSystem::OPENGL && renderSystem == RenderSystem::VULKAN) {
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
    if (renderBackend == RenderSystem::VULKAN) {
        VkCommandBuffer commandBuffer = rendererVk->getVkCommandBuffer();
        vkCmdEndRenderPass(commandBuffer);
        rendererVk->clearGraphicsPipeline();
    }
#endif
}

}

