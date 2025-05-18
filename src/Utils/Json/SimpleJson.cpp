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

#include <stdexcept>
#include <fstream>
#include <optional>
#include <cstring>

#include <Utils/Convert.hpp>
#include <Utils/StringUtils.hpp>
#include <Utils/File/FileLoader.hpp>
#include "SimpleJson.hpp"

namespace sgl {

JsonValue::JsonValue() : JsonValue(JsonValueType::NULL_VALUE) {}

JsonValue::JsonValue(JsonValueType valueType) : valueType(valueType) {
    if (valueType == JsonValueType::STRING_VALUE) {
        data.stringValue = new std::string;
    } else if (valueType == JsonValueType::ARRAY_VALUE) {
        data.arrayValues = new std::vector<JsonValue>;
    } else if (valueType == JsonValueType::OBJECT_VALUE) {
        data.mapValues = new std::map<std::string, JsonValue>;
    } else {
        memset(&data, 0, sizeof(data));
    }
}

JsonValue::JsonValue(const JsonValue& other) : valueType(other.valueType) {
    if (valueType == JsonValueType::STRING_VALUE) {
        data.stringValue = new std::string;
        *data.stringValue = *other.data.stringValue;
    } else if (valueType == JsonValueType::ARRAY_VALUE) {
        data.arrayValues = new std::vector<JsonValue>;
        *data.arrayValues = *other.data.arrayValues;
    } else if (valueType == JsonValueType::OBJECT_VALUE) {
        data.mapValues = new std::map<std::string, JsonValue>;
        *data.mapValues = *other.data.mapValues;
    } else {
        data = other.data;
    }
}

JsonValue::JsonValue(JsonValue&& other) noexcept {
    valueType = JsonValueType::NULL_VALUE;
    std::swap(valueType, other.valueType);
    std::swap(data, other.data);
}

void JsonValue::deleteData() {
    if (valueType == JsonValueType::STRING_VALUE && data.stringValue) {
        delete data.stringValue;
    }
    if (valueType == JsonValueType::ARRAY_VALUE && data.arrayValues) {
        delete data.arrayValues;
    }
    if (valueType == JsonValueType::OBJECT_VALUE && data.mapValues) {
        delete data.mapValues;
    }
    valueType = JsonValueType::NULL_VALUE;
    memset(&data, 0, sizeof(data));
}

JsonValue::~JsonValue() {
    deleteData();
}

JsonValue& JsonValue::operator=(const JsonValue& other) {
    deleteData();
    valueType = other.valueType;
    if (valueType == JsonValueType::STRING_VALUE) {
        data.stringValue = new std::string;
        *data.stringValue = *other.data.stringValue;
    } else if (valueType == JsonValueType::ARRAY_VALUE) {
        data.arrayValues = new std::vector<JsonValue>;
        *data.arrayValues = *other.data.arrayValues;
    } else if (valueType == JsonValueType::OBJECT_VALUE) {
        data.mapValues = new std::map<std::string, JsonValue>;
        *data.mapValues = *other.data.mapValues;
    } else {
        data = other.data;
    }
    return *this;
}

JsonValue& JsonValue::operator=(int32_t intObj) {
    if (valueType != JsonValueType::INT_VALUE) {
        deleteData();
        valueType = JsonValueType::INT_VALUE;
    }
    data.intValue = int64_t(intObj);
    return *this;
}

JsonValue& JsonValue::operator=(int64_t intObj) {
    if (valueType != JsonValueType::INT_VALUE) {
        deleteData();
        valueType = JsonValueType::INT_VALUE;
    }
    data.intValue = intObj;
    return *this;
}

JsonValue& JsonValue::operator=(uint32_t intObj) {
    if (valueType != JsonValueType::UINT_VALUE) {
        deleteData();
        valueType = JsonValueType::UINT_VALUE;
    }
    data.uintValue = uint64_t(intObj);
    return *this;
}

JsonValue& JsonValue::operator=(uint64_t intObj) {
    if (valueType != JsonValueType::UINT_VALUE) {
        deleteData();
        valueType = JsonValueType::UINT_VALUE;
    }
    data.uintValue = intObj;
    return *this;
}

JsonValue& JsonValue::operator=(float realObj) {
    if (valueType != JsonValueType::REAL_VALUE) {
        deleteData();
        valueType = JsonValueType::REAL_VALUE;
    }
    data.realValue = double(realObj);
    return *this;
}

JsonValue& JsonValue::operator=(double realObj) {
    if (valueType != JsonValueType::REAL_VALUE) {
        deleteData();
        valueType = JsonValueType::REAL_VALUE;
    }
    data.realValue = realObj;
    return *this;
}

JsonValue& JsonValue::operator=(bool realObj) {
    if (valueType != JsonValueType::BOOLEAN_VALUE) {
        deleteData();
        valueType = JsonValueType::BOOLEAN_VALUE;
    }
    data.intValue = int64_t(realObj);
    return *this;
}

JsonValue& JsonValue::operator=(const std::string& stringObj) {
    if (valueType != JsonValueType::STRING_VALUE) {
        deleteData();
        valueType = JsonValueType::STRING_VALUE;
        data.stringValue = new std::string;
    }
    *data.stringValue = stringObj;
    return *this;
}

JsonValue& JsonValue::operator=(const char* stringObj) {
    if (valueType != JsonValueType::STRING_VALUE) {
        deleteData();
        valueType = JsonValueType::STRING_VALUE;
        data.stringValue = new std::string;
    }
    *data.stringValue = stringObj;
    return *this;
}

JsonValue& JsonValue::operator[](size_t index) {
    if (valueType == JsonValueType::NULL_VALUE) {
        valueType = JsonValueType::ARRAY_VALUE;
        data.arrayValues = new std::vector<JsonValue>;
    }
    if (valueType != JsonValueType::ARRAY_VALUE) {
        throw std::runtime_error("Error in JsonValue::operator[](int64_t): Value type is not array.");
    }
    if (index >= data.arrayValues->size()) {
        data.arrayValues->resize(index + 1);
    }
    return (*data.arrayValues)[index];
}

const JsonValue& JsonValue::operator[](size_t index) const {
    if (valueType != JsonValueType::ARRAY_VALUE) {
        throw std::runtime_error("Error in JsonValue::operator[](int64_t) const: Value type is not array.");
    }
    if (index >= data.arrayValues->size()) {
        throw std::out_of_range("Error in JsonValue::operator[](int64_t) const: Array index out of range.");
    }
    return (*data.arrayValues)[index];
}

size_t JsonValue::size() const {
    if (valueType != JsonValueType::ARRAY_VALUE) {
        throw std::runtime_error("Error in JsonValue::size() const: Value type is not array.");
    }
    return data.arrayValues->size();
}

JsonValue& JsonValue::operator[](const std::string& key) {
    if (valueType == JsonValueType::NULL_VALUE) {
        valueType = JsonValueType::OBJECT_VALUE;
        data.mapValues = new std::map<std::string, JsonValue>;
    }
    if (valueType != JsonValueType::OBJECT_VALUE) {
        throw std::runtime_error("Error in JsonValue::operator[](const std::string&): Value type is not object.");
    }
    return (*data.mapValues)[key];
}

const JsonValue& JsonValue::operator[](const std::string& key) const {
    if (valueType != JsonValueType::OBJECT_VALUE) {
        throw std::runtime_error("Error in JsonValue::operator[](const std::string&) const: Value type is not object.");
    }
    const auto it = data.mapValues->find(key);
    if (it == data.mapValues->cend()) {
        throw std::out_of_range("Error in JsonValue::operator[](const std::string&) const: Key not found.");
    }
    return it->second;
}

bool JsonValue::hasMember(const std::string& key) const {
    if (valueType != JsonValueType::OBJECT_VALUE) {
        throw std::runtime_error("Error in JsonValue::hasMember(const std::string&) const: Value type is not object.");
    }
    const auto it = data.mapValues->find(key);
    return it != data.mapValues->cend();
}

std::map<std::string, JsonValue>::iterator JsonValue::begin() {
    if (valueType != JsonValueType::OBJECT_VALUE) {
        throw std::runtime_error("Error in JsonValue::begin(): Value type is not object.");
    }
    return data.mapValues->begin();
}

std::map<std::string, JsonValue>::iterator JsonValue::end() {
    if (valueType != JsonValueType::OBJECT_VALUE) {
        throw std::runtime_error("Error in JsonValue::end(): Value type is not object.");
    }
    return data.mapValues->end();
}

std::map<std::string, JsonValue>::const_iterator JsonValue::begin() const {
    if (valueType != JsonValueType::OBJECT_VALUE) {
        throw std::runtime_error("Error in JsonValue::begin() const: Value type is not object.");
    }
    return data.mapValues->cbegin();
}

std::map<std::string, JsonValue>::const_iterator JsonValue::end() const {
    if (valueType != JsonValueType::OBJECT_VALUE) {
        throw std::runtime_error("Error in JsonValue::end() const: Value type is not object.");
    }
    return data.mapValues->cend();
}

std::map<std::string, JsonValue>::const_iterator JsonValue::cbegin() const {
    if (valueType != JsonValueType::OBJECT_VALUE) {
        throw std::runtime_error("Error in JsonValue::cbegin() const: Value type is not object.");
    }
    return data.mapValues->cbegin();
}

std::map<std::string, JsonValue>::const_iterator JsonValue::cend() const {
    if (valueType != JsonValueType::OBJECT_VALUE) {
        throw std::runtime_error("Error in JsonValue::cend() const: Value type is not object.");
    }
    return data.mapValues->cend();
}

void JsonValue::erase(const std::string& key) {
    if (valueType != JsonValueType::OBJECT_VALUE) {
        throw std::runtime_error("Error in JsonValue::erase(): Value type is not object.");
    }
    data.mapValues->erase(key);
}

int32_t JsonValue::asInt32() const {
    return int32_t(asInt64());
}

int64_t JsonValue::asInt64() const {
    if (valueType == JsonValueType::INT_VALUE || valueType == JsonValueType::UINT_VALUE
            || valueType == JsonValueType::BOOLEAN_VALUE) {
        return data.intValue;
    } else if (valueType == JsonValueType::NULL_VALUE) {
        return 0;
    } else if (valueType == JsonValueType::REAL_VALUE) {
        return int64_t(data.realValue);
    } else if (valueType == JsonValueType::STRING_VALUE) {
        return sgl::fromString<int64_t>(*data.stringValue);
    }
    throw std::runtime_error("Error in JsonValue::asInt64(): Value type is not compatible.");
    return 0;
}

uint32_t JsonValue::asUint32() const {
    return uint32_t(asUint64());
}

uint64_t JsonValue::asUint64() const {
    if (valueType == JsonValueType::INT_VALUE || valueType == JsonValueType::UINT_VALUE
            || valueType == JsonValueType::BOOLEAN_VALUE) {
        return data.uintValue;
    } else if (valueType == JsonValueType::NULL_VALUE) {
        return 0;
    } else if (valueType == JsonValueType::REAL_VALUE) {
        return int64_t(data.realValue);
    } else if (valueType == JsonValueType::STRING_VALUE) {
        return sgl::fromString<int64_t>(*data.stringValue);
    }
    throw std::runtime_error("Error in JsonValue::asUint64(): Value type is not compatible.");
    return 0;
}

float JsonValue::asFloat() const {
    if (valueType == JsonValueType::REAL_VALUE) {
        return float(data.realValue);
    } else if (valueType == JsonValueType::NULL_VALUE) {
        return 0.0f;
    } else if (valueType == JsonValueType::INT_VALUE || valueType == JsonValueType::BOOLEAN_VALUE) {
        return float(data.intValue);
    } else if (valueType == JsonValueType::UINT_VALUE) {
        return float(data.uintValue);
    } else if (valueType == JsonValueType::STRING_VALUE) {
        return sgl::fromString<float>(*data.stringValue);
    }
    throw std::runtime_error("Error in JsonValue::asFloat(): Value type is not compatible.");
    return 0.0f;
}

double JsonValue::asDouble() const {
    if (valueType == JsonValueType::REAL_VALUE) {
        return data.realValue;
    } else if (valueType == JsonValueType::NULL_VALUE) {
        return 0.0;
    } else if (valueType == JsonValueType::INT_VALUE || valueType == JsonValueType::BOOLEAN_VALUE) {
        return double(data.intValue);
    } else if (valueType == JsonValueType::UINT_VALUE) {
        return double(data.uintValue);
    } else if (valueType == JsonValueType::STRING_VALUE) {
        return sgl::fromString<double>(*data.stringValue);
    }
    throw std::runtime_error("Error in JsonValue::asDouble(): Value type is not compatible.");
    return 0.0;
}

bool JsonValue::asBool() const {
    if (valueType == JsonValueType::BOOLEAN_VALUE || valueType == JsonValueType::INT_VALUE
            || valueType == JsonValueType::UINT_VALUE) {
        return bool(data.intValue);
    } else if (valueType == JsonValueType::NULL_VALUE) {
        return false;
    }
    throw std::runtime_error("Error in JsonValue::asBool(): Value type is not compatible.");
    return false;
}

const char* JsonValue::asCString() const {
    if (valueType == JsonValueType::STRING_VALUE) {
        return data.stringValue->c_str();
    } else if (valueType == JsonValueType::NULL_VALUE) {
        return "";
    }
    throw std::runtime_error("Error in JsonValue::asCString(): Value type is not compatible.");
    return "";
}

std::string JsonValue::asString() const {
    if (valueType == JsonValueType::STRING_VALUE) {
        return *data.stringValue;
    } else if (valueType == JsonValueType::NULL_VALUE) {
        return "";
    } else if (valueType == JsonValueType::INT_VALUE) {
        return toString(data.intValue);
    } else if (valueType == JsonValueType::UINT_VALUE) {
        return toString(data.uintValue);
    } else if (valueType == JsonValueType::REAL_VALUE) {
        return toString(data.realValue);
    } else if (valueType == JsonValueType::BOOLEAN_VALUE) {
        return data.intValue ? "true" : "false";
    }
    throw std::runtime_error("Error in JsonValue::asString(): Value type is not compatible.");
    return "";
}

std::optional<std::string> parseString(const char* jsonString, size_t length, bool checkError, size_t& i) {
    std::string accumulator;
    bool wasLastEscape = false;
    for (; i < length; i++) {
        char c = jsonString[i];
        if (wasLastEscape) {
            if (c == '\\' || c == '\"') {
                accumulator += c;
            } else if (c == 'n') {
                accumulator += '\n';
            } else if (c == 'r') {
                accumulator += '\r';
            } else if (c == 't') {
                accumulator += '\t';
            } else {
                if (checkError) {
                    throw std::runtime_error("parseSimpleJson: Invalid escaped char.");
                }
                return {};
            }
            wasLastEscape = false;
        } else if (c == '\\') {
            wasLastEscape = true;
        } else if (c == '\"') {
            // Finished
            break;
        } else {
            accumulator += c;
        }
    }
    return accumulator;
}

static inline bool isWhitespace(const char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

std::optional<JsonValue> parseSimpleJson(const char* jsonString, size_t length, bool checkError, size_t& i) {
    JsonValue curr;
    bool isUnknownPrimitiveType = false; // Primitive type info.
    bool keyMode = true; //< Whether object is expecting key or value.
    std::string accumulator; //< String accumulator for primitive data or object key name.
    bool lastWasComma = false; //< Object & array info.

    for (; i < length; i++) {
        char c = jsonString[i];
        if (isUnknownPrimitiveType) {
            if (c == ']' || c == '}') {
                // Object and list end chars are processed by caller.
                i--;
            }
            if (isWhitespace(c) || c == ',' || c == ']' || c == '}') {
                // Guess the primitive data type.
                if (accumulator == "null") {
                    return curr;
                }
                if (accumulator == "true") {
                    curr = true;
                    return curr;
                }
                if (accumulator == "false") {
                    curr = false;
                    return curr;
                }
                if (isInteger(accumulator)) {
                    // Decide if this is a signed or unsigned integer.
                    if (accumulator.front() == '-') {
                        curr = fromString<int64_t>(accumulator);
                    } else {
                        curr = fromString<uint64_t>(accumulator);
                    }
                    return curr;
                }
                if (isNumeric(accumulator)) {
                    curr = fromString<double>(accumulator);
                    return curr;
                }
                if (checkError) {
                    throw std::runtime_error(
                            "parseSimpleJson: Unknown primitive type data of \"" + accumulator + "\".");
                }
                return {};
            } else {
                accumulator += c;
                continue;
            }
        }
        if (curr.getValueType() == JsonValueType::NULL_VALUE) {
            if (isWhitespace(c)) {
                continue;
            } else if (c == '{') {
                curr = JsonValue(JsonValueType::OBJECT_VALUE);
            } else if (c == '[') {
                curr = JsonValue(JsonValueType::ARRAY_VALUE);
            } else if (c == '"') {
                curr = JsonValue(JsonValueType::STRING_VALUE);
            } else {
                accumulator += c;
                isUnknownPrimitiveType = true;
            }
            continue;
        }
        if (curr.getValueType() == JsonValueType::STRING_VALUE) {
            auto optString = parseString(jsonString, length, checkError, i);
            if (optString.has_value()) {
                curr = optString.value();
                return curr;
            } else {
                return {};
            }
        } else if (curr.getValueType() == JsonValueType::OBJECT_VALUE) {
            if (c == '}') {
                if (!keyMode) {
                    if (checkError) {
                        throw std::runtime_error("parseSimpleJson: '}' detected before value was specified.");
                    }
                    return {};
                }
                i++;
                return curr;
            }
            if (isWhitespace(c)) {
                continue;
            }
            if (keyMode) {
                if (c == ',') {
                    if (lastWasComma) {
                        if (checkError) {
                            throw std::runtime_error("parseSimpleJson: Two commas in a row in object.");
                        }
                        return {};
                    }
                    lastWasComma = true;
                    continue;
                }
                lastWasComma = false;
                if (c != '\"') {
                    if (checkError) {
                        throw std::runtime_error("parseSimpleJson: '\"' expected to define key.");
                    }
                    return {};
                }
                i++; //< skip '"'
                auto optString = parseString(jsonString, length, checkError, i);
                if (optString.has_value()) {
                    accumulator = optString.value();
                } else {
                    return {};
                }
                keyMode = false;
            } else {
                if (c != ':') {
                    if (checkError) {
                        throw std::runtime_error("parseSimpleJson: ':' expected to separate key and value.");
                    }
                    return {};
                }
                i++; //< skip ':'
                auto valueOpt = parseSimpleJson(jsonString, length, checkError, i);
                if (!valueOpt.has_value()) {
                    return {};
                }
                curr[accumulator] = valueOpt.value();
                keyMode = true;
            }
        } else if (curr.getValueType() == JsonValueType::ARRAY_VALUE) {
            if (c == ']') {
                i++;
                return curr;
            }
            if (c == ',') {
                if (lastWasComma) {
                    if (checkError) {
                        throw std::runtime_error("parseSimpleJson: Two commas in a row in array.");
                    }
                    return {};
                }
                lastWasComma = true;
                continue;
            }
            if (isWhitespace(c)) {
                continue;
            }
            lastWasComma = false;
            auto valueOpt = parseSimpleJson(jsonString, length, checkError, i);
            if (!valueOpt.has_value()) {
                return {};
            }
            curr[curr.size()] = valueOpt.value();
        } else {
            if (checkError) {
                throw std::runtime_error("parseSimpleJson: This code branch should never be reached.");
            }
            return {};
        }
    }
    return curr;
}

JsonValue parseSimpleJson(const char* jsonString, size_t length, bool checkError) {
    size_t i = 0;
    std::optional<JsonValue> root = parseSimpleJson(jsonString, length, checkError, i);
    if (!root.has_value()) {
        return {};
    }
    if (i != length) {
        // Skip any whitespace at the end of the file.
        for (; i < length; i++) {
            char c = jsonString[i];
            if (!isWhitespace(c)) {
                break;
            }
        }
    }
    if (i != length) {
        if (checkError) {
            throw std::runtime_error("parseSimpleJson: More than one object stored.");
        }
        return {};
    }
    return root.value();
}

JsonValue parseSimpleJson(const std::string& jsonString, bool checkError) {
    return parseSimpleJson(jsonString.c_str(), jsonString.length(), checkError);
}

JsonValue readSimpleJson(const std::string& filePath, bool checkError) {
    uint8_t* buffer = nullptr;
    size_t bufferSize = 0;
    if (!loadFileFromSource(filePath, buffer, bufferSize, false)) {
        return {};
    }
    JsonValue value = parseSimpleJson(reinterpret_cast<const char*>(buffer), bufferSize, checkError);
    delete[] buffer;
    return value;
}

static std::string jsonEscapeString(const std::string& s) {
    if (s.find('\\') == std::string::npos && s.find('\"') == std::string::npos
            && s.find('\n') == std::string::npos && s.find('\r') == std::string::npos) {
        // Nothing to escape
        return s;
    }

    // Replace quotes by double-quotes and return string enclosed with single quotes
    std::string escapedString = stringReplaceAllCopy(s, "\\", "\\\\");
    stringReplaceAll(escapedString, "\"", "\\\"");
    stringReplaceAll(escapedString, "\n", "\\\n");
    stringReplaceAll(escapedString, "\r", "\\\r");
    return escapedString;
}

static bool writeSimpleJson(std::ofstream& outputFile, const JsonValue& jsonValue, int numSpaces, int recursionLevel) {
    if (jsonValue.isNull()) {
        outputFile << "null";
    } else if (jsonValue.isObject()) {
        outputFile << "{";
        size_t mapIndex = 0;
        std::string spacesStringCurr = std::string(recursionLevel * numSpaces, ' ');
        std::string spacesStringNext = std::string((recursionLevel + 1) * numSpaces, ' ');
        for (const auto& it : jsonValue) {
            if (mapIndex != 0) {
                outputFile << ",\n";
            } else {
                outputFile << "\n";
            }
            outputFile << spacesStringNext << "\"" << it.first << "\": ";
            writeSimpleJson(outputFile, it.second, numSpaces, recursionLevel + 1);
            mapIndex++;
        }
        outputFile << "\n" << spacesStringCurr << "}";
    } else if (jsonValue.isArray()) {
        outputFile << "[ ";
        for (size_t i = 0; i < jsonValue.size(); i++) {
            if (i != 0) {
                outputFile << ", ";
            }
            writeSimpleJson(outputFile, jsonValue[i], numSpaces, recursionLevel);
        }
        outputFile << " ]";
    } else if (jsonValue.isInt()) {
        outputFile << jsonValue.asInt64();
    } else if (jsonValue.isUint()) {
        outputFile << jsonValue.asUint64();
    } else if (jsonValue.isReal()) {
        outputFile << jsonValue.asDouble();
    } else if (jsonValue.isBool()) {
        outputFile << jsonValue.asString();
    } else if (jsonValue.isString()) {
        outputFile << "\"" << jsonEscapeString(jsonValue.asString()) << "\"";
    }
    return true;
}

bool writeSimpleJson(const std::string& filePath, const JsonValue& jsonValue, int numSpaces) {
    std::ofstream outputFile(filePath.c_str());
    if (!outputFile.is_open()) {
        return false;
    }
    bool retVal = writeSimpleJson(outputFile, jsonValue, numSpaces, 0);
    outputFile.close();
    return retVal;
}

}
