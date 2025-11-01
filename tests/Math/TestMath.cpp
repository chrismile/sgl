/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2025, Christoph Neuhauser
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

#include <gtest/gtest.h>
#include <Math/Geometry/fallback/mat.hpp>
#include <Math/Geometry/fallback/linalg.hpp>

TEST(mat3Test, Inverse) {
    glm::mat3 m0 = glm::mat3(
        0.0f, 1.0f, 0.0f,
        2.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    );
    auto identity0 = glm::identity<glm::mat3>();
    auto identity1 = m0 * glm::inverse(m0);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            EXPECT_EQ(identity0[i][j], identity1[i][j]);
        }
    }
}

// TODO: Not yet implemented.
/*TEST(mat4Test, Inverse) {
    glm::mat4 m0 = glm::mat4(
        0.0f, 1.0f, 0.0f, 0.0f,
        2.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
    auto identity0 = glm::identity<glm::mat4>();
    auto identity1 = m0 * glm::inverse(m0);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            EXPECT_EQ(identity0[i][j], identity1[i][j]);
        }
    }
}*/

TEST(BuiltinBitMath, popcount32) {
    uint32_t testInputList[] = {
            0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 127u, 128u, 129u, 4294967295u
    };
    for (uint32_t inputValue : testInputList) {
        auto number = inputValue;
        number = number - ((number >> 1) & 0x55555555);
        number = (number & 0x33333333) + ((number >> 2) & 0x33333333);
        number = (number + (number >> 4)) & 0x0F0F0F0F;
        auto value = (number * 0x01010101) >> 24;
#if __cplusplus >= 201907L
        auto valueCxx20 = std::popcount(inputValue);
        EXPECT_EQ(value, valueCxx20);
#endif
#if defined(__GNUC__)
        auto valueGcc = __builtin_popcount(inputValue);
        EXPECT_EQ(value, valueGcc);
#endif
    }
}

TEST(BuiltinBitMath, popcount64) {
    uint64_t testInputList[] = {
            0ull, 1ull, 2ull, 3ull, 4ull, 5ull, 6ull, 7ull, 8ull, 127ull, 128ull, 129ull,
            4294967295ull, 4294967296ull, 4294967297ull, 17179869183ull, 17179869184ull, 17179869185ull
    };
    for (uint64_t inputValue : testInputList) {
        auto number = inputValue;
        number = number - ((number >> 1) & 0x5555555555555555);
        number = (number & 0x3333333333333333) + ((number >> 2) & 0x3333333333333333);
        number = (number + (number >> 4)) & 0x0F0F0F0F0F0F0F0F;
        auto value = (number * 0x0101010101010101) >> 56;
#if __cplusplus >= 201907L
        auto valueCxx20 = std::popcount(inputValue);
        EXPECT_EQ(value, valueCxx20);
#endif
#if defined(__GNUC__)
        auto valueGcc = __builtin_popcountll(inputValue);
        EXPECT_EQ(value, valueGcc);
#endif
    }
}

TEST(BuiltinBitMath, bit_width) {
    uint64_t testInputList[] = {
            0ull, 1ull, 2ull, 3ull, 4ull, 5ull, 6ull, 7ull, 8ull, 127ull, 128ull, 129ull,
            4294967295ull, 4294967296ull, 4294967297ull, 17179869183ull, 17179869184ull, 17179869185ull
    };
    for (uint64_t inputValue : testInputList) {
        auto number = inputValue;
        uint32_t bits;
        for (bits = 0u; number != 0ull; bits++) {
            number >>= 1ull;
        }
#if __cplusplus >= 201907L
        auto valueCxx20 = std::bit_width(inputValue);
        EXPECT_EQ(bits, valueCxx20);
#endif
#if defined(__GNUC__)
        auto valueGcc = inputValue == 0 ? 0 : 64 - __builtin_clzll(inputValue);
        EXPECT_EQ(bits, valueGcc);
#endif
    }
}

TEST(BuiltinBitMath, bit_ceil) {
    uint64_t testInputList[] = {
            0ull, 1ull, 2ull, 3ull, 4ull, 5ull, 6ull, 7ull, 8ull, 127ull, 128ull, 129ull,
            4294967295ull, 4294967296ull, 4294967297ull, 17179869183ull, 17179869184ull, 17179869185ull
    };
    for (uint64_t inputValue : testInputList) {
        uint64_t value;
        if (inputValue <= 1ull) {
            value = 1ull;
        } else {
            auto number = inputValue - 1ull;
            uint32_t bits;
            for (bits = 0u; number != 0ull; bits++) {
                number >>= 1ull;
            }
            value = 1ull << static_cast<uint64_t>(bits);
        }
#if __cplusplus >= 201907L
        auto valueCxx20 = std::bit_width(inputValue);
        EXPECT_EQ(value, valueCxx20);
#endif
#if defined(__GNUC__)
        auto valueGcc = inputValue <= 1 ? 1 : 1ull << (64 - __builtin_clzll(inputValue - 1));
        EXPECT_EQ(value, valueGcc);
#endif
    }
}
