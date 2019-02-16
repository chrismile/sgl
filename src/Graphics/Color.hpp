/*!
 * Color.hpp
 *
 *  Created on: 27.08.2017
 *      Author: Christoph Neuhauser
 */

#ifndef SRC_GRAPHICS_COLOR_HPP_
#define SRC_GRAPHICS_COLOR_HPP_

#include <Utils/Convert.hpp>

namespace sgl {

class Color
{
public:
    Color (uint8_t R = 255, uint8_t G = 255, uint8_t B = 255, uint8_t A = 255) : r(R), g(G), b(B), a(A) {}
    bool operator==(const Color &color) const { return r == color.r && g == color.g && b == color.b && a == color.a; }
    bool operator!=(const Color &color) const { return r != color.r || g != color.g || b != color.b || a != color.a; }

    inline uint8_t getR() const { return r; }
    inline uint8_t getG() const { return g; }
    inline uint8_t getB() const { return b; }
    inline uint8_t getA() const { return a; }
    inline float getFloatR () const { return r/255.0f; }
    inline float getFloatG () const { return g/255.0f; }
    inline float getFloatB () const { return b/255.0f; }
    inline float getFloatA () const { return a/255.0f; }
    inline uint32_t getColorRGBA() const { uint32_t col = a; col <<= 8; col |= b; col <<= 8; col |= g; col <<= 8; col |= r; return col; }
    inline uint32_t getColorRGB() const { uint32_t col = 255; col <<= 8; col |= b; col <<= 8; col |= g; col <<= 8; col |= r; return col; }

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

Color colorFromHex(const std::string &hexColor);
Color colorFromFloat(float R = 0.0f, float G = 0.0f, float B = 0.0f, float A = 1.0f);
//! 0 <= factor <= 1
Color colorLerp(const Color &color1, const Color &color2, float factor);

}



/*! SRC_GRAPHICS_COLOR_HPP_ */
#endif
