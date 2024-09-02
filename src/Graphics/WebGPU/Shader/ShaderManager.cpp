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

#include <iostream>

#include <Utils/StringUtils.hpp>
#include <Utils/AppSettings.hpp>
#include <Utils/Dialog.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/File/FileUtils.hpp>

#include "Reflect/WGSLReflect.hpp"
#include "ShaderModule.hpp"
#include "ShaderManager.hpp"

#include <webgpu/webgpu.h>

namespace sgl { namespace webgpu {

ShaderManagerWgpu::ShaderManagerWgpu(Device *device) : device(device) {
    pathPrefix = sgl::AppSettings::get()->getDataDirectory() + "Shaders/";
    indexFiles(pathPrefix);
}

void ShaderManagerWgpu::indexFiles(const std::string &file) {
    if (FileUtils::get()->isDirectory(file)) {
        // Scan content of directory.
        std::vector<std::string> elements = FileUtils::get()->getFilesInDirectoryVector(file);
        for (std::string &childFile : elements) {
            indexFiles(childFile);
        }
    } else if (FileUtils::get()->hasExtension(file.c_str(), ".wgsl")) {
        // File to index. "fileName" is name without path.
        std::string fileName = FileUtils::get()->getPureFilename(file);
        shaderFileMap.insert(make_pair(fileName, file));
    }
}

std::string ShaderManagerWgpu::getShaderFileName(const std::string &pureFilename) {
    auto it = shaderFileMap.find(pureFilename);
    if (it == shaderFileMap.end()) {
        sgl::Logfile::get()->writeError(
                "Error in ShaderManagerWgpu::getShaderFileName: Unknown file name \"" + pureFilename + "\".");
        return "";
    }
    return it->second;
}

std::string ShaderManagerWgpu::getPreprocessorDefines() {
    std::string preprocessorStatements;
    for (auto it = preprocessorDefines.begin(); it != preprocessorDefines.end(); it++) {
        preprocessorStatements += std::string() + "#define " + it->first + " " + it->second + "\n";
    }
    return preprocessorStatements;
}

void ShaderManagerWgpu::invalidateShaderCache() {
    assetMap.clear();
    effectSources.clear();
}

ShaderModulePtr ShaderManagerWgpu::getShaderModule(
        const std::string& shaderId, const std::map<std::string, std::string>& customPreprocessorDefines,
        bool dumpTextDebug) {
    preprocessorDefines = customPreprocessorDefines;
    dumpTextDebugStatic = dumpTextDebug;

    ShaderModuleInfo info;
    info.filename = shaderId;
    ShaderModulePtr shaderModule = FileManager<ShaderModule, ShaderModuleInfo>::getAsset(info);

    dumpTextDebugStatic = false;
    preprocessorDefines.clear();
    return shaderModule;
}

ShaderModulePtr ShaderManagerWgpu::loadAsset(ShaderModuleInfo& shaderInfo) {
    std::string id = shaderInfo.filename;
    std::string shaderString = getShaderString(id);

    if (dumpTextDebugStatic) {
        std::cout << "Shader dump (" << id << "):" << std::endl;
        std::cout << "--------------------------------------------" << std::endl;
        std::cout << shaderString << std::endl << std::endl;
    }

    WGPUShaderModuleDescriptor shaderModuleDescriptor{};
    WGPUShaderModuleWGSLDescriptor shaderModuleWgslDescriptor{};
    // TODO: Is WGPUSType_ShaderModuleSPIRVDescriptor only available in WGPU?
    shaderModuleWgslDescriptor.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    shaderModuleWgslDescriptor.code = shaderString.c_str();
    shaderModuleDescriptor.nextInChain = &shaderModuleWgslDescriptor.chain;
    shaderModuleDescriptor.label = id.c_str();
    errorMessageExternal.clear();
    WGPUShaderModule shaderModuleWgpu = wgpuDeviceCreateShaderModule(device->getWGPUDevice(), &shaderModuleDescriptor);
    if (!errorMessageExternal.empty()) {
        std::string errorMessage = "Shader compilation error: " + errorMessageExternal;
        if (shaderModuleWgpu) {
            wgpuShaderModuleRelease(shaderModuleWgpu);
            shaderModuleWgpu = {};
        }
        sgl::Logfile::get()->writeErrorMultiline(errorMessage, false);
#ifdef __EMSCRIPTEN__
        auto choice = dialog::openMessageBoxBlocking(
                "Error occurred", errorMessage, dialog::Choice::OK, dialog::Icon::ERROR);
#else
        std::string searchString = "wgpuDeviceCreateShaderModule";
        auto startCmd = errorMessage.find(searchString);
        if (startCmd != std::string::npos) {
            errorMessage = errorMessage.substr(startCmd + searchString.length());
            while (!errorMessage.empty() && (errorMessage[0] == ' ' || errorMessage[0] == '\n' || errorMessage[0] == '\r')) {
                errorMessage = errorMessage.substr(1);
            }
            while (!errorMessage.empty() && (errorMessage.back() == ' ' || errorMessage.back() == '\n' || errorMessage.back() == '\r')) {
                errorMessage = errorMessage.substr(0, errorMessage.size() - 1);
            }
        }

        // Find and replace "box drawings light down and right": https://www.compart.com/en/unicode/U+250C
        // UTF-8: 0xE2 0x94 0x8C, UTF-16: 0x250C.
        // This UTF-8 char otherwise leads to issues with the dialog system.
        //searchString = "\xE2\x94\x8C";
        // ICU does not replace the char...
        //sgl::stringReplaceAll(errorMessage, searchString, "|");
        //while (true) {
        //    auto start = errorMessage.find(searchString);
        //    if (start == std::string::npos) {
        //        break;
        //    }
        //    errorMessage = errorMessage.replace(start, searchString.length(), "|");
        //}

        auto choice = dialog::openMessageBoxBlocking(
                "Error occurred", errorMessage, dialog::Choice::ABORT_RETRY_IGNORE, dialog::Icon::ERROR);
        if (choice == dialog::Button::RETRY) {
            sgl::webgpu::ShaderManager->invalidateShaderCache();
            return loadAsset(shaderInfo);
        } else if (choice == dialog::Button::ABORT) {
            exit(1);
        }
#endif
        return {};
    }
    if (!shaderModuleWgpu) {
        sgl::Logfile::get()->throwError(
                "Fatal error in ShaderManagerWgpu::loadAsset: Compilation failed without error message.");
        return {};
    }

    // TODO: Pass this information to the shader.
    ReflectInfo reflectInfo{};
    std::string reflectErrorString;
    if (!wgslCodeReflect(shaderString, reflectInfo, reflectErrorString)) {
        sgl::Logfile::get()->writeErrorMultiline("Error in wgslCodeReflect: " + reflectErrorString);
    }

    return std::make_shared<ShaderModule>(shaderModuleWgpu);
}

std::string ShaderManagerWgpu::getShaderString(const std::string &globalShaderName) {
    auto it = effectSources.find(globalShaderName);
    if (it != effectSources.end()) {
        return it->second;
    }

    std::string::size_type filenameEnd = globalShaderName.find('.');
    std::string pureFilename = globalShaderName.substr(0, filenameEnd);
    std::string shaderFilename = getShaderFileName(pureFilename + ".wgsl");
    std::string shaderInternalId = globalShaderName.substr(filenameEnd + 1);

    std::ifstream file(shaderFilename.c_str());
    if (!file.is_open()) {
        Logfile::get()->throwError(
                std::string() + "Error in getShader: Couldn't open the file \"" + shaderFilename + "\".");
    }

    std::string shaderContent;
    std::string shaderName;
    std::string prependContent;

    int lineNum = 1;
    std::string linestr, trimmedLinestr;
    while (getline(file, linestr)) {
        // Remove \r if line ending is \r\n
        if (!linestr.empty() && linestr.at(linestr.size()-1) == '\r') {
            linestr = linestr.substr(0, linestr.size() - 1);
        }

        //trimmedLinestr = sgl::stringTrimCopy(linestr);
        lineNum++;
        shaderContent += std::string() + linestr + "\n";
    }
    shaderContent = prependContent + shaderContent;
    file.close();

    if (!shaderName.empty()) {
        effectSources.insert(make_pair(shaderName, shaderContent));
    } else {
        effectSources.insert(make_pair(pureFilename + ".wgsl", shaderContent));
    }

    it = effectSources.find(globalShaderName);
    if (it != effectSources.end()) {
        return it->second;
    }

    Logfile::get()->writeError(
            std::string() + "Error in getShader: Couldn't find the shader \"" + globalShaderName + "\".");
    return "";
}

}}
