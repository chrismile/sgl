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

#include <iostream>
#include <unordered_map>

#include "../libs/volk/volk.h"

#include <Utils/Convert.hpp>
#include <Utils/StringUtils.hpp>
#include <Utils/AppSettings.hpp>
#include <Utils/Dialog.hpp>
#include <Utils/File/Logfile.hpp>
#include <Utils/File/FileUtils.hpp>

#include <Graphics/Vulkan/Utils/Instance.hpp>
#include "ShaderManager.hpp"

#ifdef SUPPORT_SHADERC_BACKEND
#include "Internal/IncluderInterface.hpp"
#endif

#ifdef SUPPORT_GLSLANG_BACKEND
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

static void initializeBuiltInResourceGlslang(TBuiltInResource &defaultTBuiltInResource) {
    defaultTBuiltInResource = {};
    defaultTBuiltInResource.maxLights = 32;
    defaultTBuiltInResource.maxClipPlanes = 6;
    defaultTBuiltInResource.maxTextureUnits = 32;
    defaultTBuiltInResource.maxTextureCoords = 32;
    defaultTBuiltInResource.maxVertexAttribs = 64;
    defaultTBuiltInResource.maxVertexUniformComponents = 4096;
    defaultTBuiltInResource.maxVaryingFloats = 64;
    defaultTBuiltInResource.maxVertexTextureImageUnits = 32;
    defaultTBuiltInResource.maxCombinedTextureImageUnits = 80;
    defaultTBuiltInResource.maxTextureImageUnits = 32;
    defaultTBuiltInResource.maxFragmentUniformComponents = 4096;
    defaultTBuiltInResource.maxDrawBuffers = 32;
    defaultTBuiltInResource.maxVertexUniformVectors = 128;
    defaultTBuiltInResource.maxVaryingVectors = 8;
    defaultTBuiltInResource.maxFragmentUniformVectors = 16;
    defaultTBuiltInResource.maxVertexOutputVectors = 16;
    defaultTBuiltInResource.maxFragmentInputVectors = 15;
    defaultTBuiltInResource.minProgramTexelOffset = -8;
    defaultTBuiltInResource.maxProgramTexelOffset = 7;
    defaultTBuiltInResource.maxClipDistances = 8;
    defaultTBuiltInResource.maxComputeWorkGroupCountX = 65535;
    defaultTBuiltInResource.maxComputeWorkGroupCountY = 65535;
    defaultTBuiltInResource.maxComputeWorkGroupCountZ = 65535;
    defaultTBuiltInResource.maxComputeWorkGroupSizeX = 1024;
    defaultTBuiltInResource.maxComputeWorkGroupSizeY = 1024;
    defaultTBuiltInResource.maxComputeWorkGroupSizeZ = 64;
    defaultTBuiltInResource.maxComputeUniformComponents = 1024;
    defaultTBuiltInResource.maxComputeTextureImageUnits = 16;
    defaultTBuiltInResource.maxComputeImageUniforms = 8;
    defaultTBuiltInResource.maxComputeAtomicCounters = 8;
    defaultTBuiltInResource.maxComputeAtomicCounterBuffers = 1;
    defaultTBuiltInResource.maxVaryingComponents = 60;
    defaultTBuiltInResource.maxVertexOutputComponents = 64;
    defaultTBuiltInResource.maxGeometryInputComponents = 64;
    defaultTBuiltInResource.maxGeometryOutputComponents = 128;
    defaultTBuiltInResource.maxFragmentInputComponents = 128;
    defaultTBuiltInResource.maxImageUnits = 8;
    defaultTBuiltInResource.maxCombinedImageUnitsAndFragmentOutputs = 8;
    defaultTBuiltInResource.maxCombinedShaderOutputResources = 8;
    defaultTBuiltInResource.maxImageSamples = 0;
    defaultTBuiltInResource.maxVertexImageUniforms = 0;
    defaultTBuiltInResource.maxTessControlImageUniforms = 0;
    defaultTBuiltInResource.maxTessEvaluationImageUniforms = 0;
    defaultTBuiltInResource.maxGeometryImageUniforms = 0;
    defaultTBuiltInResource.maxFragmentImageUniforms = 8;
    defaultTBuiltInResource.maxCombinedImageUniforms = 8;
    defaultTBuiltInResource.maxGeometryTextureImageUnits = 16;
    defaultTBuiltInResource.maxGeometryOutputVertices = 256;
    defaultTBuiltInResource.maxGeometryTotalOutputComponents = 1024;
    defaultTBuiltInResource.maxGeometryUniformComponents = 1024;
    defaultTBuiltInResource.maxGeometryVaryingComponents = 64;
    defaultTBuiltInResource.maxTessControlInputComponents = 128;
    defaultTBuiltInResource.maxTessControlOutputComponents = 128;
    defaultTBuiltInResource.maxTessControlTextureImageUnits = 16;
    defaultTBuiltInResource.maxTessControlUniformComponents = 1024;
    defaultTBuiltInResource.maxTessControlTotalOutputComponents = 4096;
    defaultTBuiltInResource.maxTessEvaluationInputComponents = 128;
    defaultTBuiltInResource.maxTessEvaluationOutputComponents = 128;
    defaultTBuiltInResource.maxTessEvaluationTextureImageUnits = 16;
    defaultTBuiltInResource.maxTessEvaluationUniformComponents = 1024;
    defaultTBuiltInResource.maxTessPatchComponents = 120;
    defaultTBuiltInResource.maxPatchVertices = 32;
    defaultTBuiltInResource.maxTessGenLevel = 64;
    defaultTBuiltInResource.maxViewports = 16;
    defaultTBuiltInResource.maxVertexAtomicCounters = 0;
    defaultTBuiltInResource.maxTessControlAtomicCounters = 0;
    defaultTBuiltInResource.maxTessEvaluationAtomicCounters = 0;
    defaultTBuiltInResource.maxGeometryAtomicCounters = 0;
    defaultTBuiltInResource.maxFragmentAtomicCounters = 8;
    defaultTBuiltInResource.maxCombinedAtomicCounters = 8;
    defaultTBuiltInResource.maxAtomicCounterBindings = 1;
    defaultTBuiltInResource.maxVertexAtomicCounterBuffers = 0;
    defaultTBuiltInResource.maxTessControlAtomicCounterBuffers = 0;
    defaultTBuiltInResource.maxTessEvaluationAtomicCounterBuffers = 0;
    defaultTBuiltInResource.maxGeometryAtomicCounterBuffers = 0;
    defaultTBuiltInResource.maxFragmentAtomicCounterBuffers = 1;
    defaultTBuiltInResource.maxCombinedAtomicCounterBuffers = 1;
    defaultTBuiltInResource.maxAtomicCounterBufferSize = 16384;
    defaultTBuiltInResource.maxTransformFeedbackBuffers = 4;
    defaultTBuiltInResource.maxTransformFeedbackInterleavedComponents = 64;
    defaultTBuiltInResource.maxCullDistances = 8;
    defaultTBuiltInResource.maxCombinedClipAndCullDistances = 8;
    defaultTBuiltInResource.maxSamples = 4;
    defaultTBuiltInResource.maxMeshOutputVerticesNV = 256;
    defaultTBuiltInResource.maxMeshOutputPrimitivesNV = 512;
    defaultTBuiltInResource.maxMeshWorkGroupSizeX_NV = 32;
    defaultTBuiltInResource.maxMeshWorkGroupSizeY_NV = 1;
    defaultTBuiltInResource.maxMeshWorkGroupSizeZ_NV = 1;
    defaultTBuiltInResource.maxTaskWorkGroupSizeX_NV = 32;
    defaultTBuiltInResource.maxTaskWorkGroupSizeY_NV = 1;
    defaultTBuiltInResource.maxTaskWorkGroupSizeZ_NV = 1;
    defaultTBuiltInResource.maxMeshViewCountNV = 4;
#if defined(VK_EXT_mesh_shader) && defined(GLSLANG_MESH_SHADER_EXT_SUPPORT)
    defaultTBuiltInResource.maxMeshOutputVerticesEXT = 256;
    defaultTBuiltInResource.maxMeshOutputPrimitivesEXT = 256;
    defaultTBuiltInResource.maxMeshWorkGroupSizeX_EXT = 128;
    defaultTBuiltInResource.maxMeshWorkGroupSizeY_EXT = 128;
    defaultTBuiltInResource.maxMeshWorkGroupSizeZ_EXT = 128;
    defaultTBuiltInResource.maxTaskWorkGroupSizeX_EXT = 128;
    defaultTBuiltInResource.maxTaskWorkGroupSizeY_EXT = 128;
    defaultTBuiltInResource.maxTaskWorkGroupSizeZ_EXT = 128;
    defaultTBuiltInResource.maxMeshViewCountEXT = 4;
#endif
    defaultTBuiltInResource.maxDualSourceDrawBuffersEXT = 1;

    defaultTBuiltInResource.limits.nonInductiveForLoops = true;
    defaultTBuiltInResource.limits.whileLoops = true;
    defaultTBuiltInResource.limits.doWhileLoops = true;
    defaultTBuiltInResource.limits.generalUniformIndexing = true;
    defaultTBuiltInResource.limits.generalAttributeMatrixVectorIndexing = true;
    defaultTBuiltInResource.limits.generalVaryingIndexing = true;
    defaultTBuiltInResource.limits.generalSamplerIndexing = true;
    defaultTBuiltInResource.limits.generalVariableIndexing = true;
    defaultTBuiltInResource.limits.generalConstantMatrixVectorIndexing = true;
}
#endif

