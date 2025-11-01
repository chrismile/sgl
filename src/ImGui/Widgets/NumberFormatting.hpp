/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Christoph Neuhauser
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

#ifndef SGL_NUMBERFORMATTING_HPP
#define SGL_NUMBERFORMATTING_HPP

#include <string>

namespace sgl {

/// Removes trailing zeros and unnecessary decimal points.
DLL_OBJECT std::string removeTrailingZeros(const std::string &numberString);

/// Removes decimal points if more than maxDigits digits are used.
DLL_OBJECT std::string getNiceNumberString(float number, int digits);

/// Same as above, but uses the best unit out of {B, KiB, MiB, GiB, TiB}.
DLL_OBJECT std::string getNiceMemoryString(uint64_t numBytes, int digits);

/// Same as above, but always rounds down in case results are not exact.
DLL_OBJECT std::string getNiceMemoryStringFloor(uint64_t numBytes, int digits);

/**
 * Same as the functions above, but tries to express memory amounts as differences of two power-of-two values if
 * possible (and the numbers would otherwise not be exactly representable at the selected digits). Examples:
 * - 4294967296 -> 4GiB
 * - 4294967296 -> 4GiB - 1B
 * - 4292870144 -> 4GiB - 2MiB
 * - 4292870120 -> 4GiB (no floor) or 3.99GiB (floor) for digits = 2
 * - 25769803776 -> 24GiB
 * @param numBytes The memory amount (in bytes) to convert to a nicely formatted string.
 * @param digits The number of digits to use for the individual values.
 * @param floor Whether to use @see getNiceMemoryStringFloor or @see getNiceMemoryString if a difference expression
 * is not possible.
 * @return A nicely formatted memory amount string.
 */
DLL_OBJECT std::string getNiceMemoryStringDifference(uint64_t numBytes, int digits, bool floor);

}

#endif //SGL_NUMBERFORMATTING_HPP
