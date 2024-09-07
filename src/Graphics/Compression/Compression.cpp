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

#include <Math/Math.hpp>
#include <Utils/AppSettings.hpp>
#include <Utils/File/Logfile.hpp>

#ifdef SUPPORT_VULKAN
#include <Graphics/Vulkan/Image/Image.hpp>
#include <Graphics/Vulkan/Shader/ShaderManager.hpp>
#include <Graphics/Vulkan/Render/ComputePipeline.hpp>
#include <Graphics/Vulkan/Render/Data.hpp>
#include <Graphics/Vulkan/Render/Renderer.hpp>
#endif

#include "Compression.hpp"

namespace sgl {

DLL_OBJECT bool compressImageCpuBC6H(
        uint16_t* imageDataHalf, uint32_t imageWidth, uint32_t imageHeight,
        uint8_t*& imageDataOutCpu, size_t& imageDataOutputSizeInBytes
#ifdef SUPPORT_VULKAN
        , sgl::vk::Renderer* renderer
#endif
        ) {
#ifdef SUPPORT_VULKAN

    auto* device = sgl::AppSettings::get()->getPrimaryDevice();
    if (!device) {
        sgl::Logfile::get()->writeError(
                "Error in compressImageCpuBC6H: Currently only the Vulkan backend is supported, "
                "but no primary Vulkan device was created.");
        return false;
    }

    bool customRenderer = false;
    if (!renderer) {
        renderer = new sgl::vk::Renderer(device, 2);
        customRenderer = true;
    }

    VkCommandBuffer commandBuffer = device->beginSingleTimeCommands(
            device->getComputeQueueIndex(), false);
    renderer->setCustomCommandBuffer(commandBuffer, false);
    renderer->beginCommandBuffer();

    sgl::vk::ImageViewPtr compressedImage = compressImageVulkanBC6H(
            imageDataHalf, imageWidth, imageHeight, renderer, false);
    const auto& imageSettings = compressedImage->getImage()->getImageSettings();
    imageDataOutputSizeInBytes = sgl::uiceil(imageSettings.width, 4u) * sgl::uiceil(imageSettings.height, 4u) * 16;
    sgl::vk::BufferPtr stagingBuffer = std::make_shared<sgl::vk::Buffer>(
            device, imageDataOutputSizeInBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_TO_CPU);
    compressedImage->getImage()->copyToBuffer(stagingBuffer, commandBuffer);

    renderer->endCommandBuffer();
    renderer->resetCustomCommandBuffer();
    device->endSingleTimeCommands(commandBuffer, device->getComputeQueueIndex(), false);

    void* dataStagingBuffer = stagingBuffer->mapMemory();
    memcpy(imageDataOutCpu, dataStagingBuffer, imageDataOutputSizeInBytes);
    stagingBuffer->unmapMemory();

    if (customRenderer) {
        delete renderer;
    }
    return true;

#else

    sgl::Logfile::get()->writeError(
            "Error in compressImageCpuBC6H: Currently only the Vulkan backend is supported, but "
            "sgl was built without Vulkan support.");
    return false;

#endif
}

#ifdef SUPPORT_VULKAN

/*
 * The following shader code is an adapted version of the code from the Betsy compression library.
 * https://github.com/darksylinc/betsy
 *
 * The license of the Betsy compression library is as follows:
 *
 * Copyright 2020-2022 Matias N. Goldberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in theSoftware without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This software uses code from:
 *
 * * [GPURealTimeBC6H](https://github.com/knarkowicz/GPURealTimeBC6H),
 *   under public domain. Modifications by Matias N. Goldberg
 * * [rg-etc1](https://github.com/richgel999/rg-etc1/),
 *   Copyright (c) 2012 Rich Geldreich, zlib license. Extensive modifications by
 *   Matias N. Goldberg to adapt it as a compute shader
 * * [stb_dxt](https://github.com/nothings/stb/blob/master/stb_dxt.h), under dual-license: A. MIT License
 *   Copyright (c) 2017 Sean Barrett, B. Public Domain (www.unlicense.org). Original by fabian "ryg" giesen -
 *   ported to C by stb. Modifications by Matias N. Goldberg to adapt it as a compute shader
 * * EAC loosely inspired on [etc2_encoder](https://github.com/titilambert/packaging-efl/blob/master/src/static_libs/rg_etc/etc2_encoder.c),
 *   Copyright (C) 2014 Jean-Philippe ANDRE, 2-clause BSD license
 * * ETC2 T & H modes based on [etc2_encoder](https://github.com/titilambert/packaging-efl/blob/master/src/static_libs/rg_etc/etc2_encoder.c),
 *   Copyright (C) 2014 Jean-Philippe ANDRE, 2-clause BSD license. A couple minor bugfixes applied by Matias N. Goldberg.
 *   Modifications made by Matias N. Goldberg to adapt it as a compute shader
 * * ETC2 P very loosely based on [etc2_encoder](https://github.com/titilambert/packaging-efl/blob/master/src/static_libs/rg_etc/etc2_encoder.c), *
 */
const std::string& SHADER_STRING_COMPRESS_BC6H = R"(
#version 450 core

//#include "CrossPlatformSettings_piece_all.glsl"
#define min3( a, b, c ) min( a, min( b, c ) )
#define max3( a, b, c ) max( a, max( b, c ) )

#define float2 vec2
#define float3 vec3
#define float4 vec4

#define int2 ivec2
#define int3 ivec3
#define int4 ivec4

#define uint2 uvec2
#define uint3 uvec3
#define uint4 uvec4

#define float2x2 mat2
#define float3x3 mat3
#define float4x4 mat4

#define ushort uint
#define ushort3 uint3
#define ushort4 uint4

//Short used for read operations. It's an int in GLSL & HLSL. An ushort in Metal
#define rshort int
#define rshort2 int2
#define rint int
//Short used for write operations. It's an int in GLSL. An ushort in HLSL & Metal
#define wshort2 int2
#define wshort3 int3

#define toFloat3x3( x ) mat3( x )
#define buildFloat3x3( row0, row1, row2 ) mat3( row0, row1, row2 )

#define mul( x, y ) ((x) * (y))
#define saturate(x) clamp( (x), 0.0, 1.0 )
#define lerp mix
#define rsqrt inversesqrt
#define INLINE
#define NO_INTERPOLATION_PREFIX flat
#define NO_INTERPOLATION_SUFFIX

#define PARAMS_ARG_DECL
#define PARAMS_ARG

#define reversebits bitfieldReverse

#define GatherRed( tex, sampler, uv ) textureGather( tex, uv, 0 )
#define GatherGreen( tex, sampler, uv ) textureGather( tex, uv, 1 )
#define GatherBlue( tex, sampler, uv ) textureGather( tex, uv, 2 )

#define bufferFetch1( buffer, idx ) texelFetch( buffer, idx ).x

// End of CrossPlatformSettings_piece_all.glsl


#define QUALITY

//SIGNED macro is WIP
//#define SIGNED

float3 f32tof16( float3 value )
{
	return float3( packHalf2x16( float2( value.x, 0.0 ) ),
				   packHalf2x16( float2( value.y, 0.0 ) ),
				   packHalf2x16( float2( value.z, 0.0 ) ) );
}

float3 f16tof32( uint3 value )
{
	return float3( unpackHalf2x16( value.x ).x,
				   unpackHalf2x16( value.y ).x,
				   unpackHalf2x16( value.z ).x );
}

float f32tof16( float value )
{
	return packHalf2x16( float2( value.x, 0.0 ) );
}

float f16tof32( uint value )
{
	return unpackHalf2x16( value.x ).x;
}

layout(binding = 0) uniform sampler2D srcTexture;
layout(binding = 1, rgba32ui) uniform restrict writeonly uimage2D dstTexture;
layout(push_constant) uniform PushConstants {
    float2 p_textureSizeRcp;
};

const float HALF_MAX = 65504.0f;
const uint PATTERN_NUM = 32u;

float CalcMSLE( float3 a, float3 b )
{
	float3 err = log2( ( b + 1.0f ) / ( a + 1.0f ) );;
	err = err * err;
	return err.x + err.y + err.z;
}

uint PatternFixupID( uint i )
{
	uint ret = 15u;
	ret = ( ( 3441033216u >> i ) & 0x1u ) != 0 ? 2u : ret;
	ret = ( ( 845414400u  >> i ) & 0x1u ) != 0 ? 8u : ret;
	return ret;
}

uint Pattern( uint p, uint i )
{
	uint p2 = p / 2u;
	uint p3 = p - p2 * 2u;

	uint enc = 0u;
	enc = p2 == 0u  ? 2290666700u : enc;
	enc = p2 == 1u  ? 3972591342u : enc;
	enc = p2 == 2u  ? 4276930688u : enc;
	enc = p2 == 3u  ? 3967876808u : enc;
	enc = p2 == 4u  ? 4293707776u : enc;
	enc = p2 == 5u  ? 3892379264u : enc;
	enc = p2 == 6u  ? 4278255592u : enc;
	enc = p2 == 7u  ? 4026597360u : enc;
	enc = p2 == 8u  ? 9369360u    : enc;
	enc = p2 == 9u  ? 147747072u  : enc;
	enc = p2 == 10u ? 1930428556u : enc;
	enc = p2 == 11u ? 2362323200u : enc;
	enc = p2 == 12u ? 823134348u  : enc;
	enc = p2 == 13u ? 913073766u  : enc;
	enc = p2 == 14u ? 267393000u  : enc;
	enc = p2 == 15u ? 966553998u  : enc;

	enc = p3 != 0u ? enc >> 16u : enc;
	uint ret = ( enc >> i ) & 0x1u;
	return ret;
}

#ifndef SIGNED
//UF
float3 Quantize7( float3 x )
{
	return ( f32tof16( x ) * 128.0f ) / ( 0x7bff + 1.0f );
}

float3 Quantize9( float3 x )
{
	return ( f32tof16( x ) * 512.0f ) / ( 0x7bff + 1.0f );
}

float3 Quantize10( float3 x )
{
	return ( f32tof16( x ) * 1024.0f ) / ( 0x7bff + 1.0f );
}

float3 Unquantize7( float3 x )
{
	return ( x * 65536.0f + 0x8000 ) / 128.0f;
}

float3 Unquantize9( float3 x )
{
	return ( x * 65536.0f + 0x8000 ) / 512.0f;
}

float3 Unquantize10( float3 x )
{
	return ( x * 65536.0f + 0x8000 ) / 1024.0f;
}

float3 FinishUnquantize( float3 endpoint0Unq, float3 endpoint1Unq, float weight )
{
	float3 comp = ( endpoint0Unq * ( 64.0f - weight ) + endpoint1Unq * weight + 32.0f ) * ( 31.0f / 4096.0f );
	return f16tof32( uint3( comp ) );
}
#else
//SF

float3 cmpSign( float3 value )
{
	float3 signVal;
	signVal.x = value.x >= 0.0f ? 1.0f : -1.0f;
	signVal.y = value.y >= 0.0f ? 1.0f : -1.0f;
	signVal.z = value.z >= 0.0f ? 1.0f : -1.0f;
	return signVal;
}

float3 Quantize7( float3 x )
{
	float3 signVal = cmpSign( x );
	return signVal * ( f32tof16( abs( x ) ) * 64.0f ) / ( 0x7bff + 1.0f );
}

float3 Quantize9( float3 x )
{
	float3 signVal = cmpSign( x );
	return signVal * ( f32tof16( abs( x ) ) * 256.0f ) / ( 0x7bff + 1.0f );
}

float3 Quantize10( float3 x )
{
	float3 signVal = cmpSign( x );
	return signVal * ( f32tof16( abs( x ) ) * 512.0f ) / ( 0x7bff + 1.0f );
}

float3 Unquantize7( float3 x )
{
	float3 signVal = sign( x );
	x = abs( x );
	float3 finalVal = signVal * ( x * 32768.0f + 0x4000 ) / 64.0f;
	finalVal.x = x.x >= 64.0f ? 32767.0 : finalVal.x;
	finalVal.y = x.y >= 64.0f ? 32767.0 : finalVal.y;
	finalVal.z = x.z >= 64.0f ? 32767.0 : finalVal.z;
	return finalVal;
}

float3 Unquantize9( float3 x )
{
	float3 signVal = sign( x );
	x = abs( x );
	float3 finalVal = signVal * ( x * 32768.0f + 0x4000 ) / 256.0f;
	finalVal.x = x.x >= 256.0f ? 32767.0 : finalVal.x;
	finalVal.y = x.y >= 256.0f ? 32767.0 : finalVal.y;
	finalVal.z = x.z >= 256.0f ? 32767.0 : finalVal.z;
	return finalVal;
}

float3 Unquantize10( float3 x )
{
	float3 signVal = sign( x );
	x = abs( x );
	float3 finalVal = signVal * ( x * 32768.0f + 0x4000 ) / 512.0f;
	finalVal.x = x.x >= 512.0f ? 32767.0 : finalVal.x;
	finalVal.y = x.y >= 512.0f ? 32767.0 : finalVal.y;
	finalVal.z = x.z >= 512.0f ? 32767.0 : finalVal.z;
	return finalVal;
}

float3 FinishUnquantize( float3 endpoint0Unq, float3 endpoint1Unq, float weight )
{
	float3 comp = ( endpoint0Unq * ( 64.0f - weight ) + endpoint1Unq * weight + 32.0f ) * ( 31.0f / 2048.0f );
	/*float3 signVal;
	signVal.x = comp.x >= 0.0f ? 0.0f : 0x8000;
	signVal.y = comp.y >= 0.0f ? 0.0f : 0x8000;
	signVal.z = comp.z >= 0.0f ? 0.0f : 0x8000;*/
	//return f16tof32( uint3( signVal + abs( comp ) ) );
	return f16tof32( uint3( comp ) );
}
#endif

void Swap( inout float3 a, inout float3 b )
{
	float3 tmp = a;
	a = b;
	b = tmp;
}

void Swap( inout float a, inout float b )
{
	float tmp = a;
	a = b;
	b = tmp;
}

uint ComputeIndex3( float texelPos, float endPoint0Pos, float endPoint1Pos )
{
	float r = ( texelPos - endPoint0Pos ) / ( endPoint1Pos - endPoint0Pos );
	return uint( clamp( r * 6.98182f + 0.00909f + 0.5f, 0.0f, 7.0f ) );
}

uint ComputeIndex4( float texelPos, float endPoint0Pos, float endPoint1Pos )
{
	float r = ( texelPos - endPoint0Pos ) / ( endPoint1Pos - endPoint0Pos );
	return uint( clamp( r * 14.93333f + 0.03333f + 0.5f, 0.0f, 15.0f ) );
}

void SignExtend( inout float3 v1, uint mask, uint signFlag )
{
	int3 v = int3( v1 );
	v.x = ( v.x & int( mask ) ) | ( v.x < 0 ? int( signFlag ) : 0 );
	v.y = ( v.y & int( mask ) ) | ( v.y < 0 ? int( signFlag ) : 0 );
	v.z = ( v.z & int( mask ) ) | ( v.z < 0 ? int( signFlag ) : 0 );
	v1 = v;
}

void EncodeP1( inout uint4 block, inout float blockMSLE, float3 texels[ 16 ] )
{
	// compute endpoints (min/max RGB bbox)
	float3 blockMin = texels[ 0 ];
	float3 blockMax = texels[ 0 ];
	for ( uint i = 1u; i < 16u; ++i )
	{
		blockMin = min( blockMin, texels[ i ] );
		blockMax = max( blockMax, texels[ i ] );
	}


	// refine endpoints in log2 RGB space
	float3 refinedBlockMin = blockMax;
	float3 refinedBlockMax = blockMin;
	for ( uint i = 0u; i < 16u; ++i )
	{
		refinedBlockMin = min( refinedBlockMin, texels[ i ] == blockMin ? refinedBlockMin : texels[ i ] );
		refinedBlockMax = max( refinedBlockMax, texels[ i ] == blockMax ? refinedBlockMax : texels[ i ] );
	}

	float3 logBlockMax          = log2( blockMax + 1.0f );
	float3 logBlockMin          = log2( blockMin + 1.0f );
	float3 logRefinedBlockMax   = log2( refinedBlockMax + 1.0f );
	float3 logRefinedBlockMin   = log2( refinedBlockMin + 1.0f );
	float3 logBlockMaxExt       = ( logBlockMax - logBlockMin ) * ( 1.0f / 32.0f );
	logBlockMin += min( logRefinedBlockMin - logBlockMin, logBlockMaxExt );
	logBlockMax -= min( logBlockMax - logRefinedBlockMax, logBlockMaxExt );
	blockMin = exp2( logBlockMin ) - 1.0f;
	blockMax = exp2( logBlockMax ) - 1.0f;

	float3 blockDir = blockMax - blockMin;
	blockDir = blockDir / ( blockDir.x + blockDir.y + blockDir.z );

	float3 endpoint0    = Quantize10( blockMin );
	float3 endpoint1    = Quantize10( blockMax );
	float endPoint0Pos  = f32tof16( dot( blockMin, blockDir ) );
	float endPoint1Pos  = f32tof16( dot( blockMax, blockDir ) );


	// check if endpoint swap is required
	float fixupTexelPos = f32tof16( dot( texels[ 0 ], blockDir ) );
	uint fixupIndex = ComputeIndex4( fixupTexelPos, endPoint0Pos, endPoint1Pos );
	if ( fixupIndex > 7 )
	{
		Swap( endPoint0Pos, endPoint1Pos );
		Swap( endpoint0, endpoint1 );
	}

	// compute indices
	uint indices[ 16 ] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
	for ( uint i = 0u; i < 16u; ++i )
	{
		float texelPos = f32tof16( dot( texels[ i ], blockDir ) );
		indices[ i ] = ComputeIndex4( texelPos, endPoint0Pos, endPoint1Pos );
	}

	// compute compression error (MSLE)
	float3 endpoint0Unq = Unquantize10( endpoint0 );
	float3 endpoint1Unq = Unquantize10( endpoint1 );
	float msle = 0.0f;
	for ( uint i = 0u; i < 16u; ++i )
	{
		float weight = floor( ( indices[ i ] * 64.0f ) / 15.0f + 0.5f );
		float3 texelUnc = FinishUnquantize( endpoint0Unq, endpoint1Unq, weight );

		msle += CalcMSLE( texels[ i ], texelUnc );
	}


	// encode block for mode 11
	blockMSLE = msle;
	block.x = 0x03;

	// endpoints
	block.x |= uint( endpoint0.x ) << 5u;
	block.x |= uint( endpoint0.y ) << 15u;
	block.x |= uint( endpoint0.z ) << 25u;
	block.y |= uint( endpoint0.z ) >> 7u;
	block.y |= uint( endpoint1.x ) << 3u;
	block.y |= uint( endpoint1.y ) << 13u;
	block.y |= uint( endpoint1.z ) << 23u;
	block.z |= uint( endpoint1.z ) >> 9u;

	// indices
	block.z |= indices[ 0 ] << 1u;
	block.z |= indices[ 1 ] << 4u;
	block.z |= indices[ 2 ] << 8u;
	block.z |= indices[ 3 ] << 12u;
	block.z |= indices[ 4 ] << 16u;
	block.z |= indices[ 5 ] << 20u;
	block.z |= indices[ 6 ] << 24u;
	block.z |= indices[ 7 ] << 28u;
	block.w |= indices[ 8 ] << 0u;
	block.w |= indices[ 9 ] << 4u;
	block.w |= indices[ 10 ] << 8u;
	block.w |= indices[ 11 ] << 12u;
	block.w |= indices[ 12 ] << 16u;
	block.w |= indices[ 13 ] << 20u;
	block.w |= indices[ 14 ] << 24u;
	block.w |= indices[ 15 ] << 28u;
}

void EncodeP2Pattern( inout uint4 block, inout float blockMSLE, uint pattern, float3 texels[ 16 ] )
{
	float3 p0BlockMin = float3( HALF_MAX, HALF_MAX, HALF_MAX );
	float3 p0BlockMax = float3( 0.0f, 0.0f, 0.0f );
	float3 p1BlockMin = float3( HALF_MAX, HALF_MAX, HALF_MAX );
	float3 p1BlockMax = float3( 0.0f, 0.0f, 0.0f );

	for ( uint i = 0u; i < 16u; ++i )
	{
		uint paletteID = Pattern( pattern, i );
		if ( paletteID == 0 )
		{
			p0BlockMin = min( p0BlockMin, texels[ i ] );
			p0BlockMax = max( p0BlockMax, texels[ i ] );
		}
		else
		{
			p1BlockMin = min( p1BlockMin, texels[ i ] );
			p1BlockMax = max( p1BlockMax, texels[ i ] );
		}
	}

	float3 p0BlockDir = p0BlockMax - p0BlockMin;
	float3 p1BlockDir = p1BlockMax - p1BlockMin;
	p0BlockDir = p0BlockDir / ( p0BlockDir.x + p0BlockDir.y + p0BlockDir.z );
	p1BlockDir = p1BlockDir / ( p1BlockDir.x + p1BlockDir.y + p1BlockDir.z );


	float p0Endpoint0Pos = f32tof16( dot( p0BlockMin, p0BlockDir ) );
	float p0Endpoint1Pos = f32tof16( dot( p0BlockMax, p0BlockDir ) );
	float p1Endpoint0Pos = f32tof16( dot( p1BlockMin, p1BlockDir ) );
	float p1Endpoint1Pos = f32tof16( dot( p1BlockMax, p1BlockDir ) );


	uint fixupID = PatternFixupID( pattern );
	float p0FixupTexelPos = f32tof16( dot( texels[ 0 ], p0BlockDir ) );
	float p1FixupTexelPos = f32tof16( dot( texels[ fixupID ], p1BlockDir ) );
	uint p0FixupIndex = ComputeIndex3( p0FixupTexelPos, p0Endpoint0Pos, p0Endpoint1Pos );
	uint p1FixupIndex = ComputeIndex3( p1FixupTexelPos, p1Endpoint0Pos, p1Endpoint1Pos );
	if ( p0FixupIndex > 3u )
	{
		Swap( p0Endpoint0Pos, p0Endpoint1Pos );
		Swap( p0BlockMin, p0BlockMax );
	}
	if ( p1FixupIndex > 3u )
	{
		Swap( p1Endpoint0Pos, p1Endpoint1Pos );
		Swap( p1BlockMin, p1BlockMax );
	}

	uint indices[ 16 ] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u };
	for ( uint i = 0u; i < 16u; ++i )
	{
		float p0TexelPos = f32tof16( dot( texels[ i ], p0BlockDir ) );
		float p1TexelPos = f32tof16( dot( texels[ i ], p1BlockDir ) );
		uint p0Index = ComputeIndex3( p0TexelPos, p0Endpoint0Pos, p0Endpoint1Pos );
		uint p1Index = ComputeIndex3( p1TexelPos, p1Endpoint0Pos, p1Endpoint1Pos );

		uint paletteID = Pattern( pattern, i );
		indices[ i ] = paletteID == 0u ? p0Index : p1Index;
	}