namespace sgl { namespace vk {

ShaderManagerVk::ShaderManagerVk(Device *device) : device(device) {
#ifdef SUPPORT_GLSLANG_BACKEND
    if (!glslang::InitializeProcess()) {
        Logfile::get()->throwError(
                "Fatal error in ShaderManagerVk::ShaderManagerVk: glslang::InitializeProcess failed!");
    }
#endif
#ifdef SUPPORT_SHADERC_BACKEND
    shaderCompiler = new shaderc::Compiler;
#endif
    pathPrefix = sgl::AppSettings::get()->getDataDirectory() + "Shaders/";
    indexFiles(pathPrefix);

    // Was a file called "GlobalDefinesVulkan.glsl" found? If yes, store its content in the variable globalDefines.
    auto it = shaderFileMap.find("GlobalDefinesVulkan.glsl");
    if (it != shaderFileMap.end()) {
        std::ifstream file(it->second);
        if (!file.is_open()) {
            Logfile::get()->writeError(
                    "ShaderManagerVk::ShaderManagerVk: Unexpected error occured while loading "
                    "\"GlobalDefinesVulkan.glsl\".");
        }
        globalDefines = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }
    globalDefinesMvpMatrices =
            "#ifndef SGL_MATRIX_BLOCK\n"
            "#define SGL_MATRIX_BLOCK\n"
            "layout (set = 1, binding = 0) uniform MatrixBlock {\n"
            "    mat4 mMatrix; // Model matrix\n"
            "    mat4 vMatrix; // View matrix\n"
            "    mat4 pMatrix; // Projection matrix\n"
            "    mat4 mvpMatrix; // Model-view-projection matrix\n"
            "};\n"
            "#endif\n\n";
}

ShaderManagerVk::~ShaderManagerVk() {
#ifdef SUPPORT_SHADERC_BACKEND
    if (shaderCompiler) {
        delete shaderCompiler;
        shaderCompiler = nullptr;
    }
#endif
#ifdef SUPPORT_GLSLANG_BACKEND
    glslang::FinalizeProcess();
#endif
}

void ShaderManagerVk::setShaderCompilerBackend(ShaderCompilerBackend backend) {
#ifndef SUPPORT_SHADERC_BACKEND
    if (backend == ShaderCompilerBackend::SHADERC) {
        sgl::Logfile::get()->writeWarning(
                "Warning in ShaderManagerVk::setShaderCompilerBackend: shaderc backend is not available.");
        return;
    }
#endif
#ifndef SUPPORT_GLSLANG_BACKEND
    if (backend == ShaderCompilerBackend::GLSLANG) {
        sgl::Logfile::get()->writeWarning(
                "Warning in ShaderManagerVk::setShaderCompilerBackend: glslang backend is not available.");
        return;
    }
#endif
    shaderCompilerBackend = backend;
}


ShaderStagesPtr ShaderManagerVk::getShaderStages(const std::vector<std::string> &shaderIds, bool dumpTextDebug) {
    return createShaderStages(shaderIds, dumpTextDebug);
}

ShaderStagesPtr ShaderManagerVk::getShaderStages(
        const std::vector<std::string> &shaderIds, uint32_t subgroupSize, bool dumpTextDebug) {
    std::vector<ShaderStageSettings> settings;
    settings.resize(shaderIds.size());
    for (size_t i = 0; i < settings.size(); i++) {
        settings.at(i).requiredSubgroupSize = subgroupSize;
    }
    return createShaderStages(shaderIds, settings, dumpTextDebug);
}

ShaderStagesPtr ShaderManagerVk::getShaderStages(
        const std::vector<std::string> &shaderIds, const std::map<std::string, std::string>& customPreprocessorDefines,
        bool dumpTextDebug) {
    tempPreprocessorDefines = customPreprocessorDefines;
    ShaderStagesPtr shaderStages = createShaderStages(shaderIds, dumpTextDebug);
    tempPreprocessorDefines.clear();
    return shaderStages;
}

ShaderStagesPtr ShaderManagerVk::getShaderStages(
        const std::vector<std::string> &shaderIds, const std::map<std::string, std::string>& customPreprocessorDefines,
        uint32_t subgroupSize, bool dumpTextDebug) {
    std::vector<ShaderStageSettings> settings;
    settings.resize(shaderIds.size());
    for (size_t i = 0; i < settings.size(); i++) {
        settings.at(i).requiredSubgroupSize = subgroupSize;
    }
    return getShaderStagesWithSettings(shaderIds, customPreprocessorDefines, settings, dumpTextDebug);
}

ShaderStagesPtr ShaderManagerVk::getShaderStagesWithSettings(
        const std::vector<std::string>& shaderIds,
        const std::map<std::string, std::string>& customPreprocessorDefines,
        const std::vector<ShaderStageSettings>& settings, bool dumpTextDebug) {
    tempPreprocessorDefines = customPreprocessorDefines;
    ShaderStagesPtr shaderStages = createShaderStages(shaderIds, settings, dumpTextDebug);
    tempPreprocessorDefines.clear();
    return shaderStages;
}

ShaderModulePtr ShaderManagerVk::getShaderModule(const std::string& shaderId, ShaderModuleType shaderModuleType) {
    ShaderModuleInfo info;
    info.filename = shaderId;
    info.shaderModuleType = shaderModuleType;
    return FileManager<ShaderModule, ShaderModuleInfo>::getAsset(info);
}

ShaderModulePtr ShaderManagerVk::getShaderModule(
        const std::string& shaderId, ShaderModuleType shaderModuleType,
        const std::map<std::string, std::string>& customPreprocessorDefines) {
    tempPreprocessorDefines = customPreprocessorDefines;

    ShaderModuleInfo info;
    info.filename = shaderId;
    info.shaderModuleType = shaderModuleType;
    ShaderModulePtr shaderModule = FileManager<ShaderModule, ShaderModuleInfo>::getAsset(info);

    tempPreprocessorDefines.clear();
    return shaderModule;
}

ShaderStagesPtr ShaderManagerVk::compileComputeShaderFromStringCached(
        const std::string& shaderId, const std::string& shaderString) {
    auto it = cachedShadersLoadedFromDirectString.find(shaderId);
    if (it != cachedShadersLoadedFromDirectString.end()) {
        return it->second;
    }

    ShaderModulePtr shaderModule;
    ShaderModuleInfo shaderInfo{};
    shaderInfo.shaderModuleType = ShaderModuleType::COMPUTE;
    shaderInfo.filename = shaderId;
#ifdef SUPPORT_SHADERC_BACKEND
    if (shaderCompilerBackend == ShaderCompilerBackend::SHADERC) {
        shaderModule = loadAssetShaderc(shaderInfo, shaderId, shaderString);
    }
#endif
#ifdef SUPPORT_GLSLANG_BACKEND
    if (shaderCompilerBackend == ShaderCompilerBackend::GLSLANG) {
        shaderModule = loadAssetGlslang(shaderInfo, shaderId, shaderString);
    }
#endif
    if (!shaderModule) {
        return ShaderStagesPtr();
    }

    std::vector<ShaderModulePtr> shaderModules;
    shaderModules.push_back(shaderModule);
    ShaderStagesPtr shaderProgram(new ShaderStages(device, shaderModules));
    cachedShadersLoadedFromDirectString.insert(std::make_pair(shaderId, shaderProgram));
    return shaderProgram;
}


ShaderModuleType getShaderModuleTypeFromString(const std::string& shaderId) {
    std::string shaderIdLower = sgl::toLowerCopy(shaderId);
    ShaderModuleType shaderModuleType = ShaderModuleType::UNKNOWN;
    if (sgl::endsWith(shaderIdLower, "vertex")) {
        shaderModuleType = ShaderModuleType::VERTEX;
    } else if (sgl::endsWith(shaderIdLower, "fragment")) {
        shaderModuleType = ShaderModuleType::FRAGMENT;
    } else if (sgl::endsWith(shaderIdLower, "geometry")) {
        shaderModuleType = ShaderModuleType::GEOMETRY;
    } else if (sgl::endsWith(shaderIdLower, "tesselationevaluation")) {
        shaderModuleType = ShaderModuleType::TESSELATION_EVALUATION;
    } else if (sgl::endsWith(shaderIdLower, "tesselationcontrol")) {
        shaderModuleType = ShaderModuleType::TESSELATION_CONTROL;
    } else if (sgl::endsWith(shaderIdLower, "compute")) {
        shaderModuleType = ShaderModuleType::COMPUTE;
    } else if (sgl::endsWith(shaderIdLower, "raygen")) {
        shaderModuleType = ShaderModuleType::RAYGEN;
    } else if (sgl::endsWith(shaderIdLower, "anyhit")) {
        shaderModuleType = ShaderModuleType::ANY_HIT;
    } else if (sgl::endsWith(shaderIdLower, "closesthit")) {
        shaderModuleType = ShaderModuleType::CLOSEST_HIT;
    } else if (sgl::endsWith(shaderIdLower, "miss")) {
        shaderModuleType = ShaderModuleType::MISS;
    } else if (sgl::endsWith(shaderIdLower, "intersection")) {
        shaderModuleType = ShaderModuleType::INTERSECTION;
    } else if (sgl::endsWith(shaderIdLower, "callable")) {
        shaderModuleType = ShaderModuleType::CALLABLE;
    } else if (sgl::endsWith(shaderIdLower, "tasknv")) {
        shaderModuleType = ShaderModuleType::TASK_NV;
    } else if (sgl::endsWith(shaderIdLower, "meshnv")) {
        shaderModuleType = ShaderModuleType::MESH_NV;
    }
#ifdef VK_EXT_mesh_shader
    else if (sgl::endsWith(shaderIdLower, "taskext")) {
        shaderModuleType = ShaderModuleType::TASK_EXT;
    }
    else if (sgl::endsWith(shaderIdLower, "meshext")) {
        shaderModuleType = ShaderModuleType::MESH_EXT;
    }
#endif
    else {
        if (sgl::stringContains(shaderIdLower, "vert")) {
            shaderModuleType = ShaderModuleType::VERTEX;
        } else if (sgl::stringContains(shaderIdLower, "frag")) {
            shaderModuleType = ShaderModuleType::FRAGMENT;
        } else if (sgl::stringContains(shaderIdLower, "geom")) {
            shaderModuleType = ShaderModuleType::GEOMETRY;
        } else if (sgl::stringContains(shaderIdLower, "tess")) {
            if (sgl::stringContains(shaderIdLower, "eval")) {
                shaderModuleType = ShaderModuleType::TESSELATION_EVALUATION;
            } else if (sgl::stringContains(shaderIdLower, "control")) {
                shaderModuleType = ShaderModuleType::TESSELATION_CONTROL;
            }
        } else if (sgl::stringContains(shaderIdLower, "comp")) {
            shaderModuleType = ShaderModuleType::COMPUTE;
        } else if (sgl::stringContains(shaderIdLower, "raygen")) {
            shaderModuleType = ShaderModuleType::RAYGEN;
        } else if (sgl::stringContains(shaderIdLower, "anyhit")) {
            shaderModuleType = ShaderModuleType::ANY_HIT;
        } else if (sgl::stringContains(shaderIdLower, "closesthit")) {
            shaderModuleType = ShaderModuleType::CLOSEST_HIT;
        } else if (sgl::stringContains(shaderIdLower, "miss")) {
            shaderModuleType = ShaderModuleType::MISS;
        } else if (sgl::stringContains(shaderIdLower, "intersection")) {
            shaderModuleType = ShaderModuleType::INTERSECTION;
        } else if (sgl::stringContains(shaderIdLower, "callable")) {
            shaderModuleType = ShaderModuleType::CALLABLE;
        } else if (sgl::stringContains(shaderIdLower, "tasknv")) {
            shaderModuleType = ShaderModuleType::TASK_NV;
        } else if (sgl::stringContains(shaderIdLower, "meshnv")) {
            shaderModuleType = ShaderModuleType::MESH_NV;
        }
#ifdef VK_EXT_mesh_shader
        else if (sgl::stringContains(shaderIdLower, "taskext")) {
            shaderModuleType = ShaderModuleType::TASK_EXT;
        }
        else if (sgl::stringContains(shaderIdLower, "meshext")) {
            shaderModuleType = ShaderModuleType::MESH_EXT;
        }
#endif
        //else {
        //    Logfile::get()->throwError(
        //            std::string() + "ERROR: ShaderManagerVk::createShaderProgram: "
        //            + "Unknown shader type (id: \"" + shaderId + "\")");
        //    return ShaderModuleType::UNKNOWN;
        //}
    }
    return shaderModuleType;
}

static bool dumpTextDebugStatic = false;

ShaderStagesPtr ShaderManagerVk::createShaderStages(const std::vector<std::string>& shaderIds, bool dumpTextDebug) {
    dumpTextDebugStatic = dumpTextDebug;

    std::vector<ShaderModulePtr> shaderModules;
    for (const std::string &shaderId : shaderIds) {
        ShaderModuleType shaderModuleType = getShaderModuleTypeFromString(shaderId);
        ShaderModulePtr shaderModule = getShaderModule(shaderId, shaderModuleType);
        if (!shaderModule) {
            return ShaderStagesPtr();
        }
        shaderModules.push_back(shaderModule);
    }
    dumpTextDebugStatic = false;

    ShaderStagesPtr shaderProgram(new ShaderStages(device, shaderModules));
    return shaderProgram;
}

ShaderStagesPtr ShaderManagerVk::createShaderStages(
        const std::vector<std::string>& shaderIds, const std::vector<ShaderStageSettings>& shaderStageSettings,
        bool dumpTextDebug) {
    dumpTextDebugStatic = dumpTextDebug;

    std::vector<ShaderModulePtr> shaderModules;
    for (const std::string &shaderId : shaderIds) {
        ShaderModuleType shaderModuleType = getShaderModuleTypeFromString(shaderId);
        ShaderModulePtr shaderModule = getShaderModule(shaderId, shaderModuleType);
        if (!shaderModule) {
            return ShaderStagesPtr();
        }
        shaderModules.push_back(shaderModule);
    }
    dumpTextDebugStatic = false;

    ShaderStagesPtr shaderProgram(new ShaderStages(device, shaderModules, shaderStageSettings));
    return shaderProgram;
}

ShaderModulePtr ShaderManagerVk::loadAsset(ShaderModuleInfo& shaderInfo) {
    sourceStringNumber = 0;
    recursionDepth = 0;
    std::string id = shaderInfo.filename;
    std::string shaderString = getShaderString(id);

    if (isFirstShaderCompilation) {
        std::string backendName = shaderCompilerBackend == ShaderCompilerBackend::SHADERC ? "shaderc" : "glslang";
        sgl::Logfile::get()->write(
                "ShaderManagerVk::loadAsset: Using the " + backendName + " shader compiler backend.",
                sgl::BLUE);
        isFirstShaderCompilation = false;
    }

    if (dumpTextDebugStatic) {
        std::cout << "Shader dump (" << id << "):" << std::endl;
        std::cout << "--------------------------------------------" << std::endl;
        std::cout << shaderString << std::endl << std::endl;
    }

#ifdef SUPPORT_SHADERC_BACKEND
    if (shaderCompilerBackend == ShaderCompilerBackend::SHADERC) {
        return loadAssetShaderc(shaderInfo, id, shaderString);
    }
#endif
#ifdef SUPPORT_GLSLANG_BACKEND
    if (shaderCompilerBackend == ShaderCompilerBackend::GLSLANG) {
        return loadAssetGlslang(shaderInfo, id, shaderString);
    }
#endif
    return {};
}


#ifdef SUPPORT_SHADERC_BACKEND
ShaderModulePtr ShaderManagerVk::loadAssetShaderc(
        ShaderModuleInfo& shaderInfo, const std::string& id, const std::string& shaderString) {
    shaderc::CompileOptions compileOptions;
    for (auto& it : preprocessorDefines) {
        compileOptions.AddMacroDefinition(it.first, it.second);
    }
    for (auto& it : tempPreprocessorDefines) {
        compileOptions.AddMacroDefinition(it.first, it.second);
    }
    auto includerInterface = new IncluderInterface();
    includerInterface->setShaderManager(this);
    compileOptions.SetIncluder(std::unique_ptr<shaderc::CompileOptions::IncluderInterface>(includerInterface));
    if (isOptimizationLevelSet) {
        if (shaderOptimizationLevel == ShaderOptimizationLevel::ZERO) {
            compileOptions.SetOptimizationLevel(shaderc_optimization_level_zero);
        } else if (shaderOptimizationLevel == ShaderOptimizationLevel::SIZE) {
            compileOptions.SetOptimizationLevel(shaderc_optimization_level_size);
        } else {
            compileOptions.SetOptimizationLevel(shaderc_optimization_level_performance);
        }
    }
    if (generateDebugInfo) {
        compileOptions.SetGenerateDebugInfo();
    }


    if (device->getInstance()->getInstanceVulkanVersion() < VK_API_VERSION_1_1) {
        compileOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0);
        compileOptions.SetTargetSpirv(shaderc_spirv_version_1_0);
    } else if (device->getInstance()->getInstanceVulkanVersion() < VK_MAKE_API_VERSION(0, 1, 2, 0)
               || device->getApiVersion() < VK_MAKE_API_VERSION(0, 1, 2, 0)
               || device->getInstance()->getApplicationInfo().apiVersion < VK_MAKE_API_VERSION(0, 1, 2, 0)) {
        compileOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_1);
        compileOptions.SetTargetSpirv(shaderc_spirv_version_1_3);
    }
#if defined(VK_VERSION_1_3) && VK_HEADER_VERSION >= 204 && !defined(SHADERC_NO_VULKAN_1_3_SUPPORT)
    else if (device->getInstance()->getInstanceVulkanVersion() < VK_MAKE_API_VERSION(0, 1, 3, 0)
             || device->getApiVersion() < VK_MAKE_API_VERSION(0, 1, 3, 0)
             || device->getInstance()->getApplicationInfo().apiVersion < VK_MAKE_API_VERSION(0, 1, 3, 0)) {
        compileOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
        compileOptions.SetTargetSpirv(shaderc_spirv_version_1_5);
    } else {
        compileOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
        compileOptions.SetTargetSpirv(shaderc_spirv_version_1_6);
    }
#else
    else {
        compileOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
        compileOptions.SetTargetSpirv(shaderc_spirv_version_1_5);
    }
#endif

