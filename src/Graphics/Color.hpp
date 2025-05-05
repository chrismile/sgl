/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2017-2020, Christoph Neuhauser
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

#ifndef SRC_GRAPHICS_COLOR_HPP_
#define SRC_GRAPHICS_COLOR_HPP_

#include <algorithm>
#include <cmath>
#include <Utils/Convert.hpp>
#ifdef USE_GLM
#include <glm/glm.hpp>
#else
#include <Math/Geometry/vec.hpp>
#endif

namespace sgl {

class DLL_OBJECT Color {
public:
    Color(uint8_t R = 255, uint8_t G = 255, uint8_t B = 255, uint8_t A = 255) : r(R), g(G), b(B), a(A) {}
    Color(const glm::vec4 &colorNormalized) {
        this->r = std::clamp((int)std::round(colorNormalized.r*255.0f), 0, 255);
        this->g = std::clamp((int)std::round(colorNormalized.g*255.0f), 0, 255);
        this->b = std::clamp((int)std::round(colorNormalized.b*255.0f), 0, 255);
        this->a = std::clamp((int)std::round(colorNormalized.a*255.0f), 0, 255);
    }
    Color(const glm::vec3 &colorNormalized) {
        this->r = std::clamp((int)std::round(colorNormalized.r*255.0f), 0, 255);
        this->g = std::clamp((int)std::round(colorNormalized.g*255.0f), 0, 255);
        this->b = std::clamp((int)std::round(colorNormalized.b*255.0f), 0, 255);
        this->a = 255;
    }
    bool operator==(const Color &color) const { return r == color.r && g == color.g && b == color.b && a == color.a; }
    bool operator!=(const Color &color) const { return r != color.r || g != color.g || b != color.b || a != color.a; }

    [[nodiscard]] inline uint8_t getR() const { return r; }
    [[nodiscard]] inline uint8_t getG() const { return g; }
    [[nodiscard]] inline uint8_t getB() const { return b; }
    [[nodiscard]] inline uint8_t getA() const { return a; }
    [[nodiscard]] inline float getFloatR() const { return r/255.0f; }
    [[nodiscard]] inline float getFloatG() const { return g/255.0f; }
    [[nodiscard]] inline float getFloatB() const { return b/255.0f; }
    [[nodiscard]] inline float getFloatA() const { return a/255.0f; }
    [[nodiscard]] inline glm::vec3 getFloatColorRGB() const { return glm::vec3(r/255.0f, g/255.0f, b/255.0f); }
    [[nodiscard]] inline glm::vec4 getFloatColorRGBA() const { return glm::vec4(r/255.0f, g/255.0f, b/255.0f, a/255.0f); }
    [[nodiscard]] inline uint32_t getColorRGBA() const { uint32_t col = a; col <<= 8; col |= b; col <<= 8; col |= g; col <<= 8; col |= r; return col; }
    [[nodiscard]] inline uint32_t getColorRGB() const { uint32_t col = 255; col <<= 8; col |= b; col <<= 8; col |= g; col <<= 8; col |= r; return col; }

    inline void setR(uint8_t R) { r = R; }
    inline void setG(uint8_t G) { g = G; }
    inline void setB(uint8_t B) { b = B; }
    inline void setA(uint8_t A) { a = A; }
    inline void setFloatR(float R) { r = (uint8_t)(R*255.0f); }
    inline void setFloatG(float G) { g = (uint8_t)(G*255.0f); }
    inline void setFloatB(float B) { b = (uint8_t)(B*255.0f); }
    inline void setFloatA(float A) { a = (uint8_t)(A*255.0f); }
    void setColor(uint8_t R, uint8_t G, uint8_t B) { r = R; g = G; b = B; }
    void setColor(uint8_t R, uint8_t G, uint8_t B, uint8_t A) { r = R; g = G; b = B; a = A; }

private:
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

DLL_OBJECT Color colorFromHex(const std::string &hexColor);
DLL_OBJECT Color colorFromFloat(float R = 0.0f, float G = 0.0f, float B = 0.0f, float A = 1.0f);
DLL_OBJECT Color colorFromVec3(const glm::vec3 &vecColor);
DLL_OBJECT Color colorFromVec4(const glm::vec4 &vecColor);
DLL_OBJECT glm::vec3 colorToVec3(const Color &color);
DLL_OBJECT glm::vec4 colorToVec4(const Color &color);
/// 0 <= factor <= 1
DLL_OBJECT Color colorLerp(const Color &color1, const Color &color2, float factor);

//// 16-bit color
class DLL_OBJECT Color16 {
public:
    Color16(uint16_t R = 65535, uint16_t G = 65535, uint16_t B = 65535, uint16_t A = 65535) : r(R), g(G), b(B), a(A) {}
    Color16(const Color& c) {
        glm::vec4 colorFloat = c.getFloatColorRGBA();
        r = uint16_t(std::round(colorFloat.r * 65535));
        g = uint16_t(std::round(colorFloat.g * 65535));
        b = uint16_t(std::round(colorFloat.b * 65535));
        a = uint16_t(std::round(colorFloat.a * 65535));
    }
    Color16(const glm::vec4 &colorNormalized) {
        this->r = std::clamp((int)std::round(colorNormalized.r*65535.0f), 0, 65535);
        this->g = std::clamp((int)std::round(colorNormalized.g*65535.0f), 0, 65535);
        this->b = std::clamp((int)std::round(colorNormalized.b*65535.0f), 0, 65535);
        this->a = std::clamp((int)std::round(colorNormalized.a*65535.0f), 0, 65535);
    }
    Color16(const glm::vec3 &colorNormalized) {
        this->r = std::clamp((int)std::round(colorNormalized.r*65535.0f), 0, 65535);
        this->g = std::clamp((int)std::round(colorNormalized.g*65535.0f), 0, 65535);
        this->b = std::clamp((int)std::round(colorNormalized.b*65535.0f), 0, 65535);
        this->a = 65535;
    }
    bool operator==(const Color16 &color) const { return r == color.r && g == color.g && b == color.b && a == color.a; }
    bool operator!=(const Color16 &color) const { return r != color.r || g != color.g || b != color.b || a != color.a; }

