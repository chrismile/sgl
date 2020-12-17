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

Color colorFromVec3(const glm::vec3 &vecColor)
{
    return colorFromFloat(vecColor.x, vecColor.y, vecColor.z, 1.0f);
}

Color colorFromVec4(const glm::vec4 &vecColor)
{
    return colorFromFloat(vecColor.x, vecColor.y, vecColor.z, vecColor.w);
}

glm::vec3 colorToVec3(const Color &color)
{
    return glm::vec3(color.getFloatR(), color.getFloatG(), color.getFloatB());
}

glm::vec4 colorToVec4(const Color &color)
{
    return glm::vec4(color.getFloatR(), color.getFloatG(), color.getFloatB(), color.getFloatA());
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

Color16 color16FromFloat(float R /*= 0.0f*/, float G /*= 0.0f*/, float B /*= 0.0f*/, float A /*= 1.0f*/)
{
    return Color16(R*65535.0f, G*65535.0f, B*65535.0f, A*65535.0f);
}

Color16 color16FromVec3(const glm::vec3 &vecColor)
{
    return colorFromFloat(vecColor.x, vecColor.y, vecColor.z, 1.0f);
}

Color16 color16FromVec4(const glm::vec4 &vecColor)
{
    return colorFromFloat(vecColor.x, vecColor.y, vecColor.z, vecColor.w);
}

glm::vec3 color16ToVec3(const Color16 &color)
{
    return glm::vec3(color.getFloatR(), color.getFloatG(), color.getFloatB());
}

glm::vec4 color16ToVec4(const Color16 &color)
{
    return glm::vec4(color.getFloatR(), color.getFloatG(), color.getFloatB(), color.getFloatA());
}

Color16 color16Lerp(const Color16 &color1, const Color16 &color2, float factor)
{
    factor = clamp(factor, 0.0f, 1.0f);
    uint16_t r = interpolateLinear(color1.getFloatR(), color2.getFloatR(), factor)*65535.0f;
    uint16_t g = interpolateLinear(color1.getFloatG(), color2.getFloatG(), factor)*65535.0f;
    uint16_t b = interpolateLinear(color1.getFloatB(), color2.getFloatB(), factor)*65535.0f;
    uint16_t a = interpolateLinear(color1.getFloatA(), color2.getFloatA(), factor)*65535.0f;
    return Color16(r, g, b, a);
}

}