    const std::unordered_map<ShaderModuleType, shaderc_shader_kind> shaderKindLookupTable = {
            { ShaderModuleType::VERTEX,                 shaderc_vertex_shader },
            { ShaderModuleType::FRAGMENT,               shaderc_fragment_shader },
            { ShaderModuleType::COMPUTE,                shaderc_compute_shader },
            { ShaderModuleType::GEOMETRY,               shaderc_geometry_shader },
            { ShaderModuleType::TESSELATION_CONTROL,    shaderc_tess_control_shader },
            { ShaderModuleType::TESSELATION_EVALUATION, shaderc_tess_evaluation_shader },
#if VK_VERSION_1_2 && VK_HEADER_VERSION >= 162
            { ShaderModuleType::RAYGEN,                 shaderc_raygen_shader },
            { ShaderModuleType::ANY_HIT,                shaderc_anyhit_shader },
            { ShaderModuleType::CLOSEST_HIT,            shaderc_closesthit_shader },
            { ShaderModuleType::MISS,                   shaderc_miss_shader },
            { ShaderModuleType::INTERSECTION,           shaderc_intersection_shader },
            { ShaderModuleType::CALLABLE,               shaderc_callable_shader },
            { ShaderModuleType::TASK_NV,                shaderc_task_shader },
            { ShaderModuleType::MESH_NV,                shaderc_mesh_shader },
#if defined(VK_EXT_mesh_shader) && (!defined(SUPPORT_GLSLANG_BACKEND) || defined(GLSLANG_MESH_SHADER_EXT_SUPPORT))
            { ShaderModuleType::TASK_EXT,               shaderc_task_shader },
            { ShaderModuleType::MESH_EXT,               shaderc_mesh_shader },
#endif
#endif
    };
    auto it = shaderKindLookupTable.find(shaderInfo.shaderModuleType);
    if (it == shaderKindLookupTable.end()) {
        sgl::Logfile::get()->writeError("Error in ShaderManagerVk::loadAsset: Invalid shader type.");
        return {};
    }
    shaderc_shader_kind shaderKind = it->second;
    shaderc::SpvCompilationResult compilationResult = shaderCompiler->CompileGlslToSpv(
            shaderString.c_str(), shaderString.size(), shaderKind,
            id.c_str(), compileOptions);

