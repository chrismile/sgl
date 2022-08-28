/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2022, Christoph Neuhauser
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

#include "Status.hpp"

namespace sgl { namespace vk {

#define RES_TO_STR(r) case r: return #r

std::string vulkanResultToString(VkResult res) {
    switch(res) {
        RES_TO_STR(VK_SUCCESS);
        RES_TO_STR(VK_TIMEOUT);
        RES_TO_STR(VK_EVENT_SET);
        RES_TO_STR(VK_EVENT_RESET);
        RES_TO_STR(VK_INCOMPLETE);
        RES_TO_STR(VK_ERROR_OUT_OF_HOST_MEMORY);
        RES_TO_STR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
        RES_TO_STR(VK_ERROR_INITIALIZATION_FAILED);
        RES_TO_STR(VK_ERROR_DEVICE_LOST);
        RES_TO_STR(VK_ERROR_MEMORY_MAP_FAILED);
        RES_TO_STR(VK_ERROR_LAYER_NOT_PRESENT);
        RES_TO_STR(VK_ERROR_EXTENSION_NOT_PRESENT);
        RES_TO_STR(VK_ERROR_FEATURE_NOT_PRESENT);
        RES_TO_STR(VK_ERROR_INCOMPATIBLE_DRIVER);
        RES_TO_STR(VK_ERROR_TOO_MANY_OBJECTS);
        RES_TO_STR(VK_ERROR_FORMAT_NOT_SUPPORTED);
        RES_TO_STR(VK_ERROR_SURFACE_LOST_KHR);
        RES_TO_STR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
        RES_TO_STR(VK_SUBOPTIMAL_KHR);
        RES_TO_STR(VK_ERROR_OUT_OF_DATE_KHR);
        RES_TO_STR(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
        RES_TO_STR(VK_ERROR_VALIDATION_FAILED_EXT);
        RES_TO_STR(VK_ERROR_INVALID_SHADER_NV);
        default:
            return "unknown VkResult error code";
    }
}

}}