	float3 endpoint760 = floor( Quantize7( p0BlockMin ) );
	float3 endpoint761 = floor( Quantize7( p0BlockMax ) );
	float3 endpoint762 = floor( Quantize7( p1BlockMin ) );
	float3 endpoint763 = floor( Quantize7( p1BlockMax ) );

	float3 endpoint950 = floor( Quantize9( p0BlockMin ) );
	float3 endpoint951 = floor( Quantize9( p0BlockMax ) );
	float3 endpoint952 = floor( Quantize9( p1BlockMin ) );
	float3 endpoint953 = floor( Quantize9( p1BlockMax ) );

	endpoint761 = endpoint761 - endpoint760;
	endpoint762 = endpoint762 - endpoint760;
	endpoint763 = endpoint763 - endpoint760;

	endpoint951 = endpoint951 - endpoint950;
	endpoint952 = endpoint952 - endpoint950;
	endpoint953 = endpoint953 - endpoint950;

	int maxVal76 = 0x1F;
	endpoint761 = clamp( endpoint761, -maxVal76, maxVal76 );
	endpoint762 = clamp( endpoint762, -maxVal76, maxVal76 );
	endpoint763 = clamp( endpoint763, -maxVal76, maxVal76 );

	int maxVal95 = 0xF;
	endpoint951 = clamp( endpoint951, -maxVal95, maxVal95 );
	endpoint952 = clamp( endpoint952, -maxVal95, maxVal95 );
	endpoint953 = clamp( endpoint953, -maxVal95, maxVal95 );

