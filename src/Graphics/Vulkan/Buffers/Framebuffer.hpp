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

#ifndef SGL_FRAMEBUFFER_HPP
#define SGL_FRAMEBUFFER_HPP

#include <memory>
#include <vulkan/vulkan.h>

namespace sgl { namespace vk {

class Device;
class ImageView;
typedef std::shared_ptr<ImageView> ImageViewPtr;

struct AttachmentState {
    static AttachmentState standardColorAttachment() {
        AttachmentState attachmentState;
        attachmentState.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        return attachmentState;
    }
    static AttachmentState standardDepthStencilAttachment() {
        AttachmentState attachmentState;
        attachmentState.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        return attachmentState;
    }
    static AttachmentState standardResolveAttachment() {
        AttachmentState attachmentState;
        attachmentState.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        return attachmentState;
    }
    VkAttachmentLoadOp  loadOp          = VK_ATTACHMENT_LOAD_OP_LOAD;
    VkAttachmentStoreOp storeOp         = VK_ATTACHMENT_STORE_OP_STORE;
    VkAttachmentLoadOp  stencilLoadOp   = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    VkAttachmentStoreOp stencilStoreOp  = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    VkImageLayout       initialLayout   = VK_IMAGE_LAYOUT_UNDEFINED;
    // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    // VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, ...
    // For direct rendering to screen framebuffer: VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    VkImageLayout       finalLayout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
};

class DLL_OBJECT Framebuffer {
public:
    Framebuffer(Device* device, uint32_t width, uint32_t height, uint32_t layers = 1);
    ~Framebuffer();

    /**
     * Sets the color attachment at the specified index.
     * This function may only be called before a call to @see build or @see getVkFramebuffer.
     * The color attachment at the specified index can be used in GLSL as: layout(location = [index]) out [type] [name];
     * @param imageView The image view to use as an attachment.
     * @param index The index of the attachment.
     */
    void setColorAttachment(
            ImageViewPtr& attachmentImageView, int index,
            const AttachmentState& attachmentState = AttachmentState::standardColorAttachment()) {
        if (int(colorAttachments.size()) <= index) {
            colorAttachments.resize(index + 1);
            colorAttachmentStates.resize(index + 1);
        }
        colorAttachments.at(index) = attachmentImageView;
        colorAttachmentStates.at(index) = attachmentState;
    }
    /**
     * Sets the depth-stencil attachment.
     * This function may only be called before a call to @see build or @see getVkFramebuffer.
     * @param imageView The image view to use as an attachment.
     */
    inline void setDepthStencilAttachment(
            ImageViewPtr& attachmentImageView,
            const AttachmentState& attachmentState = AttachmentState::standardDepthStencilAttachment()) {
        depthStencilAttachment = attachmentImageView;
        depthStencilAttachmentState = attachmentState;
    }
    /**
     * Sets the resolve attachment.
     * This function may only be called before a call to @see build or @see getVkFramebuffer.
     * @param imageView The image view to use as an attachment.
     */
    inline void setResolveAttachment(
            ImageViewPtr& attachmentImageView,
            const AttachmentState& attachmentState = AttachmentState::standardResolveAttachment()) {
        resolveAttachment = attachmentImageView;
        resolveAttachmentState = attachmentState;
    }
    /**
     * Sets the input attachment at the specified index.
     * This function may only be called before a call to @see build or @see getVkFramebuffer.
     * @param imageView The image view to use as an attachment.
     * @param index The index of the attachment.
     */
    void setInputAttachment(
            ImageViewPtr& attachmentImageView, int index,
            const AttachmentState& attachmentState = AttachmentState::standardColorAttachment()) {
        if (int(inputAttachments.size()) <= index) {
            inputAttachments.resize(index + 1);
            inputAttachmentStates.resize(index + 1);
        }
        inputAttachments.at(index) = attachmentImageView;
        inputAttachmentStates.at(index) = attachmentState;
    }

    /**
     * Builds and finalizes the internal representation.
     */
    void build();

    inline uint32_t getWidth() const { return width; }
    inline uint32_t getHeight() const { return height; }
    inline uint32_t getLayers() const { return layers; }
    inline VkExtent2D getExtent2D() const { return { width, height }; }

    inline VkFramebuffer getVkFramebuffer() { if (framebuffer == VK_NULL_HANDLE) build(); return framebuffer; }
    inline VkRenderPass getVkRenderPass() { if (renderPass == VK_NULL_HANDLE) build(); return renderPass; }

private:
    Device* device;
    uint32_t width, height, layers;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;

    std::vector<ImageViewPtr> colorAttachments;
    std::vector<AttachmentState> colorAttachmentStates;
    ImageViewPtr depthStencilAttachment;
    AttachmentState depthStencilAttachmentState;
    ImageViewPtr resolveAttachment;
    AttachmentState resolveAttachmentState;
    std::vector<ImageViewPtr> inputAttachments;
    std::vector<AttachmentState> inputAttachmentStates;
};

typedef std::shared_ptr<Framebuffer> FramebufferPtr;

}}

#endif //SGL_FRAMEBUFFER_HPP
