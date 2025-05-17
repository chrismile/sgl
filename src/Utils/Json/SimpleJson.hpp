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

#ifndef SIMPLEJSON_HPP
#define SIMPLEJSON_HPP

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <Utils/Convert.hpp>

namespace sgl {

/*
 * Simple JSON reader and writer with limited subset of supported functionality.
 * While jsoncpp or nlohmann/json could be added as a dependency, we want to keep sgl lean.
 */

enum class JsonValueType {
    NULL_VALUE, INT_VALUE, UINT_VALUE, REAL_VALUE, BOOLEAN_VALUE, STRING_VALUE, ARRAY_VALUE, OBJECT_VALUE
};

class JsonValue;

class DLL_OBJECT JsonValue {
public:
    JsonValue();
    explicit JsonValue(JsonValueType valueType);
    JsonValue(const JsonValue& other);
    JsonValue(JsonValue&& other) noexcept;
    ~JsonValue();
    JsonValue& operator=(const JsonValue& other);
    [[nodiscard]] inline JsonValueType getValueType() const { return valueType; }

    // Setters for primitive types.
    JsonValue& operator=(int32_t intObj);
    JsonValue& operator=(int64_t intObj);
    JsonValue& operator=(uint32_t intObj);
    JsonValue& operator=(uint64_t intObj);
    JsonValue& operator=(float realObj);
    JsonValue& operator=(double realObj);
    JsonValue& operator=(bool boolObj);

    // String functionality.
    JsonValue& operator=(const std::string& stringObj);
    JsonValue& operator=(const char* stringObj);

    // Array functionality.
    [[nodiscard]] JsonValue& operator[](size_t index);
    [[nodiscard]] const JsonValue& operator[](size_t index) const;
    [[nodiscard]] size_t size() const;

    // Object functionality.
    [[nodiscard]] JsonValue& operator[](const std::string& key);
    [[nodiscard]] const JsonValue& operator[](const std::string& key) const;
    [[nodiscard]] bool hasMember(const std::string& key) const;
    [[nodiscard]] std::map<std::string, JsonValue>::iterator begin();
    [[nodiscard]] std::map<std::string, JsonValue>::iterator end();
    [[nodiscard]] std::map<std::string, JsonValue>::const_iterator begin() const;
    [[nodiscard]] std::map<std::string, JsonValue>::const_iterator end() const;
    [[nodiscard]] std::map<std::string, JsonValue>::const_iterator cbegin() const;
    [[nodiscard]] std::map<std::string, JsonValue>::const_iterator cend() const;
    void erase(const std::string& key);

    // Functionality for querying type information.
    [[nodiscard]] inline bool isNull() const { return valueType == JsonValueType::NULL_VALUE; }
    [[nodiscard]] inline bool isAnyInt() const { return valueType == JsonValueType::INT_VALUE || valueType == JsonValueType::UINT_VALUE; }
    [[nodiscard]] inline bool isInt() const { return valueType == JsonValueType::INT_VALUE; }
    [[nodiscard]] inline bool isUint() const { return valueType == JsonValueType::UINT_VALUE; }
    [[nodiscard]] inline bool isReal() const { return valueType == JsonValueType::REAL_VALUE; }
    [[nodiscard]] inline bool isBool() const { return valueType == JsonValueType::BOOLEAN_VALUE; }
    [[nodiscard]] inline bool isString() const { return valueType == JsonValueType::STRING_VALUE; }
    [[nodiscard]] inline bool isArray() const { return valueType == JsonValueType::ARRAY_VALUE; }
    [[nodiscard]] inline bool isObject() const { return valueType == JsonValueType::OBJECT_VALUE; }

    // Functionality for retrieving type data.
    [[nodiscard]] int32_t asInt32() const;
    [[nodiscard]] int64_t asInt64() const;
    [[nodiscard]] uint32_t asUint32() const;
    [[nodiscard]] uint64_t asUint64() const;
    [[nodiscard]] float asFloat() const;
    [[nodiscard]] double asDouble() const;
    [[nodiscard]] bool asBool() const;
    [[nodiscard]] const char* asCString() const;
    [[nodiscard]] std::string asString() const;
    template<class T> void getTyped(T& val) const { val = sgl::fromString<T>(asString()); }
    void getTyped(int16_t& val) const { val = int16_t(asInt64()); }
    void getTyped(int32_t& val) const { val = asInt32(); }
    void getTyped(int64_t& val) const { val = asInt64(); }
    void getTyped(uint16_t& val) const { val = uint16_t(asUint64()); }
    void getTyped(uint32_t& val) const { val = asUint32(); }
    void getTyped(uint64_t& val) const { val = asUint64(); }
    void getTyped(float& val) const { val = asFloat(); }
    void getTyped(double& val) const { val = asDouble(); }
    void getTyped(bool& val) const { val = asBool(); }
    void getTyped(std::string& val) const { val = asString(); }

private:
    void deleteData();
    JsonValueType valueType;
    union ValueData {
        int64_t intValue;
        uint64_t uintValue;
        double realValue;
        std::string* stringValue;
        std::vector<JsonValue>* arrayValues;
        std::map<std::string, JsonValue>* mapValues;
    } data;
};

DLL_OBJECT JsonValue parseSimpleJson(const char* jsonString, size_t length, bool checkError);
DLL_OBJECT JsonValue parseSimpleJson(const std::string& jsonString, bool checkError);
DLL_OBJECT JsonValue readSimpleJson(const std::string& filePath, bool checkError);
DLL_OBJECT bool writeSimpleJson(const std::string& filePath, const JsonValue& jsonValue, int numSpaces);

/*
 * Sample code validated with Valgring memcheck:
 *
 * sgl::JsonValue nestedObj;
 * nestedObj["abc"] = 0;
 *
 * sgl::JsonValue array;
 * array[0] = 1;
 * array[1] = 10;
 * array[2] = 100;
 *
 * sgl::JsonValue root;
 * root["key0"] = "string_value";
 *  root["key1"] = 17;
 *  root["key2"] = -3;
 *  root["key3"] = 18.75;
 *  root["key4"] = 0.00001;
 *  root["key5"] = array;
 *  root["key6"] = nestedObj;
 *  root["key7"] = true;
 *  root["key8"] = false;
 *
 * sgl::writeSimpleJson("test.json", root, 4);
 *
 * auto rootClone = sgl::readSimpleJson("test.json");
 *
 * auto rootClone = sgl::readSimpleJson("test.json", true);
 * sgl::writeSimpleJson("test-clone.json", rootClone, 4);
 */

}

#endif //SIMPLEJSON_HPP
