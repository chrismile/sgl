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

#if !defined(USE_ICU) && !defined(_WIN32) && (!defined(__cplusplus) || __cplusplus < 201703L) && (!defined(_MSVC_LANG) || _MSVC_LANG < 201703L)
// codecvt is deprecated in C++17 and removed in C++26.
#define USE_CODECVT
#endif

// ICU can be used, but for legacy reasons strings will be converted to/from std::string.
#ifdef USE_ICU
#include <unicode/unistr.h>
#include <unicode/ustream.h>
#include <unicode/locid.h>
#include <unicode/ustring.h>
#elif defined(USE_CODECVT)
#include <locale>
#include <sstream>
#include <codecvt>
#else
#include <cstdint>
#include <cwchar>
#include <vector>
#endif

#if !defined(USE_ICU) && defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// Allow using boost::algorithm, as Boost may be built with ICU support for Unicode.
#ifdef USE_BOOST_ALGORITHM
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/replace.hpp>
#else
#include <algorithm>
#include <cctype>
#endif

#include "StringUtils.hpp"

namespace sgl {

#if !defined(USE_ICU) && !defined(_WIN32)
static bool utf8ToCodePoint(const uint8_t* utf8String, size_t utf8StringSize, uint32_t& codePoint, size_t& idx) {
    codePoint = 0;
    if (idx >= utf8StringSize) {
        return false;
    }
    uint8_t byte0 = utf8String[idx];
    if ((byte0 & 0x80u) == 0u) {
        idx += 1;
        codePoint |= byte0;
        return true;
    }
    if ((byte0 & 0x40u) == 0u) {
        // The two most significant bits must be set.
        return false;
    }
    uint8_t numBytes = 2;
    if ((byte0 & 0x20u) != 0u) {
        numBytes++;
        if ((byte0 & 0x10u) != 0u) {
            numBytes++;
        }
    }
    if ((byte0 & 1u << (7u - numBytes)) != 0u) {
        // There must be one zero bit.
        return false;
    }
    if (idx + numBytes > utf8StringSize) {
        return false;
    }
    codePoint |= static_cast<uint32_t>(static_cast<uint8_t>(byte0 << numBytes) >> numBytes) << ((numBytes - 1u) * 6u);
    for (uint8_t byteIdx = 1; byteIdx < numBytes; byteIdx++) {
        uint8_t byteI = utf8String[idx + byteIdx];
        codePoint |= (byteI & 0x3Fu) << ((numBytes - byteIdx - 1u) * 6u);
    }
    idx += numBytes;
    return true;
}

#if WCHAR_MAX <= 0xFFFFu
static bool utf16ToCodePoint(const uint16_t* utf16String, size_t utf16StringSize, uint32_t& codePoint, size_t& idx) {
    codePoint = 0;
    if (idx >= utf16StringSize) {
        return false;
    }
    uint16_t entry0 = utf16String[idx];
    if (entry0 <= 0xD7FFu || entry0 >= 0xE000u) {
        // Basic Multilingual Plane (BMP) code point.
        idx += 1;
        codePoint |= entry0;
        return true;
    }
    if (idx + 2 > utf16StringSize) {
        return false;
    }
    uint16_t entry1 = utf16String[idx + 1];
    codePoint += (entry0 - 0xD800u) << 10u;
    codePoint += entry1 - 0xDC00;
    codePoint += 0x10000u;
    idx += 2;
    return true;
}
#endif
#endif

bool startsWith(const std::string& str, const std::string& prefix) {
#if defined(USE_ICU)
    icu::UnicodeString unicodeStr(str.c_str(), "UTF-8");
    icu::UnicodeString unicodePrefix(prefix.c_str(), "UTF-8");
    return unicodeStr.startsWith(unicodePrefix);
#else
    return prefix.length() <= str.length() && std::equal(prefix.begin(), prefix.end(), str.begin());
#endif
}

bool endsWith(const std::string& str, const std::string& postfix) {
#if defined(USE_ICU)
    icu::UnicodeString unicodeStr(str.c_str(), "UTF-8");
    icu::UnicodeString unicodePostfix(postfix.c_str(), "UTF-8");
    return unicodeStr.endsWith(unicodePostfix);
#else
    return postfix.length() <= str.length() && std::equal(postfix.rbegin(), postfix.rend(), str.rbegin());
#endif
}

bool stringContains(const std::string& str, const std::string& substr) {
#ifdef USE_BOOST_ALGORITHM
    return boost::algorithm::contains(str, substr);
#else
    return str.find(substr) != std::string::npos;
#endif
}

void toUpper(std::string& str) {
#if defined(USE_ICU)
    icu::UnicodeString unicodeStr(str.c_str(), "UTF-8");
    str.clear();
    unicodeStr.toUpper().toUTF8String(str);
#elif defined(USE_BOOST_ALGORITHM)
    boost::to_upper(str);
#else
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::toupper(c); });
#endif
}

