/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2017, Christoph Neuhauser
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

#ifndef SRC_UTILS_STRINGUTILS_HPP_
#define SRC_UTILS_STRINGUTILS_HPP_

#include <string>
#include <cstring>
#include <vector>

#include "Convert.hpp"

namespace sgl {

/**
 * Returns whether @str starts with @prefix.
 * @param str The full string.
 * @param prefix The prefix.
 * @return Returns whether the full string starts with the passed prefix.
 */
DLL_OBJECT bool startsWith(const std::string& str, const std::string& prefix);

/**
 * Returns whether @str ends with @postfix.
 * @param str The full string.
 * @param postfix The postfix.
 * @return Returns whether the full string starts with the passed postfix.
 */
DLL_OBJECT bool endsWith(const std::string& str, const std::string& postfix);

/**
 * Returns whether @str contains @substr.
 * @param str The full string.
 * @param substr The substring.
 * @return Returns whether the full string contains the passed substring.
 */
DLL_OBJECT bool stringContains(const std::string& str, const std::string& substr);

/**
 * Converts a string to upper case (in-place).
 * @param str The string to convert.
 */
DLL_OBJECT void toUpper(std::string& str);

/**
 * Converts a string to upper case and returns the new string.
 * @param str The string to convert.
 * @return The converted string.
 */
DLL_OBJECT std::string toUpperCopy(const std::string& str);

/**
 * Converts a string to lower case (in-place).
 * @param str The string to convert.
 */
DLL_OBJECT void toLower(std::string& str);

/**
 * Converts a string to lower case and returns the new string.
 * @param str The string to convert.
 * @return The converted string.
 */
DLL_OBJECT std::string toLowerCopy(const std::string& str);

/**
 * Converts a string to lower case and returns the new string.
 * @param str The string to convert.
 * @return The converted string.
 */
DLL_OBJECT std::string toLowerCopy(const std::string& str);

/**
 * Remove all leading and trailing from @str (in-place).
 * @param str The string to process.
 * @return The string without leading and trailing
 */
DLL_OBJECT void stringTrim(std::string& str);

/**
 * Remove all leading and trailing from @str.
 * @param str The string to process.
 * @return The string without leading and trailing
 */
DLL_OBJECT std::string stringTrimCopy(const std::string& str);

/**
 * Replaces all occurrences of @searchPattern with @replString in @str (in-place).
 * @param str The string to process.
 * @param searchPattern The search pattern.
 * @param replStr The string to replace the search pattern with.
 */
DLL_OBJECT void stringReplaceAll(std::string& str, const std::string& searchPattern, const std::string& replStr);

/**
 * Replaces all occurrences of @searchPattern with @replString in @str.
 * @param str The string to process.
 * @param searchPattern The search pattern.
 * @param replStr The string to replace the search pattern with.
 * @return The processed string.
 */
DLL_OBJECT std::string stringReplaceAllCopy(
        const std::string& str, const std::string& searchPattern, const std::string& replStr);

/**
 * Converts a wchar_t array to a std::string object.
 * If ICU is not available, wide characters are replaced by the ASCII character '?'.
 * @param wcharStr The wchar_t string array.
 * @return The converted string (UTF-8 or ASCII).
 */
DLL_OBJECT std::string wideStringArrayToStdString(const wchar_t* wcharStr);

/**
 * Converts strings like "This is a test!" with separator ' ' to { "This", "is", "a", "test!" }.
 * @tparam InputIterator The list class to use.
 * @param stringObject The string to split.
 * @param separator The separator to use for splitting.
 * @param listObject The split parts.
 */
template<class InputIterator>
void splitString(const std::string& stringObject, char separator, InputIterator& listObject) {
    std::string buffer;
    for (char c : stringObject) {
        if (c != separator) {
            buffer += c;
        } else {
            if (buffer.length() > 0) {
                listObject.push_back(buffer);
                buffer = "";
            }
        }
    }
    if (buffer.length() > 0) {
        listObject.push_back(buffer);
        buffer = "";
    }
}

/**
 * Converts strings like "This, is a test!" with separators ' ' and ',' to { "This", "is", "a", "test!" }.
 * @tparam InputIterator The list class to use.
 * @param stringObject The string to split.
 * @param s0 The first separator to use for splitting.
 * @param s1 The second separator to use for splitting.
 * @param listObject The split parts.
 */
template<class InputIterator>
void splitString2(const std::string& stringObject, InputIterator& listObject, char s0, char s1) {
    std::string buffer;
    for (char c : stringObject) {
        if (c != s0 && c != s1) {
            buffer += c;
        } else {
            if (buffer.length() > 0) {
                listObject.push_back(buffer);
                buffer = "";
            }
        }
    }
    if (buffer.length() > 0) {
        listObject.push_back(buffer);
        buffer = "";
    }
}

/**
 * Converts strings like "This is a test!" with separators ' ' and '\t' to { "This", "is", "a", "test!" }.
 * @tparam InputIterator The list class to use.
 * @param stringObject The string to split.
 * @param listObject The split parts.
 * NOTE: This is equivalent to
 * 'boost::algorithm::split(listObject, stringObject, boost::is_any_of("\t "), boost::token_compress_on);'.
 */
template<class InputIterator>
void splitStringWhitespace(const std::string& stringObject, InputIterator& listObject) {
    std::string buffer;
    for (char c : stringObject) {
        if (c != ' ' && c != '\t') {
            buffer += c;
        } else {
            if (buffer.length() > 0) {
                listObject.push_back(buffer);
                buffer = "";
            }
        }
    }
    if (buffer.length() > 0) {
        listObject.push_back(buffer);
        buffer = "";
    }
}

/**
 * Converts strings like "1 2 3" with separator ' ' to { 1, 2, 3 }.
 * @tparam InputIterator The list class to use.
 * @param stringObject The string to split.
 * @param separator The separator to use for splitting.
 * @param listObject The split parts.
 */
template<class T, class InputIterator>
void splitStringTyped(const std::string& stringObject, char separator, InputIterator& listObject) {
    std::string buffer;
    for (char c : stringObject) {
        if (c != separator) {
            buffer += c;
        } else {
            if (buffer.length() > 0) {
                listObject.push_back(sgl::fromString<T>(buffer));
                buffer = "";
            }
        }
    }
    if (buffer.length() > 0) {
        listObject.push_back(sgl::fromString<T>(buffer));
        buffer = "";
    }
}

/**
 * Converts strings like "1 2 3" with separators ' ' and '\t' to { 1, 2, 3 }.
 * @tparam InputIterator The list class to use.
 * @param stringObject The string to split.
 * @param listObject The split parts.
 * NOTE: This is equivalent to
 * 'boost::algorithm::split(listObject, stringObject, boost::is_any_of("\t "), boost::token_compress_on);'.
 */
template<class T, class InputIterator>
void splitStringWhitespaceTyped(const std::string& stringObject, InputIterator& listObject) {
    std::string buffer;
    for (char c : stringObject) {
        if (c != ' ' && c != '\t') {
            buffer += c;
        } else {
            if (buffer.length() > 0) {
                listObject.push_back(sgl::fromString<T>(buffer));
                buffer = "";
            }
        }
    }
    if (buffer.length() > 0) {
        listObject.push_back(sgl::fromString<T>(buffer));
        buffer = "";
    }
}

}


// https://stackoverflow.com/questions/56833000/c20-with-u8-char8-t-and-stdstring
#if defined(__cpp_char8_t)
template<typename T>
inline const char* u8Cpp20(T&& t) noexcept
{
#ifdef _MSC_VER
    #pragma warning (disable: 26490)
   return reinterpret_cast<const char*>(t);
#pragma warning (default: 26490)
#else // _MSC_VER
    return reinterpret_cast<const char*>(t);
#endif
}
#define U8(x) u8Cpp20(u8##x)
#else
#define U8(x) u8##x
#endif

/*! SRC_UTILS_STRINGUTILS_HPP_ */
#endif