	float3 endpoint760Unq = Unquantize7( endpoint760 );
	float3 endpoint761Unq = Unquantize7( endpoint760 + endpoint761 );
	float3 endpoint762Unq = Unquantize7( endpoint760 + endpoint762 );
	float3 endpoint763Unq = Unquantize7( endpoint760 + endpoint763 );
	float3 endpoint950Unq = Unquantize9( endpoint950 );
	float3 endpoint951Unq = Unquantize9( endpoint950 + endpoint951 );
	float3 endpoint952Unq = Unquantize9( endpoint950 + endpoint952 );
	float3 endpoint953Unq = Unquantize9( endpoint950 + endpoint953 );

	float msle76 = 0.0f;
	float msle95 = 0.0f;
	for ( uint i = 0u; i < 16u; ++i )
	{
		uint paletteID = Pattern( pattern, i );

		float3 tmp760Unq = paletteID == 0u ? endpoint760Unq : endpoint762Unq;
		float3 tmp761Unq = paletteID == 0u ? endpoint761Unq : endpoint763Unq;
		float3 tmp950Unq = paletteID == 0u ? endpoint950Unq : endpoint952Unq;
		float3 tmp951Unq = paletteID == 0u ? endpoint951Unq : endpoint953Unq;

		float weight = floor( ( indices[ i ] * 64.0f ) / 7.0f + 0.5f );
		float3 texelUnc76 = FinishUnquantize( tmp760Unq, tmp761Unq, weight );
		float3 texelUnc95 = FinishUnquantize( tmp950Unq, tmp951Unq, weight );

		msle76 += CalcMSLE( texels[ i ], texelUnc76 );
		msle95 += CalcMSLE( texels[ i ], texelUnc95 );
	}