    if (compilationResult.GetNumErrors() != 0 || compilationResult.GetNumWarnings() != 0) {
        std::string errorMessage = compilationResult.GetErrorMessage();
        sgl::Logfile::get()->writeErrorMultiline(errorMessage, false);
        auto choice = dialog::openMessageBoxBlocking(
                "Error occurred", errorMessage, dialog::Choice::ABORT_RETRY_IGNORE, dialog::Icon::ERROR);
        if (choice == dialog::Button::RETRY) {
            sgl::vk::ShaderManager->invalidateShaderCache();
            return loadAsset(shaderInfo);
        } else if (choice == dialog::Button::ABORT) {
            exit(1);
        }
        if (compilationResult.GetNumErrors() != 0) {
            return {};
        }
    }

    std::vector<uint32_t> compilationResultWords(compilationResult.cbegin(), compilationResult.cend());
    ShaderModulePtr shaderModule = std::make_shared<ShaderModule>(
            device, shaderInfo.filename, shaderInfo.shaderModuleType, compilationResultWords);
    return shaderModule;
}
#endif


#ifdef SUPPORT_GLSLANG_BACKEND
ShaderModulePtr ShaderManagerVk::loadAssetGlslang(
        ShaderModuleInfo& shaderInfo, const std::string& id, const std::string& shaderString) {
    std::string preprocessorDefinesString = "";
    for (auto& it : preprocessorDefines) {
        preprocessorDefinesString += "#define " + it.first + " " + it.second + "\n";
    }
    for (auto& it : tempPreprocessorDefines) {
        preprocessorDefinesString += "#define " + it.first + " " + it.second + "\n";
    }

    glslang::EShSource source = glslang::EShSourceGlsl;
#ifdef ENABLE_HLSL
    if (endsWith(shaderInfo.filename, ".hlsl")) {
        source = glslang::EShSourceHlsl;
    }
#endif
    glslang::EShClient client = glslang::EShClientVulkan;
    glslang::EShTargetLanguage targetLanguage = glslang::EShTargetSpv;
    glslang::EShTargetClientVersion targetClientVersion = glslang::EShTargetVulkan_1_0;
    glslang::EShTargetLanguageVersion targetLanguageVersion = glslang::EShTargetSpv_1_0;
    if (device->getInstance()->getInstanceVulkanVersion() < VK_API_VERSION_1_1) {
        targetClientVersion = glslang::EShTargetVulkan_1_0;
        targetLanguageVersion = glslang::EShTargetSpv_1_0;
    } else if (device->getInstance()->getInstanceVulkanVersion() < VK_MAKE_API_VERSION(0, 1, 2, 0)
               || device->getApiVersion() < VK_MAKE_API_VERSION(0, 1, 2, 0)
               || device->getInstance()->getApplicationInfo().apiVersion < VK_MAKE_API_VERSION(0, 1, 2, 0)) {
        targetClientVersion = glslang::EShTargetVulkan_1_1;
        targetLanguageVersion = glslang::EShTargetSpv_1_3;
    }
#if defined(VK_VERSION_1_3) && VK_HEADER_VERSION >= 204 && !defined(GLSLANG_NO_VULKAN_1_3_SUPPORT)
    else if (device->getInstance()->getInstanceVulkanVersion() < VK_MAKE_API_VERSION(0, 1, 3, 0)
             || device->getApiVersion() < VK_MAKE_API_VERSION(0, 1, 3, 0)
             || device->getInstance()->getApplicationInfo().apiVersion < VK_MAKE_API_VERSION(0, 1, 3, 0)) {
        targetClientVersion = glslang::EShTargetVulkan_1_2;
        targetLanguageVersion = glslang::EShTargetSpv_1_5;
    } else {
        targetClientVersion = glslang::EShTargetVulkan_1_3;
        targetLanguageVersion = glslang::EShTargetSpv_1_6;
    }
#else
    else {
        targetClientVersion = glslang::EShTargetVulkan_1_2;
        targetLanguageVersion = glslang::EShTargetSpv_1_5;
    }
#endif

    const std::unordered_map<ShaderModuleType, EShLanguage> shaderKindLookupTable = {
            { ShaderModuleType::VERTEX,                 EShLangVertex },
            { ShaderModuleType::FRAGMENT,               EShLangFragment },
            { ShaderModuleType::COMPUTE,                EShLangCompute },
            { ShaderModuleType::GEOMETRY,               EShLangGeometry },
            { ShaderModuleType::TESSELATION_CONTROL,    EShLangTessControl },
            { ShaderModuleType::TESSELATION_EVALUATION, EShLangTessEvaluation },
#if VK_VERSION_1_2 && VK_HEADER_VERSION >= 162
            { ShaderModuleType::RAYGEN,                 EShLangRayGen },
            { ShaderModuleType::ANY_HIT,                EShLangAnyHit },
            { ShaderModuleType::CLOSEST_HIT,            EShLangClosestHit },
            { ShaderModuleType::MISS,                   EShLangMiss },
            { ShaderModuleType::INTERSECTION,           EShLangIntersect },
            { ShaderModuleType::CALLABLE,               EShLangCallable },
            { ShaderModuleType::TASK_NV,                EShLangTaskNV },
            { ShaderModuleType::MESH_NV,                EShLangMeshNV },
#if defined(VK_EXT_mesh_shader) && defined(GLSLANG_MESH_SHADER_EXT_SUPPORT)
            { ShaderModuleType::TASK_EXT,               EShLangTask },
            { ShaderModuleType::MESH_EXT,               EShLangMesh },
#endif
#endif
    };
    auto it = shaderKindLookupTable.find(shaderInfo.shaderModuleType);
    if (it == shaderKindLookupTable.end()) {
        sgl::Logfile::get()->writeError("Error in ShaderManagerVk::loadAsset: Invalid shader type.");
        return {};
    }
    EShLanguage shaderKind = it->second;
    //EShMessages messages = EShMsgDefault;
    auto messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
    auto* shader = new glslang::TShader(shaderKind);
    auto* program = new glslang::TProgram();
    auto shaderStringSize = int(shaderString.size());
    const char* shaderStringPtr = shaderString.c_str();
    const char* idStringPtr = id.c_str();
    shader->setStringsWithLengthsAndNames(&shaderStringPtr, &shaderStringSize, &idStringPtr, 1);
    if (!preprocessorDefinesString.empty()) {
        shader->setPreamble(preprocessorDefinesString.c_str());
    }
    shader->setEnvInput(source, shaderKind, client, 100);
    shader->setEnvClient(client, targetClientVersion);
    shader->setEnvTarget(targetLanguage, targetLanguageVersion);
    //shader->setEntryPoint(entryPointName);
    //shader->setOverrideVersion(glslVersion); // e.g., 450.
    TBuiltInResource builtInResource = {};
    initializeBuiltInResourceGlslang(builtInResource);
    if (!shader->parse(&builtInResource, 100, false, messages)) {
        std::string errorString = "Error in ShaderManagerVk::loadAssetGlslang: Shader parsing failed. \n";
        errorString += shader->getInfoLog();
        errorString += shader->getInfoDebugLog();

        sgl::Logfile::get()->writeErrorMultiline(errorString, false);
        auto choice = dialog::openMessageBoxBlocking(
                "Error occurred", errorString, dialog::Choice::ABORT_RETRY_IGNORE, dialog::Icon::ERROR);
        if (choice == dialog::Button::RETRY) {
            sgl::vk::ShaderManager->invalidateShaderCache();
            return loadAsset(shaderInfo);
        } else if (choice == dialog::Button::ABORT) {
            exit(1);
        }
        return {};
    }

    program->addShader(shader);
    if (!program->link(messages)) {
        std::string errorString = "Error in ShaderManagerVk::loadAssetGlslang: Program linking failed. \n";
        errorString += program->getInfoLog();
        errorString += program->getInfoDebugLog();

        sgl::Logfile::get()->writeErrorMultiline(errorString, false);
        auto choice = dialog::openMessageBoxBlocking(
                "Error occurred", errorString, dialog::Choice::ABORT_RETRY_IGNORE, dialog::Icon::ERROR);
        if (choice == dialog::Button::RETRY) {
            sgl::vk::ShaderManager->invalidateShaderCache();
            return loadAsset(shaderInfo);
        } else if (choice == dialog::Button::ABORT) {
            exit(1);
        }
        return {};
    }

    std::vector<uint32_t> compilationResultWords;
    glslang::GlslangToSpv(*program->getIntermediate(shaderKind), compilationResultWords);
    ShaderModulePtr shaderModule(new ShaderModule(
            device, shaderInfo.filename, shaderInfo.shaderModuleType,
            compilationResultWords));

    delete shader;
    delete program;
    return shaderModule;
}
#endif