    [[nodiscard]] inline uint16_t getR() const { return r; }
    [[nodiscard]] inline uint16_t getG() const { return g; }
    [[nodiscard]] inline uint16_t getB() const { return b; }
    [[nodiscard]] inline uint16_t getA() const { return a; }
    [[nodiscard]] inline float getFloatR() const { return r/65535.0f; }
    [[nodiscard]] inline float getFloatG() const { return g/65535.0f; }
    [[nodiscard]] inline float getFloatB() const { return b/65535.0f; }
    [[nodiscard]] inline float getFloatA() const { return a/65535.0f; }
    [[nodiscard]] inline glm::vec3 getFloatColorRGB() const { return glm::vec3(r/65535.0f, g/65535.0f, b/65535.0f); }
    [[nodiscard]] inline glm::vec4 getFloatColorRGBA() const { return glm::vec4(r/65535.0f, g/65535.0f, b/65535.0f, a/65535.0f); }
    [[nodiscard]] inline uint32_t getColorRGBA() const { uint32_t col = a; col <<= 8; col |= b; col <<= 8; col |= g; col <<= 8; col |= r; return col; }
    [[nodiscard]] inline uint32_t getColorRGB() const { uint32_t col = 65535; col <<= 8; col |= b; col <<= 8; col |= g; col <<= 8; col |= r; return col; }

    inline void setR(uint16_t R) { r = R; }
    inline void setG(uint16_t G) { g = G; }
    inline void setB(uint16_t B) { b = B; }
    inline void setA(uint16_t A) { a = A; }
    inline void setFloatR(float R) { r = (uint16_t)(R*65535.0f); }
    inline void setFloatG(float G) { g = (uint16_t)(G*65535.0f); }
    inline void setFloatB(float B) { b = (uint16_t)(B*65535.0f); }
    inline void setFloatA(float A) { a = (uint16_t)(A*65535.0f); }
    void setColor(uint16_t R, uint16_t G, uint16_t B) { r = R; g = G; b = B; }
    void setColor(uint16_t R, uint16_t G, uint16_t B, uint16_t A) { r = R; g = G; b = B; a = A; }
    void setColor(const Color& c) {
        glm::vec4 colorFloat = c.getFloatColorRGBA();
        r = uint16_t(std::round(colorFloat.r * 65535));
        g = uint16_t(std::round(colorFloat.g * 65535));
        b = uint16_t(std::round(colorFloat.b * 65535));
        a = uint16_t(std::round(colorFloat.a * 65535));
    }

private:
    uint16_t r;
    uint16_t g;
    uint16_t b;
    uint16_t a;
};

DLL_OBJECT Color16 color16FromFloat(float R = 0.0f, float G = 0.0f, float B = 0.0f, float A = 1.0f);
DLL_OBJECT Color16 color16FromVec3(const glm::vec3 &vecColor);
DLL_OBJECT Color16 color16FromVec4(const glm::vec4 &vecColor);
DLL_OBJECT glm::vec3 color16ToVec3(const Color16 &color);
DLL_OBJECT glm::vec4 color16ToVec4(const Color16 &color);
/// 0 <= factor <= 1
DLL_OBJECT Color16 color16Lerp(const Color16 &color1, const Color16 &color2, float factor);

}

/*! SRC_GRAPHICS_COLOR_HPP_ */
#endif