	SignExtend( endpoint761, 0x1F, 0x20 );
	SignExtend( endpoint762, 0x1F, 0x20 );
	SignExtend( endpoint763, 0x1F, 0x20 );

	SignExtend( endpoint951, 0xF, 0x10 );
	SignExtend( endpoint952, 0xF, 0x10 );
	SignExtend( endpoint953, 0xF, 0x10 );

	// encode block
	float p2MSLE = min( msle76, msle95 );
	if ( p2MSLE < blockMSLE )
	{
		blockMSLE   = p2MSLE;
		block       = uint4( 0u, 0u, 0u, 0u );

		if ( p2MSLE == msle76 )
		{
			// 7.6
			block.x = 0x1u;
			block.x |= ( uint( endpoint762.y ) & 0x20u ) >> 3u;
			block.x |= ( uint( endpoint763.y ) & 0x10u ) >> 1u;
			block.x |= ( uint( endpoint763.y ) & 0x20u ) >> 1u;
			block.x |= uint( endpoint760.x ) << 5u;
			block.x |= ( uint( endpoint763.z ) & 0x01u ) << 12u;
			block.x |= ( uint( endpoint763.z ) & 0x02u ) << 12u;
			block.x |= ( uint( endpoint762.z ) & 0x10u ) << 10u;
			block.x |= uint( endpoint760.y ) << 15u;
			block.x |= ( uint( endpoint762.z ) & 0x20u ) << 17u;
			block.x |= ( uint( endpoint763.z ) & 0x04u ) << 21u;
			block.x |= ( uint( endpoint762.y ) & 0x10u ) << 20u;
			block.x |= uint( endpoint760.z ) << 25u;
			block.y |= ( uint( endpoint763.z ) & 0x08u ) >> 3u;
			block.y |= ( uint( endpoint763.z ) & 0x20u ) >> 4u;
			block.y |= ( uint( endpoint763.z ) & 0x10u ) >> 2u;
			block.y |= uint( endpoint761.x ) << 3u;
			block.y |= ( uint( endpoint762.y ) & 0x0Fu ) << 9u;
			block.y |= uint( endpoint761.y ) << 13u;
			block.y |= ( uint( endpoint763.y ) & 0x0Fu ) << 19u;
			block.y |= uint( endpoint761.z ) << 23u;
			block.y |= ( uint( endpoint762.z ) & 0x07u ) << 29u;
			block.z |= ( uint( endpoint762.z ) & 0x08u ) >> 3u;
			block.z |= uint( endpoint762.x ) << 1u;
			block.z |= uint( endpoint763.x ) << 7u;
		}
		else
		{
			// 9.5
			block.x = 0xEu;
			block.x |= uint( endpoint950.x ) << 5u;
			block.x |= ( uint( endpoint952.z ) & 0x10u ) << 10u;
			block.x |= uint( endpoint950.y ) << 15u;
			block.x |= ( uint( endpoint952.y ) & 0x10u ) << 20u;
			block.x |= uint( endpoint950.z ) << 25u;
			block.y |= uint( endpoint950.z ) >> 7u;
			block.y |= ( uint( endpoint953.z ) & 0x10u ) >> 2u;
			block.y |= uint( endpoint951.x ) << 3u;
			block.y |= ( uint( endpoint953.y ) & 0x10u ) << 4u;
			block.y |= ( uint( endpoint952.y ) & 0x0Fu ) << 9u;
			block.y |= uint( endpoint951.y ) << 13u;
			block.y |= ( uint( endpoint953.z ) & 0x01u ) << 18u;
			block.y |= ( uint( endpoint953.y ) & 0x0Fu ) << 19u;
			block.y |= uint( endpoint951.z ) << 23u;
			block.y |= ( uint( endpoint953.z ) & 0x02u ) << 27u;
			block.y |= uint( endpoint952.z ) << 29u;
			block.z |= ( uint( endpoint952.z ) & 0x08u ) >> 3u;
			block.z |= uint( endpoint952.x ) << 1u;
			block.z |= ( uint( endpoint953.z ) & 0x04u ) << 4u;
			block.z |= uint( endpoint953.x ) << 7u;
			block.z |= ( uint( endpoint953.z ) & 0x08u ) << 9u;
		}

		block.z |= pattern << 13u;
		uint blockFixupID = PatternFixupID( pattern );
		if ( blockFixupID == 15u )
		{
			block.z |= indices[ 0 ] << 18u;
			block.z |= indices[ 1 ] << 20u;
			block.z |= indices[ 2 ] << 23u;
			block.z |= indices[ 3 ] << 26u;
			block.z |= indices[ 4 ] << 29u;
			block.w |= indices[ 5 ] << 0u;
			block.w |= indices[ 6 ] << 3u;
			block.w |= indices[ 7 ] << 6u;
			block.w |= indices[ 8 ] << 9u;
			block.w |= indices[ 9 ] << 12u;
			block.w |= indices[ 10 ] << 15u;
			block.w |= indices[ 11 ] << 18u;
			block.w |= indices[ 12 ] << 21u;
			block.w |= indices[ 13 ] << 24u;
			block.w |= indices[ 14 ] << 27u;
			block.w |= indices[ 15 ] << 30u;
		}
		else if ( blockFixupID == 2u )
		{
			block.z |= indices[ 0 ] << 18u;
			block.z |= indices[ 1 ] << 20u;
			block.z |= indices[ 2 ] << 23u;
			block.z |= indices[ 3 ] << 25u;
			block.z |= indices[ 4 ] << 28u;
			block.z |= indices[ 5 ] << 31u;
			block.w |= indices[ 5 ] >> 1u;
			block.w |= indices[ 6 ] << 2u;
			block.w |= indices[ 7 ] << 5u;
			block.w |= indices[ 8 ] << 8u;
			block.w |= indices[ 9 ] << 11u;
			block.w |= indices[ 10 ] << 14u;
			block.w |= indices[ 11 ] << 17u;
			block.w |= indices[ 12 ] << 20u;
			block.w |= indices[ 13 ] << 23u;
			block.w |= indices[ 14 ] << 26u;
			block.w |= indices[ 15 ] << 29u;
		}
		else
		{
			block.z |= indices[ 0 ] << 18u;
			block.z |= indices[ 1 ] << 20u;
			block.z |= indices[ 2 ] << 23u;
			block.z |= indices[ 3 ] << 26u;
			block.z |= indices[ 4 ] << 29u;
			block.w |= indices[ 5 ] << 0u;
			block.w |= indices[ 6 ] << 3u;
			block.w |= indices[ 7 ] << 6u;
			block.w |= indices[ 8 ] << 9u;
			block.w |= indices[ 9 ] << 11u;
			block.w |= indices[ 10 ] << 14u;
			block.w |= indices[ 11 ] << 17u;
			block.w |= indices[ 12 ] << 20u;
			block.w |= indices[ 13 ] << 23u;
			block.w |= indices[ 14 ] << 26u;
			block.w |= indices[ 15 ] << 29u;
		}
	}
}