std::string ShaderManagerVk::loadHeaderFileString(const std::string& shaderName, std::string& prependContent) {
    return loadHeaderFileString(
        shaderName, sgl::FileUtils::get()->getPureFilename(shaderName), prependContent);
}

std::string ShaderManagerVk::loadHeaderFileString(
        const std::string& shaderName, const std::string& headerName, std::string &prependContent) {
    std::ifstream file(shaderName.c_str());
    if (!file.is_open()) {
        Logfile::get()->throwError(
                std::string() + "Error in loadHeaderFileString: Couldn't open the file \"" + shaderName + "\".");
        return "";
    }
    //std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    sourceStringNumber++;
    std::string fileContent;
    if (useCppLineStyle) {
        fileContent = "#line 1 \"" + headerName + "\"\n";
    } else if (dumpTextDebugStatic) {
        fileContent = "#line 1 " + std::to_string(sourceStringNumber) + "\n";
    } else {
        fileContent = "#line 1\n";
    }

    // Support preprocessor for embedded headers
    bool hasUsedInclude = false;
    int preprocessorConditionalsDepth = 0;
    int lineNum = 1;
    std::string linestr, trimmedLinestr;
    while (getline(file, linestr)) {
        // Remove \r if line ending is \r\n
        if (!linestr.empty() && linestr.at(linestr.size() - 1) == '\r') {
            linestr = linestr.substr(0, linestr.size() - 1);
        }

        trimmedLinestr = sgl::stringTrimCopy(linestr);
        lineNum++;

        if (sgl::startsWith(trimmedLinestr, "#include")) {
            std::string headerNameSub = getHeaderName(linestr);
            std::string includedFileName = getShaderFileName(headerNameSub);
            std::string includedFileContent = loadHeaderFileString(includedFileName, headerNameSub, prependContent);
            fileContent += includedFileContent + "\n";
            if (useCppLineStyle) {
                fileContent +=
                        std::string() + "#line " + toString(lineNum) + " \"" + headerName + "\"\n";
            } else if (dumpTextDebugStatic) {
                fileContent +=
                        std::string() + "#line " + toString(lineNum) + " " + std::to_string(sourceStringNumber) + "\n";
            } else {
                fileContent += std::string() + "#line " + toString(lineNum) + "\n";
            }
            if (preprocessorConditionalsDepth > 0) {
                hasUsedInclude = true;
            }
        } else if (sgl::startsWith(trimmedLinestr, "#import")) {
            std::string importedShaderModuleContent = getImportedShaderString(
                    getHeaderName(linestr), "", prependContent);
            fileContent += importedShaderModuleContent + "\n";
            if (useCppLineStyle) {
                fileContent +=
                        std::string() + "#line " + toString(lineNum) + " \"" + headerName + "\"\n";
            } else if (dumpTextDebugStatic) {
                fileContent +=
                        std::string() + "#line " + toString(lineNum) + " "
                        + std::to_string(sourceStringNumber) + "\n";
            } else {
                fileContent += std::string() + "#line " + toString(lineNum) + "\n";
            }
            if (preprocessorConditionalsDepth > 0) {
                hasUsedInclude = true;
            }
        } else if (sgl::startsWith(trimmedLinestr, "#extension") || sgl::startsWith(trimmedLinestr, "#version")) {
            prependContent += linestr + "\n";
            if (useCppLineStyle) {
                fileContent += std::string() + "#line " + toString(lineNum) + " \"" + headerName + "\"\n";
            } else if (dumpTextDebugStatic) {
                fileContent += "#line " + toString(lineNum) + " " + std::to_string(sourceStringNumber) + "\n";
            } else {
                fileContent += "#line " + toString(lineNum) + "\n";
            }
        } else if (sgl::startsWith(sgl::stringTrimCopy(trimmedLinestr), "#if")) {
            fileContent += std::string() + linestr + "\n";
            preprocessorConditionalsDepth++;
        } else if (sgl::startsWith(sgl::stringTrimCopy(trimmedLinestr), "#endif")) {
            fileContent += std::string() + linestr + "\n";
            preprocessorConditionalsDepth--;
            /*
              * Tests seem to indicate that #line statements are affected by #if/#ifdef.
              * Consequentially, to be conservative, a #line statement will be inserted after every #endif after an
              * include statement.
              */
            if (hasUsedInclude) {
                if (useCppLineStyle) {
                    fileContent +=
                            std::string() + "#line " + toString(lineNum) + " \"" + headerName + "\"\n";
                } else if (dumpTextDebugStatic) {
                    fileContent +=
                            std::string() + "#line " + toString(lineNum) + " " + std::to_string(sourceStringNumber) + "\n";
                } else {
                    fileContent += std::string() + "#line " + toString(lineNum) + "\n";
                }
            }
            if (preprocessorConditionalsDepth == 0 && hasUsedInclude) {
                hasUsedInclude = false;
            }
        } else {
            fileContent += std::string() + linestr + "\n";
        }
    }

    file.close();
    return fileContent;
}


