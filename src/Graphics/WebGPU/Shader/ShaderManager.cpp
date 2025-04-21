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

#ifdef SUPPORT_NAGA_CROSS
#include <naga_cross.h>
#endif

#include <Utils/StringUtils.hpp>
#include <Utils/AppSettings.hpp>
#include <Utils/Dialog.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/File/FileUtils.hpp>
#include <Graphics/GLSL/PreprocessorGlsl.hpp>

#include "../Utils/Common.hpp"
#include "Reflect/WGSLReflect.hpp"
#include "Shader.hpp"
#include "ShaderManager.hpp"

#include <webgpu/webgpu.h>

namespace sgl { namespace webgpu {

ShaderManagerWgpu::ShaderManagerWgpu(Device *device) : device(device) {
    preprocessor = new PreprocessorGlsl;
    pathPrefix = sgl::AppSettings::get()->getDataDirectory() + "Shaders/";
    std::map<std::string, std::string>& shaderFileMap = preprocessor->getShaderFileMap();
    indexFiles(shaderFileMap, pathPrefix);
    preprocessor->setUseCppLineStyle(false); //< Not supported by naga_cross at the moment.
}

ShaderManagerWgpu::~ShaderManagerWgpu() {
    delete preprocessor;
}

void ShaderManagerWgpu::indexFiles(std::map<std::string, std::string>& shaderFileMap, const std::string &file) {
    if (FileUtils::get()->isDirectory(file)) {
        // Scan content of directory.
        std::vector<std::string> elements = FileUtils::get()->getFilesInDirectoryVector(file);
        for (std::string &childFile : elements) {
            indexFiles(shaderFileMap, childFile);
        }
    } else {
        auto fileExtension = FileUtils::get()->getFileExtensionLower(file);
        if (fileExtension == "glsl" || fileExtension == "wgsl") {
            // File to index. "fileName" is name without path.
            std::string fileName = FileUtils::get()->getPureFilename(file);
            shaderFileMap.insert(make_pair(fileName, file));
        }
    }
}

const std::map<std::string, std::string>& ShaderManagerWgpu::getShaderFileMap() const {
    return preprocessor->getShaderFileMap();
}

void ShaderManagerWgpu::invalidateShaderCache() {
    assetMap.clear();
    preprocessor->invalidateShaderCache();
}

std::string ShaderManagerWgpu::getShaderFileName(const std::string &pureFilename) {
    return preprocessor->getShaderFileName(pureFilename);
}

ShaderModulePtr ShaderManagerWgpu::getShaderModule(const std::string& shaderId) {
    ShaderModuleInfo info;
    info.filename = shaderId;
    ShaderModulePtr shaderModule = FileManager<ShaderModule, ShaderModuleInfo>::getAsset(info);
    return shaderModule;
}

ShaderModulePtr ShaderManagerWgpu::getShaderModule(
        const std::string& shaderId, const std::map<std::string, std::string>& customPreprocessorDefines) {
    preprocessor->setTempPreprocessorDefines(customPreprocessorDefines);

    ShaderModuleInfo info;
    info.filename = shaderId;
    ShaderModulePtr shaderModule = FileManager<ShaderModule, ShaderModuleInfo>::getAsset(info);

    preprocessor->clearTempPreprocessorDefines();
    return shaderModule;
}

ShaderModulePtr ShaderManagerWgpu::getShaderModule(
        const std::string& shaderId, const std::map<std::string, std::string>& customPreprocessorDefines,
        bool dumpTextDebug) {
    preprocessor->setTempPreprocessorDefines(customPreprocessorDefines);
    preprocessor->setDumpTextDebugStatic(dumpTextDebug);

    ShaderModuleInfo info;
    info.filename = shaderId;
    ShaderModulePtr shaderModule = FileManager<ShaderModule, ShaderModuleInfo>::getAsset(info);

    preprocessor->setDumpTextDebugStatic(false);
    preprocessor->clearTempPreprocessorDefines();
    return shaderModule;
}


ShaderStagesPtr ShaderManagerWgpu::getShaderStagesSingleSource(
        const std::string& shaderId, const std::vector<std::string>& entryPoints) {
    std::vector<ShaderModulePtr> shaderModules;
    shaderModules.reserve(entryPoints.size());
    ShaderModulePtr shaderModule = getShaderModule(shaderId);
    for (size_t i = 0; i < entryPoints.size(); i++) {
        shaderModules.push_back(shaderModule);
    }
    if (!shaderModule) {
        return {};
    }
    return std::make_shared<sgl::webgpu::ShaderStages>(device, shaderModules, entryPoints);
}

ShaderStagesPtr ShaderManagerWgpu::getShaderStagesSingleSource(
        const std::string& shaderId, const std::vector<std::string>& entryPoints,
        const std::map<std::string, std::string>& customPreprocessorDefines) {
    preprocessor->setTempPreprocessorDefines(customPreprocessorDefines);
    auto shaderStages = getShaderStagesSingleSource(shaderId, entryPoints);
    preprocessor->clearTempPreprocessorDefines();
    return shaderStages;
}

ShaderStagesPtr ShaderManagerWgpu::getShaderStagesSingleSource(
        const std::string& shaderId, const std::vector<std::string>& entryPoints,
        const std::map<std::string, std::string>& customPreprocessorDefines, bool dumpTextDebug) {
    preprocessor->setTempPreprocessorDefines(customPreprocessorDefines);
    preprocessor->setDumpTextDebugStatic(dumpTextDebug);
    auto shaderStages = getShaderStagesSingleSource(shaderId, entryPoints);
    preprocessor->setDumpTextDebugStatic(false);
    preprocessor->clearTempPreprocessorDefines();
    return shaderStages;
}


ShaderStagesPtr ShaderManagerWgpu::getShaderStagesMultiSource(
        const std::vector<std::string>& shaderIds) {
    std::vector<std::string> entryPoints;
    entryPoints.reserve(shaderIds.size());
    for (size_t i = 0; i < shaderIds.size(); i++) {
        entryPoints.emplace_back("main");
    }
    return getShaderStagesMultiSource(shaderIds, entryPoints);
}

ShaderStagesPtr ShaderManagerWgpu::getShaderStagesMultiSource(
        const std::vector<std::string>& shaderIds, const std::vector<std::string>& entryPoints) {
    std::vector<ShaderModulePtr> shaderModules;
    for (const std::string &shaderId : shaderIds) {
        ShaderModulePtr shaderModule = getShaderModule(shaderId);
        if (!shaderModule) {
            return {};
        }
        shaderModules.push_back(shaderModule);
    }
    return std::make_shared<sgl::webgpu::ShaderStages>(device, shaderModules, entryPoints);
}

ShaderStagesPtr ShaderManagerWgpu::getShaderStagesMultiSource(
        const std::vector<std::string>& shaderIds,
        const std::map<std::string, std::string>& customPreprocessorDefines) {
    preprocessor->setTempPreprocessorDefines(customPreprocessorDefines);
    auto shaderStages = getShaderStagesMultiSource(shaderIds);
    preprocessor->clearTempPreprocessorDefines();
    return shaderStages;
}

ShaderStagesPtr ShaderManagerWgpu::getShaderStagesMultiSource(
        const std::vector<std::string>& shaderIds, const std::vector<std::string>& entryPoints,
        const std::map<std::string, std::string>& customPreprocessorDefines) {
    preprocessor->setTempPreprocessorDefines(customPreprocessorDefines);
    auto shaderStages = getShaderStagesMultiSource(shaderIds, entryPoints);
    preprocessor->clearTempPreprocessorDefines();
    return shaderStages;
}

ShaderStagesPtr ShaderManagerWgpu::getShaderStagesMultiSource(
        const std::vector<std::string>& shaderIds, const std::vector<std::string>& entryPoints,
        const std::map<std::string, std::string>& customPreprocessorDefines, bool dumpTextDebug) {
    preprocessor->setTempPreprocessorDefines(customPreprocessorDefines);
    preprocessor->setDumpTextDebugStatic(dumpTextDebug);
    auto shaderStages = getShaderStagesMultiSource(shaderIds, entryPoints);
    preprocessor->setDumpTextDebugStatic(false);
    preprocessor->clearTempPreprocessorDefines();
    return shaderStages;
}


ShaderType getShaderTypeFromStringWgsl(const std::string& shaderId) {
    std::string shaderIdLower = sgl::toLowerCopy(shaderId);
    ShaderType shaderModuleType = ShaderType::COMPUTE;
    if (sgl::endsWith(shaderIdLower, "vertex")) {
        shaderModuleType = ShaderType::VERTEX;
    } else if (sgl::endsWith(shaderIdLower, "fragment")) {
        shaderModuleType = ShaderType::FRAGMENT;
    } else if (sgl::endsWith(shaderIdLower, "compute")) {
        shaderModuleType = ShaderType::COMPUTE;
    } else {
        if (sgl::stringContains(shaderIdLower, "vert")) {
            shaderModuleType = ShaderType::VERTEX;
        } else if (sgl::stringContains(shaderIdLower, "frag")) {
            shaderModuleType = ShaderType::FRAGMENT;
        } else if (sgl::stringContains(shaderIdLower, "comp")) {
            shaderModuleType = ShaderType::COMPUTE;
        }
    }
    return shaderModuleType;
}

ShaderModulePtr ShaderManagerWgpu::loadAsset(ShaderModuleInfo& shaderInfo) {
    std::string id = shaderInfo.filename;
    ShaderSource shaderSource;
    std::string shaderString;
    if (sgl::endsWith(shaderInfo.filename, ".wgsl")) {
        shaderSource = ShaderSource::WGSL;
        shaderString = getShaderStringWgsl(id);
    } else {
        shaderSource = ShaderSource::GLSL;
        shaderString = preprocessor->getShaderString(id);
    }

    if (preprocessor->getDumpTextDebugStatic()) {
        std::cout << "Shader dump (" << id << "):" << std::endl;
        std::cout << "--------------------------------------------" << std::endl;
        std::cout << shaderString << std::endl << std::endl;
    }

    // Use naga_cross for GLSL -> WGSL cross-compilation.
    if (shaderSource == ShaderSource::GLSL) {
#ifdef SUPPORT_NAGA_CROSS
        std::vector<NagaCrossShaderDefine> shaderDefines;
        const size_t numDefines =
                preprocessor->getPreprocessorDefines().size()
                + preprocessor->getTempPreprocessorDefines().size();
        shaderDefines.resize(numDefines);
        int defineIdx = 0;
        for (const auto& preprocessorDefine : preprocessor->getPreprocessorDefines()) {
            NagaCrossShaderDefine& shaderDefine = shaderDefines.at(defineIdx);
            shaderDefine.name = preprocessorDefine.first.c_str();
            shaderDefine.value = preprocessorDefine.second.c_str();
            defineIdx++;
        }
        for (const auto& preprocessorDefine : preprocessor->getTempPreprocessorDefines()) {
            NagaCrossShaderDefine& shaderDefine = shaderDefines.at(defineIdx);
            shaderDefine.name = preprocessorDefine.first.c_str();
            shaderDefine.value = preprocessorDefine.second.c_str();
            defineIdx++;
        }

        auto shaderType = getShaderTypeFromStringWgsl(id);
        NagaCrossShaderStage nagaCrossShaderStage;
        if (shaderType == ShaderType::VERTEX) {
            nagaCrossShaderStage = NAGA_CROSS_SHADER_STAGE_VERTEX;
        } else if (shaderType == ShaderType::FRAGMENT) {
            nagaCrossShaderStage = NAGA_CROSS_SHADER_STAGE_FRAGMENT;
        } else {
            nagaCrossShaderStage = NAGA_CROSS_SHADER_STAGE_COMPUTE;
        }

        NagaCrossParams params;
        params.shader_stage = nagaCrossShaderStage;
        params.num_defines = int(shaderDefines.size());
        params.shader_defines = shaderDefines.data();

        NagaCrossResult result;
        naga_cross_glsl_to_wgsl(shaderString.c_str(), &params, &result);
        if (result.succeeded) {
            shaderString = result.wgsl_code;
            if (preprocessor->getDumpTextDebugStatic()) {
                std::cout << "Shader dump (" << id << ") cross-compiled:" << std::endl;
                std::cout << "--------------------------------------------" << std::endl;
                std::cout << shaderString << std::endl << std::endl;
            }
            naga_cross_release_result(&result);
        } else {
            std::string errorMessage = result.error_string;
            naga_cross_release_result(&result);
#ifdef __EMSCRIPTEN__
            auto choice = dialog::openMessageBoxBlocking(
                "Error occurred", errorMessage, dialog::Choice::OK, dialog::Icon::ERROR);
#else
            auto choice = dialog::openMessageBoxBlocking(
                    "Error occurred", errorMessage, dialog::Choice::ABORT_RETRY_IGNORE, dialog::Icon::ERROR);
            if (choice == dialog::Button::RETRY) {
                naga_cross_release_result(&result);
                sgl::webgpu::ShaderManager->invalidateShaderCache();
                return loadAsset(shaderInfo);
            } else if (choice == dialog::Button::ABORT) {
                exit(1);
            }
#endif
        }
#else
        sgl::Logfile::get()->throwError(
                "Error in ShaderManagerWgpu::loadAsset: Attempting to load GLSL shader, but naga_cross support is not enabled.");
#endif
    }

    WGPUShaderModuleDescriptor shaderModuleDescriptor{};
#ifdef WEBGPU_LEGACY_API
    WGPUShaderModuleWGSLDescriptor shaderSourceWgsl{};
    // TODO: Is WGPUSType_ShaderModuleSPIRVDescriptor only available in WGPU?
    shaderSourceWgsl.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
#else
    WGPUShaderSourceWGSL shaderSourceWgsl{};
    // TODO: Is WGPUSType_ShaderSourceSPIRV only available in WGPU?
    shaderSourceWgsl.chain.sType = WGPUSType_ShaderSourceWGSL;
#endif
    stdStringToWgpuView(shaderSourceWgsl.code, shaderString);
    shaderModuleDescriptor.nextInChain = &shaderSourceWgsl.chain;
    stdStringToWgpuView(shaderModuleDescriptor.label, id);
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

    ReflectInfo reflectInfo{};
    std::string reflectErrorString;
    if (!wgslCodeReflect(shaderString, reflectInfo, reflectErrorString)) {
        sgl::Logfile::get()->writeErrorMultiline("Error in wgslCodeReflect: " + reflectErrorString);
    }

    return std::make_shared<ShaderModule>(shaderModuleWgpu, reflectInfo);
}

std::string ShaderManagerWgpu::getShaderStringWgsl(const std::string &globalShaderName) {
    auto& effectSources = preprocessor->getEffectSources();
    auto it = effectSources.find(globalShaderName);
    if (it != effectSources.end()) {
        return it->second;
    }

    std::string::size_type filenameEnd = globalShaderName.find('.');
    std::string pureFilename = globalShaderName.substr(0, filenameEnd);
    std::string shaderFilename = getShaderFileName(globalShaderName);
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
        effectSources.insert(make_pair(globalShaderName, shaderContent));
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