layout( local_size_x = 8,
		local_size_y = 8,
		local_size_z = 1 ) in;

void main()
{
    //float2 p_textureSizeRcp = 1.0 / float2(textureSize(srcTexture, 0));
	// gather texels for current 4x4 block
	// 0 1 2 3
	// 4 5 6 7
	// 8 9 10 11
	// 12 13 14 15
	float2 uv       = gl_GlobalInvocationID.xy * p_textureSizeRcp * 4.0f - p_textureSizeRcp;
	float2 block0UV = uv;
	float2 block1UV = uv + float2( 2.0f * p_textureSizeRcp.x, 0.0f );
	float2 block2UV = uv + float2( 0.0f, 2.0f * p_textureSizeRcp.y );
	float2 block3UV = uv + float2( 2.0f * p_textureSizeRcp.x, 2.0f * p_textureSizeRcp.y );
	float4 block0X  = GatherRed( srcTexture, pointSampler, block0UV );
	float4 block1X  = GatherRed( srcTexture, pointSampler, block1UV );
	float4 block2X  = GatherRed( srcTexture, pointSampler, block2UV );
	float4 block3X  = GatherRed( srcTexture, pointSampler, block3UV );
	float4 block0Y  = GatherGreen( srcTexture, pointSampler, block0UV );
	float4 block1Y  = GatherGreen( srcTexture, pointSampler, block1UV );
	float4 block2Y  = GatherGreen( srcTexture, pointSampler, block2UV );
	float4 block3Y  = GatherGreen( srcTexture, pointSampler, block3UV );
	float4 block0Z  = GatherBlue( srcTexture, pointSampler, block0UV );
	float4 block1Z  = GatherBlue( srcTexture, pointSampler, block1UV );
	float4 block2Z  = GatherBlue( srcTexture, pointSampler, block2UV );
	float4 block3Z  = GatherBlue( srcTexture, pointSampler, block3UV );

	float3 texels[ 16 ];
	texels[ 0 ]     = float3( block0X.w, block0Y.w, block0Z.w );
	texels[ 1 ]     = float3( block0X.z, block0Y.z, block0Z.z );
	texels[ 2 ]     = float3( block1X.w, block1Y.w, block1Z.w );
	texels[ 3 ]     = float3( block1X.z, block1Y.z, block1Z.z );
	texels[ 4 ]     = float3( block0X.x, block0Y.x, block0Z.x );
	texels[ 5 ]     = float3( block0X.y, block0Y.y, block0Z.y );
	texels[ 6 ]     = float3( block1X.x, block1Y.x, block1Z.x );
	texels[ 7 ]     = float3( block1X.y, block1Y.y, block1Z.y );
	texels[ 8 ]     = float3( block2X.w, block2Y.w, block2Z.w );
	texels[ 9 ]     = float3( block2X.z, block2Y.z, block2Z.z );
	texels[ 10 ]    = float3( block3X.w, block3Y.w, block3Z.w );
	texels[ 11 ]    = float3( block3X.z, block3Y.z, block3Z.z );
	texels[ 12 ]    = float3( block2X.x, block2Y.x, block2Z.x );
	texels[ 13 ]    = float3( block2X.y, block2Y.y, block2Z.y );
	texels[ 14 ]    = float3( block3X.x, block3Y.x, block3Z.x );
	texels[ 15 ]    = float3( block3X.y, block3Y.y, block3Z.y );

	uint4 block     = uint4( 0u, 0u, 0u, 0u );
	float blockMSLE = 0.0f;

	EncodeP1( block, blockMSLE, texels );
#ifdef QUALITY
	for ( uint i = 0u; i < 32u; ++i )
	{
		EncodeP2Pattern( block, blockMSLE, i, texels );
	}
#endif

	imageStore( dstTexture, int2( gl_GlobalInvocationID.xy ), block );
}
)";