std::string ShaderManagerVk::getHeaderName(const std::string &lineString) {
    // Filename in quotes?
    auto startFilename = lineString.find('\"');
    auto endFilename = lineString.find_last_of('\"');
    if (startFilename != std::string::npos && endFilename != std::string::npos) {
        return lineString.substr(startFilename+1, endFilename-startFilename-1);
    } else {
        // Filename is user-specified #define directive?
        std::vector<std::string> line;
        sgl::splitStringWhitespace(lineString, line);
        if (line.size() < 2) {
            Logfile::get()->writeError("Error in ShaderManagerVk::getHeaderFilename: Too few tokens.");
            return "";
        }

        auto it = preprocessorDefines.find(line.at(1));
        auto itTemp = tempPreprocessorDefines.find(line.at(1));
        if (itTemp != tempPreprocessorDefines.end()) {
            std::string::size_type startFilename = itTemp->second.find('\"');
            std::string::size_type endFilename = itTemp->second.find_last_of('\"');
            return itTemp->second.substr(startFilename+1, endFilename-startFilename-1);
        } else if (it != preprocessorDefines.end()) {
            std::string::size_type startFilename = it->second.find('\"');
            std::string::size_type endFilename = it->second.find_last_of('\"');
            return it->second.substr(startFilename+1, endFilename-startFilename-1);
        } else {
            Logfile::get()->writeError("Error in ShaderManagerVk::getHeaderFilename: Invalid include directive.");
            Logfile::get()->writeError(std::string() + "Line string: " + lineString);
            return "";
        }
    }
}





