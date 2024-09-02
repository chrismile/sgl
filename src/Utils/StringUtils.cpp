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

// ICU can be used, but for legacy reasons strings will be converted to/from std::string.
#ifdef USE_ICU
#include <unicode/unistr.h>
#include <unicode/ustream.h>
#include <unicode/locid.h>
#else
#include <locale>
#include <sstream>
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
#if U_SIZEOF_WCHAR_T == 2
    icu::UnicodeString unicodeStr(wcharStr);
#elif U_SIZEOF_WCHAR_T == 4
    auto unicodeStr = icu::UnicodeString::fromUTF32(
            reinterpret_cast<const UChar32*>(wcharStr), int32_t(std::wcslen(wcharStr)));
#else
#error "Error in wideStringArrayToStdString: Unsupported wchar_t format detected."
#endif
    std::string outString;
    unicodeStr.toUpper().toUTF8String(outString);
    return outString;
#else
    std::ostringstream stm;
    while(*wcharStr != L'\0') {
        stm << std::use_facet< std::ctype<wchar_t> >(std::locale()).narrow(*wcharStr++, '?');
    }
    return stm.str();
#endif
}

}
