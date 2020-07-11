/*!
 * SystemGL.hpp
 *
 *  Created on: 10.01.2015
 *      Author: Christoph Neuhauser
 */

#ifndef GRAPHICS_OPENGL_SYSTEMGL_HPP_
#define GRAPHICS_OPENGL_SYSTEMGL_HPP_

#include <Defs.hpp>
#include <Utils/Singleton.hpp>
#include <string>
#include <unordered_set>

namespace sgl {

//! Get information about the OpenGL context
class DLL_OBJECT SystemGL : public Singleton<SystemGL>
{
public:
    SystemGL();

    //! Information about version of OpenGL
    bool isGLExtensionAvailable(const char *extensionName);
    inline int getGLMajorVersionNumber() { return majorVersionNumber; }
    inline int getGLMinorVersionNumber() { return minorVersionNumber; }
    inline int getGLMajorShadingLanguageVersionNumber() { return majorShadingLanguageVersionNumber; }
    inline int getGLMinorShadingLanguageVersionNumber() { return minorShadingLanguageVersionNumber; }
    inline std::string getVendorString() { return vendorString; }

    /*! Returns whether the current OpenGL context supports the features of the passed OpenGL version
     * You could for example call "openglVersionMinimum(3)" or "openglVersionMinimum(2, 1)" */
    bool openglVersionMinimum(int major, int minor = 0);

    //! Information about hardware limitations
    inline int getMaximumTextureSize() { return maximumTextureSize; }
    inline int getMaximumTextureSamples() { return maxSamples; }
    inline float getMaximumAnisotropy() { return maxSamples; }
    inline float getMinimumLineSize() { return glLineSizeRange[0]; }
    inline float getMaximumLineSize() { return glLineSizeRange[1]; }
    inline float getLineSizeIncrementStep() { return glLineSizeIncrementStep; }

    //! Enable/disable features of engine
    //! Standard: true
    void setPremulAlphaEnabled(bool enabled);
    bool isPremulAphaEnabled() { return premulAlphaEnabled; }

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