std::string toUpperCopy(const std::string& str) {
#if defined(USE_ICU)
    icu::UnicodeString unicodeStr(str.c_str(), "UTF-8");
    std::string outString;
    unicodeStr.toUpper().toUTF8String(outString);
    return outString;
#elif defined(USE_BOOST_ALGORITHM)
    return boost::to_upper_copy(str);
#else
    std::string strCopy = str;
    toUpper(strCopy);
    return strCopy;
#endif
}

void toLower(std::string& str) {
#if defined(USE_ICU)
    icu::UnicodeString unicodeStr(str.c_str(), "UTF-8");
    str.clear();
    unicodeStr.toLower().toUTF8String(str);
#elif defined(USE_BOOST_ALGORITHM)
    boost::to_lower(str);
#else
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c); });
#endif
}

std::string toLowerCopy(const std::string& str) {
#if defined(USE_ICU)
    icu::UnicodeString unicodeStr(str.c_str(), "UTF-8");
    std::string outString;
    unicodeStr.toLower().toUTF8String(outString);
    return outString;
#elif defined(USE_BOOST_ALGORITHM)
    return boost::to_lower_copy(str);
#else
    std::string strCopy = str;
    toLower(strCopy);
    return strCopy;
#endif
}

void stringTrim(std::string& str) {
#if defined(USE_ICU)
    icu::UnicodeString unicodeStr(str.c_str(), "UTF-8");
    str.clear();
    unicodeStr.trim().toUTF8String(str);
#elif defined(USE_BOOST_ALGORITHM)
    return boost::trim(str);
#else
    str = stringTrimCopy(str);
#endif
}

std::string stringTrimCopy(const std::string& str) {
#if defined(USE_ICU)
    icu::UnicodeString unicodeStr(str.c_str(), "UTF-8");
    std::string outString;
    unicodeStr.trim().toUTF8String(outString);
    return outString;
#elif defined(USE_BOOST_ALGORITHM)
    return boost::trim_copy(str);
#else
    int N = int(str.size());
    int start = 0, end = N - 1;
    while (start < N) {
        char c = str.at(start);
        if (c != ' ' && c != '\t') {
            break;
        }
        start++;
    }
    if (start == N) {
        return "";
    }
    while (end >= 0) {
        char c = str.at(end);
        if (c != ' ' && c != '\t') {
            break;
        }
        end--;
    }
    return str.substr(start, end - start + 1);
#endif
}

void stringReplaceAll(std::string& str, const std::string& searchPattern, const std::string& replStr) {
#if defined(USE_ICU)
    icu::UnicodeString unicodeStr(str.c_str(), "UTF-8");
    icu::UnicodeString unicodeSearchPattern(searchPattern.c_str(), "UTF-8");
    icu::UnicodeString unicodeReplStr(replStr.c_str(), "UTF-8");
    str.clear();
    unicodeStr.findAndReplace(unicodeSearchPattern, unicodeReplStr).toUTF8String(str);
#elif defined(USE_BOOST_ALGORITHM)
    boost::replace_all(str, searchPattern, replStr);
#else
    while (true) {
        auto start = str.find(searchPattern);
        if (start == std::string::npos) {
            break;
        }
        str = str.replace(start, searchPattern.length(), replStr);
    }
#endif
}