void ShaderManagerVk::indexFiles(const std::string &file) {
    if (FileUtils::get()->isDirectory(file)) {
        // Scan content of directory
        std::vector<std::string> elements = FileUtils::get()->getFilesInDirectoryVector(file);
        for (std::string &childFile : elements) {
            indexFiles(childFile);
        }
    } else if (FileUtils::get()->hasExtension(file.c_str(), ".glsl")) {
        // File to index. "fileName" is name without path.
        std::string fileName = FileUtils::get()->getPureFilename(file);
        shaderFileMap.insert(make_pair(fileName, file));
    }
}


std::string ShaderManagerVk::getShaderFileName(const std::string &pureFilename) {
    auto it = shaderFileMap.find(pureFilename);
    if (it == shaderFileMap.end()) {
        sgl::Logfile::get()->writeError(
                "Error in ShaderManagerVk::getShaderFileName: Unknown file name \"" + pureFilename + "\".");
        return "";
    }
    return it->second;
}


std::string ShaderManagerVk::getPreprocessorDefines(ShaderModuleType shaderModuleType) {
    std::string preprocessorStatements;
    for (auto it = preprocessorDefines.begin(); it != preprocessorDefines.end(); it++) {
        preprocessorStatements += std::string() + "#define " + it->first + " " + it->second + "\n";
    }
    if (shaderModuleType == ShaderModuleType::VERTEX || shaderModuleType == ShaderModuleType::GEOMETRY
            || shaderModuleType == ShaderModuleType::FRAGMENT || shaderModuleType == ShaderModuleType::MESH_NV
#ifdef VK_EXT_mesh_shader
            || shaderModuleType == ShaderModuleType::MESH_EXT
#endif
    ) {
        preprocessorStatements += globalDefinesMvpMatrices;
    }
    preprocessorStatements += globalDefines;
    return preprocessorStatements;
}


std::string ShaderManagerVk::getImportedShaderString(
        const std::string& moduleName, const std::string& parentModuleName, std::string& prependContent) {
    recursionDepth++;
    if (recursionDepth > 1) {
        sgl::Logfile::get()->throwError(
                "Error in ShaderManagerVk::getImportedShaderString: Nested/recursive imports are not supported.");
    }

    if (moduleName.empty()) {
        sgl::Logfile::get()->throwError(
                "Error in ShaderManagerVk::getImportedShaderString: Empty import statement in module \""
                + parentModuleName + "\".");
    }

    std::string absoluteModuleName;
    if (moduleName.front() == '.') {
        // Relative mode.
        absoluteModuleName = parentModuleName + moduleName;
    } else {
        // Absolute mode.
        absoluteModuleName = moduleName;
    }

    std::string::size_type filenameEnd = absoluteModuleName.find('.');
    std::string pureFilename = absoluteModuleName.substr(0, filenameEnd);

    std::string moduleContentString;
    if (pureFilename != parentModuleName) {
        getShaderString(absoluteModuleName);
    }

    // Only allow importing previously defined modules for now.
    auto itRaw = effectSourcesRaw.find(absoluteModuleName);
    auto itPrepend = effectSourcesPrepend.find(absoluteModuleName);
    if (itRaw != effectSourcesRaw.end() || itPrepend != effectSourcesPrepend.end()) {
        moduleContentString = itRaw->second;
        prependContent += itPrepend->second;
    } else {
        sgl::Logfile::get()->throwError(
                "Error in ShaderManagerVk::getImportedShaderString: The module \"" + absoluteModuleName
                + "\" couldn't be found. Hint: Only modules occurring in a file before the importing module can be "
                  "imported.");
    }

    recursionDepth--;
    return moduleContentString;
}

void ShaderManagerVk::addExtensions(std::string& prependContent, const std::map<std::string, std::string>& defines) {
    auto extensionsIt = defines.find("__extensions");
    if (extensionsIt != defines.end()) {
        std::vector<std::string> extensions;
        sgl::splitString2(extensionsIt->second, extensions, ';', ',');
        for (const std::string& extension : extensions) {
            prependContent += "#extension " + extension + " : require\n";
        }
    }
}

