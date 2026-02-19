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

#ifndef SGL_FORMATRANGE_HPP
#define SGL_FORMATRANGE_HPP

#include <cstdint>
#include <Graphics/Utils/FormatInfo.hpp>

struct format_range_base
{
    static constexpr int max_integer_value = 1;
    static constexpr int modulo_value = 1;
};

template<typename T>
struct format_range : public format_range_base
{
};

template<>
struct format_range<uint16_t>
{
    static constexpr int max_integer_value = 65536;
    static constexpr int modulo_value = 65521;
};

template<>
struct format_range<half>
{
    static constexpr int max_integer_value = 2048;
    static constexpr int modulo_value = 2039;
};

int getFormatRangeModuloValue(sgl::ChannelFormat channelFormat);

#endif //SGL_FORMATRANGE_HPP