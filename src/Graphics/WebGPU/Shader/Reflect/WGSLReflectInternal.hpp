/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2024, Christoph Neuhauser
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

#ifndef SGL_WGSLREFLECTINTERNAL_HPP
#define SGL_WGSLREFLECTINTERNAL_HPP

#include <vector>
#include <string>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

namespace sgl { namespace webgpu {

// See: https://www.w3.org/TR/WGSL/#types, e.g.: ptr<function,vec3<f32>>
struct wgsl_type {
    std::string name;
    boost::optional<std::vector<wgsl_type>> template_parameters;
};

// E.g.: @builtin(position)
struct wgsl_attribute {
    std::string name;
    std::string expression;
};

// E.g.: @builtin(position) position: vec4f
struct wgsl_struct_entry {
    std::vector<wgsl_attribute> attributes;
    std::string name;
    wgsl_type type;
};

/*
 * E.g.:
 * struct VertexOutput {
 * 	   @builtin(position) position: vec4f,
 * 	   @location(0) color: vec3f,
 * };
 */
struct wgsl_struct {
    std::string name;
    std::vector<wgsl_struct_entry> entries;
};

// E.g.: @group(0) @binding(0) var<storage,read> inputBuffer: array<f32,64>;
struct wgsl_variable {
    std::vector<wgsl_attribute> attributes;
    boost::optional<std::vector<std::string>> modifiers;
    std::string name;
    wgsl_type type;
};

// E.g.: const PI: f32 = 3.141;
struct wgsl_constant {
    std::string name;
    wgsl_type type;
    std::string value;
};

// E.g.: @fragment fn fs_main(vertex_in: VertexOut) -> FragmentOut { ... }
struct wgsl_function {
    std::vector<wgsl_attribute> attributes;
    std::string name;
    std::vector<wgsl_struct_entry> parameters;
    std::vector<wgsl_attribute> return_type_attributes;
    boost::optional<wgsl_type> return_type;
    std::string function_content;
};

struct wgsl_directive {
    std::string directive_type; // enable, requires, diagnostic
    std::vector<std::string> values;
};

typedef boost::variant<wgsl_struct, wgsl_variable, wgsl_constant, wgsl_function, wgsl_directive> wgsl_entry;

typedef std::vector<wgsl_entry> wgsl_content;

/**
 * Retrieves information about the line containing the char with the passed index.
 */
void getLineInfo(
        const std::string& fileContent, size_t charIdx, std::string& lineStr, size_t& lineIdx, size_t& charIdxInLine);

/**
 * Parses the content of a .wgsl file with Spirit Qi (a legacy, but stable C++03 Boost library).
 * @param fileContent The content of the file.
 * @param content The parsed AST content of the file.
 * @param errorString The error string (if false is returned).
 * @return Whether parsing succeeded.
 */
bool wgslReflectParseQi(const std::string& fileContent, wgsl_content& content, std::string& errorString);

/**
 * Parses the content of a .wgsl file with Spirit X3 (a faster, but not yet stable C++17 Boost library).
 * @param fileContent The content of the file.
 * @param content The parsed AST content of the file.
 * @param errorString The error string (if false is returned).
 * @return Whether parsing succeeded.
 */
bool wgslReflectParseX3(const std::string& fileContent, wgsl_content& content, std::string& errorString);

}}

#endif //SGL_WGSLREFLECTINTERNAL_HPP