std::string ShaderManagerVk::getShaderString(const std::string &globalShaderName) {
    auto it = effectSources.find(globalShaderName);
    if (it != effectSources.end()) {
        return it->second;
    }

    std::string::size_type filenameEnd = globalShaderName.find('.');
    std::string pureFilename = globalShaderName.substr(0, filenameEnd);
    std::string shaderFilename = getShaderFileName(pureFilename + ".glsl");
    std::string shaderInternalId = globalShaderName.substr(filenameEnd + 1);

    std::ifstream file(shaderFilename.c_str());
    if (!file.is_open()) {
        Logfile::get()->throwError(
                std::string() + "Error in getShader: Couldn't open the file \"" + shaderFilename + "\".");
    }

    int oldSourceStringNumber = sourceStringNumber;
    bool hasUsedInclude = false;

    std::string shaderContent;
    if (useCppLineStyle) {
        shaderContent = "#line 1 \"" + globalShaderName + "\"\n";
    } else if (dumpTextDebugStatic) {
        shaderContent = "#line 1 " + std::to_string(sourceStringNumber) + "\n";
    } else {
        shaderContent = "#line 1\n";
    }

    std::string extensionsString;
    addExtensions(extensionsString, preprocessorDefines);
    addExtensions(extensionsString, tempPreprocessorDefines);
    std::string shaderName;
    std::string prependContent;
    if (useCppLineStyle) {
        prependContent += "#extension GL_GOOGLE_cpp_style_line_directive : enable\n";
    }

    int preprocessorConditionalsDepth = 0;
    int lineNum = 1;
    std::string linestr, trimmedLinestr;
    while (getline(file, linestr)) {
        // Remove \r if line ending is \r\n
        if (!linestr.empty() && linestr.at(linestr.size()-1) == '\r') {
            linestr = linestr.substr(0, linestr.size()-1);
        }

        trimmedLinestr = sgl::stringTrimCopy(linestr);
        lineNum++;

        if (sgl::startsWith(linestr, "-- ")) {
            if (!shaderContent.empty() && !shaderName.empty()) {
                effectSourcesRaw.insert(make_pair(shaderName, shaderContent));
                effectSourcesPrepend.insert(make_pair(shaderName, prependContent));
                shaderContent = prependContent + shaderContent;
                effectSources.insert(make_pair(shaderName, shaderContent));
            }

            sourceStringNumber = oldSourceStringNumber;
            shaderName = pureFilename + "." + linestr.substr(3);
            ShaderModuleType shaderModuleType = getShaderModuleTypeFromString(shaderName);
            if (useCppLineStyle) {
                shaderContent =
                        std::string() + getPreprocessorDefines(shaderModuleType) + "#line " + toString(lineNum)
                        + " \"" + shaderName + "\"\n";
            } else if (dumpTextDebugStatic) {
                shaderContent =
                        std::string() + getPreprocessorDefines(shaderModuleType) + "#line " + toString(lineNum)
                        + " " + std::to_string(sourceStringNumber) + "\n";
            } else {
                shaderContent =
                        std::string() + getPreprocessorDefines(shaderModuleType) + "#line " + toString(lineNum)
                        + "\n";
            }
            if (shaderModuleType == ShaderModuleType::FRAGMENT
                    || extensionsString != "#extension GL_ARB_fragment_shader_interlock : require\n") {
                prependContent = extensionsString;
            } else {
                prependContent.clear();
            }
            if (useCppLineStyle) {
                prependContent += "#extension GL_GOOGLE_cpp_style_line_directive : enable\n";
            }
        } else if (sgl::startsWith(trimmedLinestr, "#version") || sgl::startsWith(trimmedLinestr, "#extension")) {
            if (sgl::startsWith(linestr, "#version")) {
                prependContent = linestr + "\n" + prependContent;
            } else {
                prependContent += linestr + "\n";
            }
            if (useCppLineStyle) {
                shaderContent +=
                        std::string() + "#line " + toString(lineNum) + " \"" + shaderName + "\"\n";
            } else if (dumpTextDebugStatic) {
                shaderContent +=
                        std::string() + "#line " + toString(lineNum) + " " + std::to_string(sourceStringNumber) + "\n";
            } else {
                shaderContent += std::string() + "#line " + toString(lineNum) + "\n";
            }
        } else if (sgl::startsWith(trimmedLinestr, "#include")) {
            std::string headerName = getHeaderName(linestr);
            std::string includedFileName = getShaderFileName(headerName);
            std::string includedFileContent = loadHeaderFileString(includedFileName, headerName, prependContent);
            shaderContent += includedFileContent + "\n";
            if (useCppLineStyle) {
                shaderContent +=
                        std::string() + "#line " + toString(lineNum) + " \"" + shaderName + "\"\n";
            } else if (dumpTextDebugStatic) {
                shaderContent +=
                        std::string() + "#line " + toString(lineNum) + " " + std::to_string(sourceStringNumber) + "\n";
            } else {
                shaderContent += std::string() + "#line " + toString(lineNum) + "\n";
            }
            if (preprocessorConditionalsDepth > 0) {
                hasUsedInclude = true;
            }
        } else if (sgl::startsWith(trimmedLinestr, "#import")) {
            std::string importedShaderModuleContent = getImportedShaderString(
                    getHeaderName(linestr), pureFilename, prependContent);
            shaderContent += importedShaderModuleContent + "\n";
            if (useCppLineStyle) {
                shaderContent +=
                        std::string() + "#line " + toString(lineNum) + " \"" + shaderName + "\"\n";
            } else if (dumpTextDebugStatic) {
                shaderContent +=
                        std::string() + "#line " + toString(lineNum) + " " + std::to_string(sourceStringNumber) + "\n";
            } else {
                shaderContent += std::string() + "#line " + toString(lineNum) + "\n";
            }
            if (preprocessorConditionalsDepth > 0) {
                hasUsedInclude = true;
            }
        } else if (sgl::startsWith(trimmedLinestr, "#codefrag")) {
            const std::string& codeFragmentName = getHeaderName(linestr);
            auto codeFragIt = tempPreprocessorDefines.find(codeFragmentName);
            if (codeFragIt != tempPreprocessorDefines.end()) {
                if (useCppLineStyle) {
                    shaderContent +=
                            std::string() + "#line 1 \"" + codeFragmentName + "\"\n";
                } else {
                    shaderContent += "#line 1\n";
                }
                shaderContent += codeFragIt->second + "\n";
                if (useCppLineStyle) {
                    shaderContent +=
                            std::string() + "#line " + toString(lineNum) + " \"" + shaderName + "\"\n";
                } else {
                    shaderContent += std::string() + "#line " + toString(lineNum) + "\n";
                }
                tempPreprocessorDefines.erase(codeFragmentName);
            }
            if (preprocessorConditionalsDepth > 0) {
                hasUsedInclude = true;
            }
        } else if (sgl::startsWith(sgl::stringTrimCopy(trimmedLinestr), "#if")) {
            shaderContent += std::string() + linestr + "\n";
            preprocessorConditionalsDepth++;
        } else if (sgl::startsWith(sgl::stringTrimCopy(trimmedLinestr), "#endif")) {
            shaderContent += std::string() + linestr + "\n";
            preprocessorConditionalsDepth--;
            /*
              * Tests seem to indicate that #line statements are affected by #if/#ifdef.
              * Consequentially, to be conservative, a #line statement will be inserted after every #endif after an
              * include statement.
              */
            if (hasUsedInclude) {
                if (useCppLineStyle) {
                    shaderContent +=
                            std::string() + "#line " + toString(lineNum) + " \"" + shaderName + "\"\n";
                } else if (dumpTextDebugStatic) {
                    shaderContent +=
                            std::string() + "#line " + toString(lineNum) + " " + std::to_string(sourceStringNumber) + "\n";
                } else {
                    shaderContent += std::string() + "#line " + toString(lineNum) + "\n";
                }
            }
            if (preprocessorConditionalsDepth == 0 && hasUsedInclude) {
                hasUsedInclude = false;
            }
        } else {
            shaderContent += std::string() + linestr + "\n";
        }
    }
    shaderContent = prependContent + shaderContent;
    file.close();

    sourceStringNumber = oldSourceStringNumber;

    if (!shaderName.empty()) {
        effectSources.insert(make_pair(shaderName, shaderContent));
    } else {
        effectSources.insert(make_pair(pureFilename + ".glsl", shaderContent));
    }

    it = effectSources.find(globalShaderName);
    if (it != effectSources.end()) {
        return it->second;
    }

    Logfile::get()->writeError(
            std::string() + "Error in getShader: Couldn't find the shader \"" + globalShaderName + "\".");
    return "";
}

void ShaderManagerVk::invalidateShaderCache() {
    assetMap.clear();
    effectSources.clear();
    effectSourcesRaw.clear();
    effectSourcesPrepend.clear();
}

}}
