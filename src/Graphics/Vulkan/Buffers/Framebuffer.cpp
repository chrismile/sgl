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
#include "../Utils/Device.hpp"
#include "../Image/Image.hpp"
#include "Framebuffer.hpp"

namespace sgl { namespace vk {

Framebuffer::Framebuffer(Device* device, uint32_t width, uint32_t height, uint32_t layers)
        : device(device), width(width), height(height), layers(layers) {
}

void Framebuffer::build() {
    size_t attachmentCounter = 0;

    std::vector<VkAttachmentDescription> attachmentDescriptions;
    attachmentDescriptions.reserve(
            colorAttachments.size() + (depthStencilAttachment ? 1 : 0) + (resolveAttachment ? 1 : 0));

    std::vector<VkAttachmentReference> colorAttachmentReferences;
    colorAttachmentReferences.reserve(colorAttachments.size());
    for (size_t i = 0; i < colorAttachments.size(); i++) {
        ImageViewPtr& imageView = colorAttachments.at(i);
        AttachmentState& attachmentState = colorAttachmentStates.at(i);
        VkAttachmentDescription attachmentDescription = {};
        attachmentDescription.format = imageView->getImage()->getImageSettings().format;
        attachmentDescription.samples = imageView->getImage()->getImageSettings().numSamples;
        attachmentDescription.loadOp = attachmentState.loadOp;
        attachmentDescription.storeOp = attachmentState.storeOp;
        attachmentDescription.stencilLoadOp = attachmentState.stencilLoadOp;
        attachmentDescription.stencilStoreOp = attachmentState.stencilStoreOp;
        attachmentDescription.initialLayout = attachmentState.initialLayout;
        attachmentDescription.finalLayout = attachmentState.finalLayout;
        attachmentDescriptions.push_back(attachmentDescription);

        VkAttachmentReference attachmentReference = {};
        attachmentReference.attachment = attachmentCounter;
        attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentReferences.push_back(attachmentReference);

        attachmentCounter++;
    }

    std::vector<VkAttachmentReference> inputAttachmentReferences;
    inputAttachmentReferences.reserve(inputAttachments.size());
    for (size_t i = 0; i < inputAttachments.size(); i++) {
        ImageViewPtr& imageView = inputAttachments.at(i);
        AttachmentState& attachmentState = inputAttachmentStates.at(i);
        VkAttachmentDescription attachmentDescription = {};
        attachmentDescription.format = imageView->getImage()->getImageSettings().format;
        attachmentDescription.samples = imageView->getImage()->getImageSettings().numSamples;
        attachmentDescription.loadOp = attachmentState.loadOp;
        attachmentDescription.storeOp = attachmentState.storeOp;
        attachmentDescription.stencilLoadOp = attachmentState.stencilLoadOp;
        attachmentDescription.stencilStoreOp = attachmentState.stencilStoreOp;
        attachmentDescription.initialLayout = attachmentState.initialLayout;
        attachmentDescription.finalLayout = attachmentState.finalLayout;
        attachmentDescriptions.push_back(attachmentDescription);

        VkAttachmentReference attachmentReference = {};
        attachmentReference.attachment = attachmentCounter;
        attachmentReference.layout = attachmentDescription.finalLayout;
        inputAttachmentReferences.push_back(attachmentReference);

        attachmentCounter++;
    }

    VkAttachmentDescription depthStencilAttachmentDescription = {};
    VkAttachmentReference depthStencilAttachmentReference = {};
    if (depthStencilAttachment) {
        depthStencilAttachmentDescription.format = depthStencilAttachment->getImage()->getImageSettings().format;
        depthStencilAttachmentDescription.samples = depthStencilAttachment->getImage()->getImageSettings().numSamples;
        depthStencilAttachmentDescription.loadOp = depthStencilAttachmentState.loadOp;
        depthStencilAttachmentDescription.storeOp = depthStencilAttachmentState.storeOp;
        depthStencilAttachmentDescription.stencilLoadOp = depthStencilAttachmentState.stencilLoadOp;
        depthStencilAttachmentDescription.stencilStoreOp = depthStencilAttachmentState.stencilStoreOp;
        depthStencilAttachmentDescription.initialLayout = depthStencilAttachmentState.initialLayout;
        depthStencilAttachmentDescription.finalLayout = depthStencilAttachmentState.finalLayout;
        attachmentDescriptions.push_back(depthStencilAttachmentDescription);

        depthStencilAttachmentReference.attachment = attachmentCounter;
        depthStencilAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachmentCounter++;
    }

    VkAttachmentDescription resolveAttachmentDescription = {};
    VkAttachmentReference resolveAttachmentReference = {};
    if (resolveAttachment) {
        resolveAttachmentDescription.format = resolveAttachment->getImage()->getImageSettings().format;
        resolveAttachmentDescription.samples = resolveAttachment->getImage()->getImageSettings().numSamples;
        resolveAttachmentDescription.loadOp = resolveAttachmentState.loadOp;
        resolveAttachmentDescription.storeOp = resolveAttachmentState.storeOp;
        resolveAttachmentDescription.stencilLoadOp = resolveAttachmentState.stencilLoadOp;
        resolveAttachmentDescription.stencilStoreOp = resolveAttachmentState.stencilStoreOp;
        resolveAttachmentDescription.initialLayout = resolveAttachmentState.initialLayout;
        resolveAttachmentDescription.finalLayout = resolveAttachmentState.finalLayout;
        attachmentDescriptions.push_back(resolveAttachmentDescription);

        resolveAttachmentReference.attachment = attachmentCounter;
        resolveAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachmentCounter++;
    }


    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    subpass.colorAttachmentCount = uint32_t(colorAttachmentReferences.size());
    subpass.pColorAttachments = colorAttachmentReferences.empty() ? nullptr : colorAttachmentReferences.data();
    subpass.inputAttachmentCount = uint32_t(colorAttachmentReferences.size());
    subpass.pInputAttachments = colorAttachmentReferences.empty() ? nullptr : colorAttachmentReferences.data();
    subpass.pDepthStencilAttachment = depthStencilAttachment ? &depthStencilAttachmentReference : nullptr;
    subpass.pResolveAttachments = resolveAttachment ? &resolveAttachmentReference : nullptr;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = 0;
    if (!colorAttachments.empty()) {
        dependency.dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    if (depthStencilAttachment) {
        dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
    renderPassInfo.pAttachments = attachmentDescriptions.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device->getVkDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        Logfile::get()->throwError("Error in Framebuffer::build: Could not create a render pass.");
    }

    std::vector<VkImageView> attachments;
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.layers = layers;

    if (vkCreateFramebuffer(device->getVkDevice(), &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
        Logfile::get()->throwError("Error: Framebuffer::Framebuffer: Could not create a framebuffer.");
    }
}

Framebuffer::~Framebuffer() {
    if (renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device->getVkDevice(), renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }
    if (framebuffer != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(device->getVkDevice(), framebuffer, nullptr);
        framebuffer = VK_NULL_HANDLE;
    }
}

}}