std::string stringReplaceAllCopy(
        const std::string& str, const std::string& searchPattern, const std::string& replStr) {
#if defined(USE_ICU)
    icu::UnicodeString unicodeStr(str.c_str(), "UTF-8");
    icu::UnicodeString unicodeSearchPattern(searchPattern.c_str(), "UTF-8");
    icu::UnicodeString unicodeReplStr(replStr.c_str(), "UTF-8");
    std::string outString;
    unicodeStr.findAndReplace(unicodeSearchPattern, unicodeReplStr).toUTF8String(outString);
    return outString;
#elif defined(USE_BOOST_ALGORITHM)
    return boost::replace_all_copy(str, searchPattern, replStr);
#else
    std::string processedStr;
    std::string::size_type startLast = 0;
    while (true) {
        auto start = str.find(searchPattern, startLast);
        if (start == std::string::npos) {
            processedStr += str.substr(startLast);
            break;
        }
        processedStr += str.substr(startLast, start - startLast);
        processedStr += replStr;
        startLast = start + searchPattern.length();
    }
    return processedStr;
#endif
}

std::string wideStringArrayToStdString(const wchar_t* wcharStr) {
#if defined(USE_ICU)
#  if U_SIZEOF_WCHAR_T == 2
    icu::UnicodeString unicodeStr(wcharStr);
#  elif U_SIZEOF_WCHAR_T == 4
    auto unicodeStr = icu::UnicodeString::fromUTF32(
            reinterpret_cast<const UChar32*>(wcharStr), int32_t(std::wcslen(wcharStr)));
#  else
#error "Error in wideStringArrayToStdString: Unsupported wchar_t format detected."
#  endif
    std::string outString;
    unicodeStr.toUTF8String(outString);
    return outString;
#elif defined(_WIN32)
    size_t wideStringLen = std::wcslen(wcharStr);
    if (wideStringLen == 0) {
        return {};
    }
    const int utf8StringSize = WideCharToMultiByte(
            CP_UTF8, 0, wcharStr, int(wideStringLen), NULL, 0, NULL, NULL);
    std::string utf8String(utf8StringSize, 0);
    WideCharToMultiByte                  (
            CP_UTF8, 0, wcharStr, int(wideStringLen), utf8String.data(), utf8StringSize, NULL, NULL);
    return utf8String;
#elif USE_CODECVT
    // Better solution below? Will be deprecated, though... (https://stackoverflow.com/questions/2573834/c-convert-string-or-char-to-wstring-or-wchar-t)
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(wcharStr);
    // Code block below does not handle wide chars.
    //std::ostringstream stm;
    //while(*wcharStr != L'\0') {
    //    stm << std::use_facet< std::ctype<wchar_t> >(std::locale()).narrow(*wcharStr++, '?');
    //}
    //return stm.str();
#else
    // Convert to UTF-8, see https://en.wikipedia.org/wiki/UTF-8.
    size_t wcharStrLen = std::wcslen(wcharStr);
    std::vector<uint8_t> utf8Buffer;
    utf8Buffer.reserve(wcharStrLen);
    for (size_t i = 0; i < wcharStrLen; ) {
#  if WCHAR_MAX > 0xFFFFu
        // Input is UTF-32, see https://en.wikipedia.org/wiki/UTF-32.
        auto codePoint = reinterpret_cast<const uint32_t*>(wcharStr)[i];
        i++;
#  else
        // Input is UTF-16, see https://en.wikipedia.org/wiki/UTF-32.
        uint32_t codePoint;
        if (!utf16ToCodePoint(reinterpret_cast<const uint16_t*>(wcharStr), wcharStrLen, codePoint, i)) {
            // Invalid UTF-16 stream.
            return {};
        }
#  endif
        if (codePoint < 0x80u) {
            utf8Buffer.push_back(static_cast<uint8_t>(codePoint));
        } else if (codePoint < 0x800u) {
            utf8Buffer.push_back(0xC0u | ((codePoint >> 6u) & 0x1Fu));
            utf8Buffer.push_back(0x80u | (codePoint & 0x3Fu));
        } else if (codePoint < 0x10000u) {
            utf8Buffer.push_back(0xE0u | ((codePoint >> 12u) & 0xFu));
            utf8Buffer.push_back(0x80u | ((codePoint >> 6u) & 0x3Fu));
            utf8Buffer.push_back(0x80u | (codePoint & 0x3Fu));
        } else {
            utf8Buffer.push_back(0xF0u | ((codePoint >> 18u) & 0x7u));
            utf8Buffer.push_back(0x80u | ((codePoint >> 12u) & 0x3Fu));
            utf8Buffer.push_back(0x80u | ((codePoint >> 6u) & 0x3Fu));
            utf8Buffer.push_back(0x80u | (codePoint & 0x3Fu));
        }
    }
    std::string utf8String(reinterpret_cast<char*>(utf8Buffer.data()), utf8Buffer.size());
    return utf8String;
#endif
}

