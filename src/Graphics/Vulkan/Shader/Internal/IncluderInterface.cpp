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

#include <cstring>
#include "../ShaderManager.hpp"
#include "IncluderInterface.hpp"

namespace sgl { namespace vk {

/*std::string loadFileContent(const std::string &filename) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        std::cerr << "ERROR in loadFileContent: Couldn't open the file \"" << filename + "\"." << std::endl;
    }

    std::string fileContent;
    std::string lineString;
    while (getline(file, lineString)) {
        fileContent += lineString + "\n";
    }
    file.close();

    return fileContent;
}*/

std::string IncluderInterface::getDirectoryFromFilename(const std::string &filename) {
    return filename.substr(0, filename.find_last_of('/') + 1);
}

shaderc_include_result *IncluderInterface::GetInclude(
        const char *requestedSource, shaderc_include_type type,
        const char *requestingSource, size_t includeDepth) {
    shaderc_include_result *includeResult = new shaderc_include_result();

    std::string headerName = requestedSource;
    std::string headerFilename;
    if (type == shaderc_include_type_relative) {
        // E.g. #include "source"
        headerFilename = shaderManager->getShaderPathPrefix() + getDirectoryFromFilename(headerFilename) + headerName;
    } else if (type == shaderc_include_type_standard) {
        // E.g. #include <source>
        headerFilename = shaderManager->getShaderPathPrefix() + headerName;
    } else {
        throw std::runtime_error("Error in IncluderInterface::GetInclude: Unknown shaderc include type.");
    }

    std::string headerContent = loadFileContent(headerFilename); // TODO

    char *sourceName = new char[headerName.length()];
    char *content = new char[headerContent.length()];
    memcpy(sourceName, headerName.c_str(), headerName.length());
    memcpy(content, headerContent.c_str(), headerContent.length());
    includeResult->source_name = sourceName;
    includeResult->source_name_length = headerName.length();
    includeResult->content = content;
    includeResult->content_length = headerContent.length();
    return includeResult;
}

void IncluderInterface::ReleaseInclude(shaderc_include_result *data) {
    delete[] data->source_name;
    delete[] data->content;
    delete data;
}

}}