sgl::vk::ImageViewPtr compressImageVulkanBC6H(
        uint16_t* imageDataHalf, uint32_t imageWidth, uint32_t imageHeight,
        sgl::vk::Renderer* renderer, bool runSync) {
    auto* device = sgl::AppSettings::get()->getPrimaryDevice();

    auto shaderStages = sgl::vk::ShaderManager->compileComputeShaderFromStringCached(
            "CompressBC6H.Compute", SHADER_STRING_COMPRESS_BC6H);

    sgl::vk::ImageSettings imageSettings{};
    imageSettings.width = imageWidth;
    imageSettings.height = imageHeight;
    imageSettings.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    imageSettings.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    sgl::vk::ImageSamplerSettings imageSamplerSettings{};
    sgl::vk::TexturePtr srcTexture = std::make_shared<sgl::vk::Texture>(
            std::make_shared<sgl::vk::ImageView>(std::make_shared<sgl::vk::Image>(device, imageSettings)),
            imageSamplerSettings);

    /*
     * According to https://vulkan.lunarg.com/doc/view/1.3.261.0/linux/1.3-extensions/vkspec.html#formats-compatibility,
     * VK_FORMAT_BC6H_UFLOAT_BLOCK is not aliasing-compatible with VK_FORMAT_R32G32B32A32_UINT.
     * So we need to copy VK_FORMAT_R32G32B32A32_UINT to VK_FORMAT_BC6H_UFLOAT_BLOCK.
     */
    imageSettings.usage =
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageSettings.format = VK_FORMAT_BC6H_UFLOAT_BLOCK;
    sgl::vk::ImageViewPtr dstImageViewBC6H = std::make_shared<sgl::vk::ImageView>(
            std::make_shared<sgl::vk::Image>(device, imageSettings));

    imageSettings.width = sgl::uiceil(imageWidth, 4u);
    imageSettings.height = sgl::uiceil(imageHeight, 4u);
    imageSettings.format = VK_FORMAT_R32G32B32A32_UINT;
    imageSettings.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    sgl::vk::ImageViewPtr dstImageViewUint = std::make_shared<sgl::vk::ImageView>(
            std::make_shared<sgl::vk::Image>(device, imageSettings));

    sgl::vk::ComputePipelineInfo computePipelineInfo(shaderStages);
    sgl::vk::ComputePipelinePtr computePipeline(new sgl::vk::ComputePipeline(device, computePipelineInfo));
    auto computeData = std::make_shared<sgl::vk::ComputeData>(renderer, computePipeline);
    computeData->setStaticTexture(srcTexture, 0);
    computeData->setStaticImageView(dstImageViewUint, 1);

    VkCommandBuffer commandBuffer = nullptr;
    if (runSync) {
        commandBuffer = device->beginSingleTimeCommands(
                device->getComputeQueueIndex(), false);
        renderer->setCustomCommandBuffer(commandBuffer, false);
        renderer->beginCommandBuffer();
    }

    srcTexture->getImage()->uploadData(imageWidth * imageHeight * 8, imageDataHalf, renderer->getVkCommandBuffer());

    renderer->insertImageMemoryBarrier(
            dstImageViewUint->getImage(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_ACCESS_NONE_KHR, VK_ACCESS_SHADER_WRITE_BIT);
    glm::vec2 invTextureSize(1.0f / float(imageWidth), 1.0f / float(imageHeight));
    renderer->pushConstants(computeData->getComputePipeline(), VK_SHADER_STAGE_COMPUTE_BIT, 0, invTextureSize);
    renderer->dispatch(computeData, sgl::uiceil(imageWidth, 32u), sgl::uiceil(imageHeight, 32u), 1);

    renderer->insertImageMemoryBarrier(
            dstImageViewUint->getImage(),
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT);
    renderer->insertImageMemoryBarrier(
            dstImageViewBC6H->getImage(),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_ACCESS_NONE_KHR, VK_ACCESS_TRANSFER_WRITE_BIT);
    dstImageViewUint->getImage()->copyToImage(
            dstImageViewBC6H->getImage(), VK_IMAGE_ASPECT_COLOR_BIT, renderer->getVkCommandBuffer());

    if (runSync) {
        renderer->endCommandBuffer();
        renderer->resetCustomCommandBuffer();
        device->endSingleTimeCommands(commandBuffer, device->getComputeQueueIndex(), false);
    }

    return dstImageViewBC6H;
}

#endif

}
