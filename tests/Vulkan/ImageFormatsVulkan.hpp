/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2025, Christoph Neuhauser
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

#ifndef SGL_TESTS_IMAGEFORMATS_HPP
#define SGL_TESTS_IMAGEFORMATS_HPP

const auto testedImageFormats = testing::Values(
        // bool entry shows if this format is required (true) or optional (false).
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_R32_SFLOAT, 1024, 1024, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_R32G32_SFLOAT, 1024, 1024, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_R32G32B32A32_SFLOAT, 1024, 1024, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_R16_SFLOAT, 1024, 1024, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_R16G16_SFLOAT, 1024, 1024, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_R16G16B16A16_SFLOAT, 1024, 1024, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_R8G8B8A8_UNORM, 1024, 1024, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_B8G8R8A8_UNORM, 1024, 1024, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_D32_SFLOAT, 1024, 1024, false},
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_R32_UINT, 1024, 1024, false}
        // Level Zero has no ZE_IMAGE_FORMAT_LAYOUT_64 image format layout.
        //{VK_FORMAT_R64_UINT, false}
);

const auto testedImageFormatsCopy = testing::Values(
        // bool entry shows if this format is required (true) or optional (false).
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_R32_SFLOAT, 1024, 1024, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_R32G32_SFLOAT, 1024, 1024, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_R32G32B32A32_SFLOAT, 1024, 1024, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_D32_SFLOAT, 1024, 1024, false},
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_R32_UINT, 1024, 1024, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_R32G32_UINT, 1024, 1024, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_R32G32B32A32_UINT, 1024, 1024, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_R16_UINT, 128, 128, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_R16G16_UINT, 128, 128, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_R16G16B16A16_UINT, 128, 128, true},
        // Maximum representable integer value is 2048 for float16_t.
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_R16_SFLOAT, 64, 32, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_R16G16_SFLOAT, 32, 32, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool>{VK_FORMAT_R16G16B16A16_SFLOAT, 32, 16, true}
);

const auto testedImageFormatsReadWriteAsync = testing::Values(
        // second bool entry shows if semaphores should be used
        // third bool entry shows if this format is required (true) or optional (false).
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R32_SFLOAT, 1024, 1024, true, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R32G32_SFLOAT, 1024, 1024, true, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R32G32B32A32_SFLOAT, 1024, 1024, true, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R32G32B32A32_SFLOAT, 512, 256, true, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R32G32B32A32_SFLOAT, 515, 201, true, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_D32_SFLOAT, 1024, 1024, true, false},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R32_UINT, 1024, 1024, true, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R32G32_UINT, 1024, 1024, true, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R32G32B32A32_UINT, 1024, 1024, true, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R16_UINT, 128, 128, true, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R16G16_UINT, 128, 128, true, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R16G16B16A16_UINT, 128, 128, true, true},
        // Maximum representable integer value is 2048 for float16_t.
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R16_SFLOAT, 64, 32, true, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R16G16_SFLOAT, 32, 32, true, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R16G16B16A16_SFLOAT, 32, 16, true, true}
);

const auto testedImageFormatsReadWriteSync = testing::Values(
        // second bool entry shows if semaphores should be used
        // third bool entry shows if this format is required (true) or optional (false).
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R32_SFLOAT, 1024, 1024, false, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R32G32_SFLOAT, 1024, 1024, false, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R32G32B32A32_SFLOAT, 1024, 1024, false, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R32G32B32A32_SFLOAT, 512, 256, false, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R32G32B32A32_SFLOAT, 515, 201, false, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_D32_SFLOAT, 1024, 1024, false, false},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R32_UINT, 1024, 1024, false, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R32G32_UINT, 1024, 1024, false, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R32G32B32A32_UINT, 1024, 1024, false, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R16_UINT, 128, 128, false, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R16G16_UINT, 128, 128, false, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R16G16B16A16_UINT, 128, 128, false, true},
        // Maximum representable integer value is 2048 for float16_t.
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R16_SFLOAT, 64, 32, false, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R16G16_SFLOAT, 32, 32, false, true},
        std::tuple<VkFormat, uint32_t, uint32_t, bool, bool>{VK_FORMAT_R16G16B16A16_SFLOAT, 32, 16, false, true}
);

#endif //SGL_TESTS_IMAGEFORMATS_HPP
