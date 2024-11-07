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

#ifndef SGL_PREPROCESSORGLSL_HPP
#define SGL_PREPROCESSORGLSL_HPP

#include <string>
#include <map>

namespace sgl {

enum class ShaderModuleTypeGlsl {
    UNKNOWN = 0,
    VERTEX = 0x00000001,
    TESSELLATION_CONTROL = 0x00000002,
    TESSELLATION_EVALUATION = 0x00000004,
    GEOMETRY = 0x00000008,
    FRAGMENT = 0x00000010,
    COMPUTE = 0x00000020,
    RAYGEN = 0x00000100,
    ANY_HIT = 0x00000200,
    CLOSEST_HIT = 0x00000400,
    MISS = 0x00000800,
    INTERSECTION = 0x00001000,
    CALLABLE = 0x00002000,
    TASK_NV = 0x00000040,
    MESH_NV = 0x00000080,
    TASK_EXT = 0x00000040 | 0x10000000, // NV == EXT, so mark using unused bit 28.
    MESH_EXT = 0x00000080 | 0x10000000, // NV == EXT, so mark using unused bit 28.
};

class DLL_OBJECT PreprocessorGlsl {
public:
    PreprocessorGlsl();

    // Global settings.
    void setUseCppLineStyle(bool _useCppLineStyle) { useCppLineStyle = _useCppLineStyle; }
    [[nodiscard]] bool getDumpTextDebugStatic() const { return dumpTextDebugStatic; }
    void setDumpTextDebugStatic(bool _dumpTextDebugStatic) { dumpTextDebugStatic = _dumpTextDebugStatic; }

    /**
     * Deletes all cached shaders in the ShaderManager. This is necessary, e.g., when wanting to switch to a
     * different rendering technique with "addPreprocessorDefine" after already having loaded a certain shader.
     * Already loaded shaders will stay intact thanks to reference counting.
     */
    void invalidateShaderCache();

    // For use by IncluderInterface.
    [[nodiscard]] std::map<std::string, std::string>& getShaderFileMap() { return shaderFileMap; }

    /**
     * Used for adding preprocessor defines to all shader files before compiling.
     * This function is useful for e.g. switching at runtime between different different techniques.
     * The generated preprocessor statements are of the form "#define <token> <value>".
     */
    template<typename T>
    void addPreprocessorDefine(const std::string& token, const T& value) {
        preprocessorDefines[token] = toString(value);
    }
    void addPreprocessorDefine(const std::string& token, const std::string& value) {
        preprocessorDefines[token] = value;
    }
    void addPreprocessorDefine(const std::string& token) {
        preprocessorDefines[token] = "";
    }
    std::string getPreprocessorDefine(const std::string& token) {
        return preprocessorDefines[token];
    }

    /// Removes a preprocessor #define token set by "addPreprocessorDefine".
    void removePreprocessorDefine(const std::string& token) {
        auto it = preprocessorDefines.find(token);
        if (it != preprocessorDefines.end()) {
            preprocessorDefines.erase(it);
        }
    }

    inline const std::map<std::string, std::string>& getPreprocessorDefines() { return preprocessorDefines; }
    inline const std::map<std::string, std::string>& getTempPreprocessorDefines() { return tempPreprocessorDefines; }
    void setTempPreprocessorDefines(const std::map<std::string, std::string>& _tempPreprocessorDefines) {
        tempPreprocessorDefines = _tempPreprocessorDefines;
    }
    inline void clearTempPreprocessorDefines() { tempPreprocessorDefines.clear(); }
    void setGlobalDefines(const std::string& _globalDefines) { globalDefines = _globalDefines; }
    // Global defines for vertex and geometry shaders.
    void setGlobalDefinesMvpMatrices(const std::string& _globalDefines) { globalDefinesMvpMatrices = _globalDefines; }

    /**
     * @see After indexFiles was called by the constructor, this function can be used to resolve a shader file path.
     */
    std::string getShaderFileName(const std::string& pureFilename);
    std::string getShaderString(const std::string& globalShaderName);
    void resetLoad();
    void loadGlobalDefinesFileIfExists(const std::string& id);
    std::map<std::string, std::string>& getEffectSources() { return effectSources; }

    std::string loadHeaderFileString(const std::string& shaderName, std::string& prependContent);
    std::string loadHeaderFileString(
            const std::string& shaderName, const std::string& headerName, std::string& prependContent);

private:
    /// Internal loading
    std::string getHeaderName(const std::string& lineString);
    std::string getImportedShaderString(
            const std::string& moduleName, const std::string& parentModuleName, std::string& prependContent);
    std::string getPreprocessorDefines(ShaderModuleTypeGlsl shaderModuleType);
    static void addExtensions(std::string& prependContent, const std::map<std::string, std::string>& defines);

    /// Maps shader name -> shader source, e.g. "Blur.Fragment" -> "void main() { ... }".
    std::map<std::string, std::string> effectSources;
    std::map<std::string, std::string> effectSourcesRaw; ///< without prepended header.
    std::map<std::string, std::string> effectSourcesPrepend; ///< only prepended header.

    /// Maps file names without path to full file paths for "*.glsl" shader files,
    /// e.g. "Blur.glsl" -> "Data/Shaders/PostProcessing/Blur.glsl".
    std::map<std::string, std::string> shaderFileMap;
    int sourceStringNumber = 0;
    int recursionDepth = 0;
    /// Whether to include a file name in "#line" directives (for more details see:
    /// https://github.com/google/shaderc/tree/main/glslc#51-source-filename-based-line-and-__file__).
    bool useCppLineStyle = true;
    bool dumpTextDebugStatic = false;

    // If a file named "GlobalDefinesVulkan.glsl" is found: Appended to all shaders.
    std::string globalDefines;
    // Global defines for vertex and geometry shaders.
    std::string globalDefinesMvpMatrices;

    /// A token-value map for user-provided preprocessor #define's
    std::map<std::string, std::string> preprocessorDefines;
    std::map<std::string, std::string> tempPreprocessorDefines; // Temporarily set when loading a shader.
};

}

#endif //SGL_PREPROCESSORGLSL_HPP
