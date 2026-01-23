/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2026, Christoph Neuhauser
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
#include <Utils/Format.hpp>

#if __cplusplus >= 202002L

#include <format>

TEST(FormatTestStd, Default) {
    std::string formattedStringStd = std::format(
            "{} lies in {} and has more than {} shops {{}}", "Munich", "Germany", 20);
    std::string formattedString = sgl::formatString(
            "{} lies in {} and has more than {} shops {{}}", "Munich", "Germany", 20);
    EXPECT_EQ(formattedString, formattedStringStd);
}

TEST(FormatTestStd, Positional) {
    std::string formattedStringStd = std::format(
            "{} lies in {} and {} lies in {}", "Munich", "Germany", "Frankfurt", "Germany");
    std::string formattedString = sgl::formatStringPositional(
            "$1 lies in $0 and $2 lies in $0", "Germany", "Munich", "Frankfurt");
    EXPECT_EQ(formattedString, formattedStringStd);
}

#endif

TEST(FormatTest, Default) {
        std::string formattedStringStd = "Munich lies in Germany and has more than 20 shops {}";
        std::string formattedString = sgl::formatString(
                "{} lies in {} and has more than {} shops {{}}", "Munich", "Germany", 20);
        EXPECT_EQ(formattedString, formattedStringStd);
}

TEST(FormatTest, PositionalSimple) {
        std::string formattedStringStd = "Munich lies in Germany and Frankfurt lies in Germany";
        std::string formattedString = sgl::formatStringPositional(
                "$1 lies in $0 and $2 lies in $0", "Germany", "Munich", "Frankfurt");
        EXPECT_EQ(formattedString, formattedStringStd);
}

TEST(FormatTest, PositionalAdvanced) {
        std::string formattedStringStd = "Munich lies in Germany and Frankfurt lies in Germany $";
        std::string formattedString = sgl::formatStringPositional(
                "$1 lies in $0 and ${2} lies in $0 $$", "Germany", "Munich", "Frankfurt");
        EXPECT_EQ(formattedString, formattedStringStd);
}
