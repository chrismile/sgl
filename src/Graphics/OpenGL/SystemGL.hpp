/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2015, Christoph Neuhauser
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

#ifndef GRAPHICS_OPENGL_SYSTEMGL_HPP_
#define GRAPHICS_OPENGL_SYSTEMGL_HPP_

#include <string>
#include <unordered_set>
#include <cstdint>

#include <Defs.hpp>
#include <Utils/Singleton.hpp>

namespace sgl {

/// Get information about the OpenGL context.
class DLL_OBJECT SystemGL : public Singleton<SystemGL> {
public:
    SystemGL();

    /// Information about version of OpenGL
    bool isGLExtensionAvailable(const char *extensionName);
    [[nodiscard]] inline int getGLMajorVersionNumber() const { return majorVersionNumber; }
    [[nodiscard]] inline int getGLMinorVersionNumber() const { return minorVersionNumber; }
    [[nodiscard]] inline int getGLMajorShadingLanguageVersionNumber() const { return majorShadingLanguageVersionNumber; }
    [[nodiscard]] inline int getGLMinorShadingLanguageVersionNumber() const { return minorShadingLanguageVersionNumber; }
    [[nodiscard]] inline const std::string& getVendorString() const { return vendorString; }

    /*! Returns whether the current OpenGL context supports the features of the passed OpenGL version
     * You could for example call "openglVersionMinimum(3)" or "openglVersionMinimum(2, 1)" */
    bool openglVersionMinimum(int major, int minor = 0);

    /// Information about hardware limitations.
    [[nodiscard]] inline int getMaximumTextureSize() const { return maximumTextureSize; }
    [[nodiscard]] inline int getMaximumTextureSamples() const { return maxSamples; }
    [[nodiscard]] inline float getMaximumAnisotropy() const { return maximumAnisotropy; }
    [[nodiscard]] inline float getMinimumLineSize() const { return glLineSizeRange[0]; }
    [[nodiscard]] inline float getMaximumLineSize() const { return glLineSizeRange[1]; }
    [[nodiscard]] inline float getLineSizeIncrementStep() const { return glLineSizeIncrementStep; }

    /// Enable/disable features of engine.
    /// Standard: true
    void setPremulAlphaEnabled(bool enabled);
    [[nodiscard]] bool isPremulAphaEnabled() const { return premulAlphaEnabled; }

    /// Load OpenGL function pointers from the current context.
    void* getFunctionPointer(const char* functionName);

    uint64_t getFreeMemoryBytes();

private:
    std::unordered_set<std::string> extensions;
    std::string versionString;
    std::string vendorString;
    std::string shadingLanguageVersionString;
    int majorVersionNumber;
    int minorVersionNumber;
    int majorShadingLanguageVersionNumber;
    int minorShadingLanguageVersionNumber;
    int maximumTextureSize;
    float maximumAnisotropy;
    float glLineSizeRange[2];
    float glLineSizeIncrementStep;
    int maxSamples;
    bool premulAlphaEnabled;
};

}

/*! GRAPHICS_OPENGL_SYSTEMGL_HPP_ */
#endif
