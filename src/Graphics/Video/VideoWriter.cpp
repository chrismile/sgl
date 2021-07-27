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

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <GL/glew.h>

#include <Graphics/Window.hpp>
#include <Utils/AppSettings.hpp>
#include <Utils/Convert.hpp>
#include <Utils/File/Logfile.hpp>

#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/Utils/Device.hpp>
#include <Graphics/Vulkan/Utils/Swapchain.hpp>
#include <Graphics/Vulkan/Render/Renderer.hpp>
#endif

#include "VideoWriter.hpp"

namespace sgl {

VideoWriter::VideoWriter(const std::string& filename, int frameW, int frameH, int framerate, bool useAsyncCopy)
        : useAsyncCopy(useAsyncCopy), frameW(frameW), frameH(frameH), framebuffer(NULL) {
    if (useAsyncCopy) {
        initializeReadBackBuffers();
    }
    openFile(filename, framerate);
}

VideoWriter::VideoWriter(const std::string& filename, int framerate, bool useAsyncCopy)
        : useAsyncCopy(useAsyncCopy), framebuffer(NULL) {
    sgl::Window *window = sgl::AppSettings::get()->getMainWindow();
    frameW = window->getWidth();
    frameH = window->getHeight();
    if (useAsyncCopy) {
        initializeReadBackBuffers();
    }
    openFile(filename, framerate);
}

void VideoWriter::openFile(const std::string& filename, int framerate) {
    std::string command = std::string() + "ffmpeg -y -f rawvideo -s "
                          + sgl::toString(frameW) + "x" + sgl::toString(frameH)
                          + " -pix_fmt rgb24 -r " + sgl::toString(framerate)
                          //+ " -i - -vf vflip -an -b:v 100M \"" + filename + "\"";
                          + " -i - -vf vflip -an -vcodec libx264 -crf 5 \"" + filename + "\""; // -crf 15
    std::cout << command << std::endl;
#if defined(__linux__) ||defined(__MINGW32__)
    avfile = popen(command.c_str(), "w");
    if (avfile == NULL) {
        sgl::Logfile::get()->writeError("ERROR in VideoWriter::VideoWriter: Couldn't open file.");
        sgl::Logfile::get()->writeError(std::string() + "Error in errno: " + strerror(errno));
    }
#else
    sgl::Logfile::get()->writeInfo("Warning: Video writer is currently not supported on MSVC.");
#endif
}

VideoWriter::~VideoWriter() {
    if (AppSettings::get()->getRenderSystem() == RenderSystem::OPENGL) {
        if (useAsyncCopy) {
            while (!isReadBackBufferEmpty()) {
                readBackOldestFrame();
            }
        }
    }
#ifdef SUPPORT_VULKAN
    if (AppSettings::get()->getRenderSystem() == RenderSystem::VULKAN) {
        while (queueSize > 0) {
            readBackOldestFrameVulkan();
        }
    }
#endif

    if (framebuffer != NULL) {
#ifdef __USE_ISOC11
        free(framebuffer);
#else
        delete[] framebuffer;
#endif
    }
#if defined(__linux__) ||defined(__MINGW32__)
    if (avfile) {
        pclose(avfile);
    }
#endif
}

void VideoWriter::pushFrame(uint8_t* pixels) {
    if (avfile) {
        fwrite((const void *)pixels, frameW*frameH*3, 1, avfile);
    }
}

void VideoWriter::pushWindowFrame() {
    sgl::Window *window = sgl::AppSettings::get()->getMainWindow();
    if (frameW != window->getWidth() || frameH != window->getHeight()) {
        sgl::Logfile::get()->writeError("ERROR in VideoWriter::VideoWriter: Window size changed.");
        sgl::Logfile::get()->writeError(std::string()
                                        + "Expected " + sgl::toString(frameW) + "x" + sgl::toString(frameH)
                                        + ", but got " + sgl::toString(window->getWidth()) + "x" + sgl::toString(window->getHeight()) + ".");
        return;
    }
    if (framebuffer == NULL) {
        // Use 512-bit alignment for e.g. AVX-512, as ffmpeg might want to use vector instructions on the data stream.
#ifdef _ISOC11_SOURCE
        framebuffer = static_cast<uint8_t*>(aligned_alloc(64, frameW * frameH * 3)); // 512-bit aligned
#else
        framebuffer = new uint8_t[frameW * frameH * 3];
#endif
    }

    if (useAsyncCopy) {
        if (!isReadBackBufferFree()) {
            readBackOldestFrame();
        }
        addCurrentFrameToQueue();
        readBackFinishedFrames();
    } else {
        /*
         * For some reasons, we found manual synchronization to be necessary on NVIDIA GPUs under some circumstances.
         */
        GLsync fence;
        fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        bool renderingFinished = false;
        while (!renderingFinished) {
            GLenum signalType = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
            if (signalType == GL_ALREADY_SIGNALED || signalType == GL_CONDITION_SATISFIED) {
                renderingFinished = true;
            } else {
                if (signalType == GL_WAIT_FAILED) {
                    sgl::Logfile::get()->writeError("ERROR in VideoWriter::pushWindowFrame: Wait for sync failed.");
                    exit(-1);
                } else if (signalType == GL_TIMEOUT_EXPIRED) {
                    sgl::Logfile::get()->writeError(
                            "ERROR in VideoWriter::pushWindowFrame: Wait for sync has timed out.");
                    continue;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        glDeleteSync(fence);

        if (frameW % 4 != 0) {
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
        }
        glReadPixels(0, 0, frameW, frameH, GL_RGB, GL_UNSIGNED_BYTE, framebuffer);
        pushFrame(framebuffer);
    }
}

void VideoWriter::onSwapchainRecreated() {
    while (queueSize > 0) {
        readBackOldestFrameVulkan();
    }

    vk::Swapchain* swapchain = AppSettings::get()->getSwapchain();
    numSwapchainImages = swapchain ? swapchain->getNumImages() : 1;

    vk::Device* device = renderer->getDevice();
    while (numSwapchainImages > readBackImages.size()) {
        vk::ImageSettings imageSettings;
        imageSettings.width = frameW;
        imageSettings.width = frameH;
        imageSettings.format = VK_FORMAT_R8G8B8A8_UINT;
        imageSettings.tiling = VK_IMAGE_TILING_LINEAR;
        imageSettings.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageSettings.memoryUsage = VMA_MEMORY_USAGE_GPU_TO_CPU;
        readBackImages.push_back(vk::ImagePtr(new vk::Image(device, imageSettings)));
    }
    queueCapacity = numSwapchainImages;
}

void VideoWriter::readBackOldestFrameVulkan() {
    // Pop operation.
    vk::ImagePtr readBackImage = readBackImages.at(startPointer);
    startPointer = (startPointer + 1) % queueCapacity;
    queueSize--;

    uint8_t* mappedData = reinterpret_cast<uint8_t*>(readBackImage->mapMemory());
    int imageSize = frameW * frameH;
    for (int i = 0; i < imageSize; i++) {
        framebuffer[i * 3] = mappedData[i * 4];
        framebuffer[i * 3 + 1] = mappedData[i * 4 + 1];
        framebuffer[i * 3 + 2] = mappedData[i * 4 + 2];
    }
    readBackImage->unmapMemory();
    pushFrame(framebuffer);
}

void VideoWriter::pushFramebufferImage(vk::ImagePtr& image) {
    vk::Swapchain* swapchain = AppSettings::get()->getSwapchain();
    uint32_t imageIndex = swapchain ? swapchain->getImageIndex() : 0;

    if (imageIndex != endPointer) {
        Logfile::get()->throwError("Error in VideoWriter::pushFramebufferImage: imageIndex != endPointer");
    }

    // Queue full?
    if (queueCapacity == queueSize) {
        readBackOldestFrameVulkan();
    }

    // Copy the image data to the GPU -> CPU read-back image.
    vk::ImagePtr& readBackImage = readBackImages.at(imageIndex);
    image->blit(readBackImage, renderer->getVkCommandBuffer());
    endPointer = (endPointer + 1) % queueCapacity;
    queueSize++;
}

void VideoWriter::initializeReadBackBuffers() {
    for (size_t i = 0; i < NUM_RB_BUFFERS; i++) {
        glGenBuffers(1, &readBackBuffers[i].pbo);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, readBackBuffers[i].pbo);
        glBufferData(GL_PIXEL_PACK_BUFFER, frameW * frameH * 3, 0, GL_STREAM_READ);
    }
}

bool VideoWriter::isReadBackBufferFree() {
    return queueSize < queueCapacity;
}

bool VideoWriter::isReadBackBufferEmpty() {
    return queueSize == 0;
}

void VideoWriter::addCurrentFrameToQueue() {
    // 1. Query a free read back buffer.
    assert(isReadBackBufferFree());
    ReadBackBuffer& readBackBuffer = readBackBuffers[endPointer];

    // 2. Read the framebuffer data to the PBO (asynchronously).
    glBindBuffer(GL_PIXEL_PACK_BUFFER, readBackBuffer.pbo);
    //glReadBuffer(GL_BACK);
    if (frameW % 4 != 0) {
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
    }
    glReadPixels(0, 0, frameW, frameH, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    // 3. Add a fence sync to later wait on. Push the read back buffer on the queue.
    assert(readBackBuffer.fence == nullptr);
    readBackBuffer.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    endPointer = (endPointer + 1) % queueCapacity;
    queueSize++;
}

void VideoWriter::readBackFinishedFrames() {
    while (queueSize > 0) {
        ReadBackBuffer& readBackBuffer = readBackBuffers[startPointer];
        assert(readBackBuffer.fence != nullptr);

        GLenum signalType = glClientWaitSync(readBackBuffer.fence, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
        if (signalType == GL_ALREADY_SIGNALED || signalType == GL_CONDITION_SATISFIED) {
            glDeleteSync(readBackBuffer.fence);
            readBackBuffer.fence = nullptr;

            glBindBuffer(GL_COPY_READ_BUFFER, readBackBuffer.pbo);
            char *pboData = reinterpret_cast<char*>(glMapBufferRange(
                    GL_COPY_READ_BUFFER, 0, frameW * frameH * 3, GL_MAP_READ_BIT));
            memcpy(framebuffer, pboData, frameW * frameH * 3);
            glUnmapBuffer(GL_COPY_READ_BUFFER);
            pushFrame(framebuffer);

            // Pop operation.
            startPointer = (startPointer + 1) % queueCapacity;
            queueSize--;
        } else {
            if (signalType == GL_WAIT_FAILED) {
                // Fail gracefully.
                sgl::Logfile::get()->writeError("ERROR in VideoWriter::readBackOldestFrame: Wait for sync failed.");

                glDeleteSync(readBackBuffer.fence);
                readBackBuffer.fence = nullptr;

                // Pop operation.
                startPointer = (startPointer + 1) % queueCapacity;
                queueSize--;

                break;
            } else if (signalType == GL_TIMEOUT_EXPIRED) {
                // Nothing ready yet.
                break;
            }
        }
    }
}

void VideoWriter::readBackOldestFrame() {
    ReadBackBuffer& readBackBuffer = readBackBuffers[startPointer];
    assert(readBackBuffer.fence != nullptr);

    bool renderingFinished = false;
    while (true) {
        GLenum signalType = glClientWaitSync(readBackBuffer.fence, GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
        if (signalType == GL_ALREADY_SIGNALED || signalType == GL_CONDITION_SATISFIED) {
            renderingFinished = true;
            break;
        } else {
            if (signalType == GL_WAIT_FAILED) {
                // Fail gracefully.
                sgl::Logfile::get()->writeError("ERROR in VideoWriter::readBackOldestFrame: Wait for sync failed.");
                //exit(-1);
                break;
            } else if (signalType == GL_TIMEOUT_EXPIRED) {
                sgl::Logfile::get()->writeError(
                        "WARNING in VideoWriter::readBackOldestFrame: Wait for sync has timed out.");
                continue;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    glDeleteSync(readBackBuffer.fence);
    readBackBuffer.fence = nullptr;

    if (renderingFinished) {
        glBindBuffer(GL_COPY_READ_BUFFER, readBackBuffer.pbo);
        char *pboData = reinterpret_cast<char*>(glMapBufferRange(
                GL_COPY_READ_BUFFER, 0, frameW * frameH * 3, GL_MAP_READ_BIT));
        memcpy(framebuffer, pboData, frameW * frameH * 3);
        glUnmapBuffer(GL_COPY_READ_BUFFER);
        pushFrame(framebuffer);
    }

    // Pop operation.
    startPointer = (startPointer + 1) % queueCapacity;
    queueSize--;
}

}
