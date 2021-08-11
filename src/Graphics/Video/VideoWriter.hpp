/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2020, Christoph Neuhauser
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

#ifndef SGL_VIDEOWRITER_HPP
#define SGL_VIDEOWRITER_HPP

#include <string>
#include <cstdio>

#ifdef SUPPORT_OPENGL
#include <Graphics/OpenGL/RendererGL.hpp>
#endif

#ifdef SUPPORT_VULKAN
namespace sgl { namespace vk {
class Image;
typedef std::shared_ptr<Image> ImagePtr;
class Renderer;
}}
#endif

namespace sgl {

/** Video writer using the libav command line tool. Supports mp4 video.
 * Please install the necessary dependencies for this writer to work:
 * https://wiki.ubuntuusers.de/avconv/
 */
class DLL_OBJECT VideoWriter
{
public:
    /// Open mp4 video file with specified frame width and height
    VideoWriter(const std::string& filename, int frameW, int frameH, int framerate = 30, bool useAsyncCopy = true);
    /// Open mp4 video file with frame width and height specified by application window
    VideoWriter(const std::string& filename, int framerate = 30, bool useAsyncCopy = true);
    /// Closes file automatically
    ~VideoWriter();
    /// Push a 24-bit RGB frame (with width and height specified in constructor)
    void pushFrame(uint8_t* pixels);

#ifdef SUPPORT_OPENGL
    /// Retrieves frame automatically from current window
    void pushWindowFrame();
#endif

#ifdef SUPPORT_VULKAN
    inline void setRenderer(vk::Renderer* renderer) {
        this->renderer = renderer;
        if (AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
            onSwapchainRecreated();
        }
    }
    /**
     * Retrieves frame from the passed framebuffer image. The flag VK_BUFFER_USAGE_TRANSFER_SRC_BIT needs to be set!
     * @param framebuffer
     */
    void pushFramebufferImage(vk::ImagePtr& image);
    void onSwapchainRecreated();
#endif

private:
    void openFile(const std::string& filename, int framerate = 25);
    void createCpuBufferData(int width, int height);

#ifdef SUPPORT_OPENGL
    // Asynchronous CPU/GPU data transfer.
    void initializeReadBackBuffers();
    bool isReadBackBufferFree();
    bool isReadBackBufferEmpty();
    void addCurrentFrameToQueue();
    void readBackFinishedFrames();
    void readBackOldestFrame();
    bool useAsyncCopy;
    static const size_t NUM_RB_BUFFERS = 4; ///< Sufficient for up to 4 frames queued at the same time.
    struct ReadBackBuffer {
        GLuint pbo = 0u;
        GLsync fence = nullptr;
    };
    ReadBackBuffer readBackBuffers[NUM_RB_BUFFERS];
    size_t startPointer = 0, endPointer = 0;
    size_t queueCapacity = NUM_RB_BUFFERS;
    size_t queueSize = 0;
#endif

#ifdef SUPPORT_VULKAN
    void readBackOldestFrameVulkan();
    size_t numSwapchainImages = 1;
    vk::Renderer* renderer = nullptr;
    //std::vector<vk::BufferPtr> readBackBuffersVk;
    std::vector<vk::ImagePtr> readBackImages;
    VkSubresourceLayout readBackImageSubresourceLayout{};
#endif

    // Frame & file data.
    FILE *avfile = nullptr;
    int frameW = 0;
    int frameH = 0;
    uint8_t *framebuffer = nullptr; ///< Used for pushWindowFrame
};

}

#endif //SGL_VIDEOWRITER_HPP
