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

#ifndef SGL_SCREENSHOTREADBACKHELPER_HPP
#define SGL_SCREENSHOTREADBACKHELPER_HPP

#include <string>
#include <vector>
#include <memory>

namespace sgl { namespace vk {

class Renderer;
class Image;
typedef std::shared_ptr<Image> ImagePtr;

class DLL_OBJECT ScreenshotReadbackHelper {
public:
    explicit ScreenshotReadbackHelper(vk::Renderer* renderer) : renderer(renderer) {}
    ~ScreenshotReadbackHelper();

    void onSwapchainRecreated();
    void onSwapchainRecreated(uint32_t width, uint32_t height);
    void requestScreenshotReadback(vk::ImagePtr& image, const std::string& filename);
    void saveDataIfAvailable(uint32_t imageIndex);
    void setScreenshotTransparentBackground(bool transparentBackground);

private:
    vk::Renderer* renderer = nullptr;

    struct FrameData {
        vk::ImagePtr image;
        std::string filename;
        bool used = false;
    };
    std::vector<FrameData> frameDataList;
    bool screenshotTransparentBackground = false;
};

typedef std::shared_ptr<ScreenshotReadbackHelper> ScreenshotReadbackHelperPtr;

}}

#endif //SGL_SCREENSHOTREADBACKHELPER_HPP