std::wstring stdStringToWideString(const std::string& stdString) {
#if defined(USE_ICU)
    // See: https://stackoverflow.com/questions/61866124/convert-icu-unicode-string-to-stdwstring-or-wchar-t
    icu::UnicodeString unicodeStr(stdString.c_str(), "UTF-8");
    std::wstring wideString;
    int32_t wstringSize = 0;
    UErrorCode error = U_ZERO_ERROR;
    u_strToWCS(nullptr, 0, &wstringSize, unicodeStr.getBuffer(), unicodeStr.length(), &error);
    // For some reason, we get error == U_BUFFER_OVERFLOW_ERROR here, but the size looks fine...
    wideString.resize(wstringSize);
    error = U_ZERO_ERROR;
    u_strToWCS(wideString.data(), wstringSize, nullptr, unicodeStr.getBuffer(), unicodeStr.length(), &error);
    return wideString;
#elif defined(_WIN32)
    if (stdString.empty()) {
        return {};
    }
    int wstringSize = MultiByteToWideChar(CP_UTF8, 0, stdString.data(), int(stdString.size()), NULL, 0);
    std::wstring wstrString(wstringSize, 0);
    MultiByteToWideChar(CP_UTF8, 0, stdString.data(), int(stdString.size()), wstrString.data(), wstringSize);
    return wstrString;
#elif USE_CODECVT
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(stdString);
#else
    size_t charStrLen = stdString.size();
    std::vector<wchar_t> wcharBuffer;
    wcharBuffer.reserve(charStrLen);
    for (size_t i = 0; i < charStrLen; ) {
        // Input is UTF-8, see https://en.wikipedia.org/wiki/UTF-8.
        uint32_t codePoint;
        if (!utf8ToCodePoint(reinterpret_cast<const uint8_t*>(stdString.data()), charStrLen, codePoint, i)) {
            // Invalid UTF-8 stream.
            return {};
        }
#  if WCHAR_MAX > 0xFFFFu
        // Output is UTF-32, see https://en.wikipedia.org/wiki/UTF-32.
        wcharBuffer.push_back(static_cast<wchar_t>(codePoint));
#  else
        // Output is UTF-16, see https://en.wikipedia.org/wiki/UTF-32.
        if (codePoint < 0x10000u) {
            wcharBuffer.push_back(static_cast<wchar_t>(codePoint));
        } else {
            codePoint -= 0x10000u;
            wcharBuffer.push_back(static_cast<wchar_t>((codePoint >> 10u) + 0xD800u));
            wcharBuffer.push_back(static_cast<wchar_t>(((codePoint >> 10u) & 0x3FFu) + 0xDC00u));
        }
#  endif
    }
    std::wstring utf8String(wcharBuffer.data(), wcharBuffer.size());
    return utf8String;
#endif
}

}
