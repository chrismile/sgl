/*
 * Color.cpp
 *
 *  Created on: 27.08.2017
 *      Author: Christoph Neuhauser
 */

#include "Color.hpp"
#include <cstring>
#include <Utils/Convert.hpp>
#include <Math/Math.hpp>

namespace sgl {

Color colorFromHex(const std::string &hexColor)
{
    Color color;
    std::string colorString = "";
    if (hexColor.length() == 6) {
        colorString = hexColor + "ff";
    } else {
        colorString = hexColor;
    }

    // The program runs on a Little Endian system - the least significant byte is stored first.
    // Therefore we have to swap the string from 0xRRGGBBAA to 0xAABBGGRR. -> Memory Alignment 0xRRGGBBAA
    colorString = colorString.substr(6, 2) + colorString.substr(4, 2) + colorString.substr(2, 2) + colorString.substr(0, 2);

    uint32_t colorValue = hexadecimalStringToUint32(colorString);
    memcpy((void*)&color, (void*)&colorValue, 4);
    return color;
}

Color colorFromFloat(float R /*= 0.0f*/, float G /*= 0.0f*/, float B /*= 0.0f*/, float A /*= 1.0f*/)
{
    return Color(R*255.0f, G*255.0f, B*255.0f, A*255.0f);
}

Color colorLerp(const Color &color1, const Color &color2, float factor)
{
    factor = clamp(factor, 0.0f, 1.0f);
    uint8_t r = interpolateLinear(color1.getFloatR(), color2.getFloatR(), factor)*255.0f;
    uint8_t g = interpolateLinear(color1.getFloatG(), color2.getFloatG(), factor)*255.0f;
    uint8_t b = interpolateLinear(color1.getFloatB(), color2.getFloatB(), factor)*255.0f;
    uint8_t a = interpolateLinear(color1.getFloatA(), color2.getFloatA(), factor)*255.0f;
    return Color(r, g, b, a);
}

}


