/*
 * This file contains modified code from the SPIR-V Reflect examples
 * (https://github.com/KhronosGroup/SPIRV-Reflect). The original code is published
 * under the Apache License, Version 2.0, and the changes under the BSD 2-Clause
 * License.
 *
 * ---------------------------------------------------------------------------
 *
 * Copyright 2017-2018 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ---------------------------------------------------------------------------
 *
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

#include "ReflectHelpers.hpp"

std::string spirvFormatToString(SpvReflectFormat format) {
    if (format == SPV_REFLECT_FORMAT_R32_UINT) {
        return "uint";
    } else if (format == SPV_REFLECT_FORMAT_R32_SINT) {
        return "int";
    } else if (format == SPV_REFLECT_FORMAT_R32_SFLOAT) {
        return "uint";
    } else if (format >= SPV_REFLECT_FORMAT_R32G32_UINT && format <= SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT) {
        int val = format - SPV_REFLECT_FORMAT_R32G32_UINT;
        std::string formatString;
        switch (val % 3) {
            case 0:
                formatString = "u";
                break;
            case 1:
                formatString = "i";
                break;
        }
        formatString += "vec";
        formatString += std::to_string(val / 3 + 2);
        return formatString;
    } else {
        return "UNKNOWN";
    }
}

std::string toStringDescriptorType(SpvReflectDescriptorType value) {
    switch (value) {
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER                    : return "VK_DESCRIPTOR_TYPE_SAMPLER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER     : return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE              : return "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE";
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE              : return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER       : return "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER       : return "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER             : return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER             : return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC     : return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC     : return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC";
        case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT           : return "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
        case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR : return "VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR";
    }
    // unhandled SpvReflectDescriptorType enum value
    return "VK_DESCRIPTOR_TYPE_???";
}

void printModuleInfo(std::ostream &os, const SpvReflectShaderModule &obj, const char * /*indent*/) {
    os << "entry point     : " << obj.entry_point_name << "\n";
    os << "source lang     : " << spvReflectSourceLanguage(obj.source_language) << "\n";
    os << "source lang ver : " << obj.source_language_version << "\n";
    if (obj.source_language == SpvSourceLanguageHLSL) {
        os << "stage           : ";
        switch (obj.shader_stage) {
            default:
                break;
            case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT                   :
                os << "VS";
                break;
            case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT     :
                os << "HS";
                break;
            case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT  :
                os << "DS";
                break;
            case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT                 :
                os << "GS";
                break;
            case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT                 :
                os << "PS";
                break;
            case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT                  :
                os << "CS";
                break;
        }
    }
}

void printDescriptorSet(std::ostream &os, const SpvReflectDescriptorSet &obj, const char *indent) {
    const char *t = indent;
    std::string tt = std::string(indent) + "  ";
    std::string ttttt = std::string(indent) + "    ";

    os << t << "set           : " << obj.set << "\n";
    os << t << "binding count : " << obj.binding_count;
    os << "\n";
    for (uint32_t i = 0; i < obj.binding_count; ++i) {
        const SpvReflectDescriptorBinding &binding = *obj.bindings[i];
        os << tt << i << ":" << "\n";
        printDescriptorBinding(os, binding, false, ttttt.c_str());
        if (i < (obj.binding_count - 1)) {
            os << "\n";
        }
    }
}

void printDescriptorBinding(
        std::ostream &os, const SpvReflectDescriptorBinding &obj, bool write_set, const char *indent) {
    const char *t = indent;
    os << t << "binding : " << obj.binding << "\n";
    if (write_set) {
        os << t << "set     : " << obj.set << "\n";
    }
    os << t << "type    : " << toStringDescriptorType(obj.descriptor_type) << "\n";

    // array
    if (obj.array.dims_count > 0) {
        os << t << "array   : ";
        for (uint32_t dim_index = 0; dim_index < obj.array.dims_count; ++dim_index) {
            os << "[" << obj.array.dims[dim_index] << "]";
        }
        os << "\n";
    }

    // counter
    if (obj.uav_counter_binding != nullptr) {
        os << t << "counter : ";
        os << "(";
        os << "set=" << obj.uav_counter_binding->set << ", ";
        os << "binding=" << obj.uav_counter_binding->binding << ", ";
        os << "name=" << obj.uav_counter_binding->name;
        os << ");";
        os << "\n";
    }

    os << t << "name    : " << obj.name;
    if ((obj.type_description->type_name != nullptr) && (strlen(obj.type_description->type_name) > 0)) {
        os << " " << "(" << obj.type_description->type_name << ")";
    }
}
