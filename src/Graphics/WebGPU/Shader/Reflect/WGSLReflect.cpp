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

//#define BOOST_SPIRIT_DEBUG
#include <boost/spirit/include/qi.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/phoenix/fusion.hpp>
#include <boost/phoenix/stl.hpp>
#include <boost/phoenix/object.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/variant/recursive_variant.hpp>

#include <Utils/Convert.hpp>
#include <Utils/StringUtils.hpp>
#include "WGSLReflect.hpp"

namespace sgl { namespace webgpu {

namespace fusion = boost::fusion;
namespace phoenix = boost::phoenix;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

// See: https://www.w3.org/TR/WGSL/#types, e.g.: ptr<function,vec3<f32>>
struct wgsl_type {
    std::string name;
    boost::optional<std::vector<wgsl_type>> template_parameters;
};

static std::set<std::string> builtinTypes = {
        "bool", "f16", "f32", "f64", "i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64",
        "vec2", "vec3", "vec4", // < with templates.
        "vec2i", "vec3i", "vec4i", "vec2u", "vec3u", "vec4u",
        "vec2f", "vec3f", "vec4f", "vec2h", "vec3h", "vec4h",
        "mat2x2f", "mat2x3f", "mat2x4f", "mat3x2f", "mat3x3f", "mat3x4f", "mat4x2f", "mat4x3f", "mat4x4f",
        "mat2x2h", "mat2x3h", "mat2x4h", "mat3x2h", "mat3x3h", "mat3x4h", "mat4x2h", "mat4x3h", "mat4x4h",
        "ref", "pointer", "atomic", //< With templates.
        "array" // <E,N> for fixed-size N or <E> for runtime-sized array.
};
static bool isTypeBuiltin(const wgsl_type& type) {
    return builtinTypes.find(type.name) != builtinTypes.end();
}

// E.g.: @builtin(position)
struct wgsl_attribute {
    std::string name;
    std::string expression;
};

/**
 * @param attributes The set of attributes.
 * @param name The name of the searched attribute.
 * @return A pointer to the attribute if found or a null pointer otherwise.
 */
static const wgsl_attribute* findAttributeByName(
        const std::vector<wgsl_attribute>& attributes, const std::string& name) {
    for (const auto& attribute : attributes) {
        if (attribute.name == name) {
            return &attribute;
        }
    }
    return nullptr;
}

// E.g.: @builtin(position) position: vec4f
struct wgsl_struct_entry {
    std::vector<wgsl_attribute> attributes;
    std::string name;
    wgsl_type type;
};

/*
 * E.g.:
 * struct VertexOutput {
 * 	   @builtin(position) position: vec4f,
 * 	   @location(0) color: vec3f,
 * };
 */
struct wgsl_struct {
    std::string name;
    std::vector<wgsl_struct_entry> entries;
};

// E.g.: @group(0) @binding(0) var<storage,read> inputBuffer: array<f32,64>;
struct wgsl_variable {
    std::vector<wgsl_attribute> attributes;
    boost::optional<std::vector<std::string>> modifiers;
    std::string name;
    wgsl_type type;
};

// E.g.: const PI: f32 = 3.141;
struct wgsl_constant {
    std::string name;
    wgsl_type type;
    std::string value;
};

// E.g.: @fragment fn fs_main(vertex_in: VertexOut) -> FragmentOut { ... }
struct wgsl_function {
    std::vector<wgsl_attribute> attributes;
    std::string name;
    std::vector<wgsl_struct_entry> parameters;
    std::vector<wgsl_attribute> return_type_attributes;
    boost::optional<wgsl_type> return_type;
    std::string function_content;
};

struct wgsl_directive {
    std::string directive_type; // enable, requires, diagnostic
    std::vector<std::string> values;
};

typedef boost::variant<wgsl_struct, wgsl_variable, wgsl_constant, wgsl_function, wgsl_directive> wgsl_entry;

}}

BOOST_FUSION_ADAPT_STRUCT(
        sgl::webgpu::wgsl_type,
        (std::string, name)
        (boost::optional<std::vector<sgl::webgpu::wgsl_type>>, template_parameters)
)

BOOST_FUSION_ADAPT_STRUCT(
        sgl::webgpu::wgsl_attribute,
        (std::string, name)
        (std::string, expression)
)
BOOST_FUSION_ADAPT_STRUCT(
        sgl::webgpu::wgsl_struct_entry,
        (std::vector<sgl::webgpu::wgsl_attribute>, attributes)
        (std::string, name)
        (sgl::webgpu::wgsl_type, type)
)
BOOST_FUSION_ADAPT_STRUCT(
        sgl::webgpu::wgsl_struct,
        (std::string, name)
        (std::vector<sgl::webgpu::wgsl_struct_entry>, entries)
)
BOOST_FUSION_ADAPT_STRUCT(
        sgl::webgpu::wgsl_variable,
        (std::vector<sgl::webgpu::wgsl_attribute>, attributes)
        (boost::optional<std::vector<std::string>>, modifiers)
        (std::string, name)
        (sgl::webgpu::wgsl_type, type)
)
BOOST_FUSION_ADAPT_STRUCT(
        sgl::webgpu::wgsl_constant,
        (std::string, name)
        (sgl::webgpu::wgsl_type, type)
        (std::string, value)
)

BOOST_FUSION_ADAPT_STRUCT(
        sgl::webgpu::wgsl_function,
        (std::vector<sgl::webgpu::wgsl_attribute>, attributes)
        (std::string, name)
        (std::vector<sgl::webgpu::wgsl_struct_entry>, parameters)
        (std::vector<sgl::webgpu::wgsl_attribute>, return_type_attributes)
        (boost::optional<sgl::webgpu::wgsl_type>, return_type)
        (std::string, function_content)
)

BOOST_FUSION_ADAPT_STRUCT(
        sgl::webgpu::wgsl_directive,
        (std::string, directive_type)
        (std::vector<std::string>, values)
)

namespace sgl { namespace webgpu {

typedef std::vector<wgsl_entry> wgsl_content;

// The necessary subset of the WGSL grammar for reflection built via boost::spirit:qi. Ignores function bodies.
template <typename Iterator>
struct wgsl_shaders_grammar
        : qi::grammar<Iterator, wgsl_content(), ascii::space_type>
{
    wgsl_shaders_grammar()
            : wgsl_shaders_grammar::base_type(content, "content")
    {
        using qi::lit;
        using qi::lexeme;
        using qi::on_error;
        using qi::fail;
        using qi::double_;
        using qi::int_;
        using qi::blank;
        using qi::no_skip;
        using qi::space;
        using ascii::char_;
        using ascii::string;
        using ascii::space_type;
        using namespace qi::labels;

        using phoenix::construct;
        using phoenix::val;
        using phoenix::at_c;
        using phoenix::push_back;

        expression %=
                *char_("a-zA-Z_0-9\\-\\*\\+/\\.");
        expression.name("expression");
        //BOOST_SPIRIT_DEBUG_NODE(expression);

        identifier =
                char_("a-zA-Z_")[_val = _1]
                >> *char_("a-zA-Z_0-9\\-")[_val += _1];
        identifier.name("identifier");
        //BOOST_SPIRIT_DEBUG_NODE(identifier);

        identifier_noskip =
                lexeme[char_("a-zA-Z_")[_val = _1]
                 >> *char_("a-zA-Z_0-9\\-")[_val += _1]];
        identifier_noskip.name("identifier_noskip");
        //BOOST_SPIRIT_DEBUG_NODE(identifier_noskip);

        //type =
        //        char_("a-zA-Z_")[_val = _1]
        //                >> *char_("a-zA-Z_0-9\\-<>")[_val += _1];
        //type.name("type");
        //BOOST_SPIRIT_DEBUG_NODE(type);
        // For sake of simplicity and recursion support, allow starting with number.
        type %=
                +char_("a-zA-Z_0-9")//[at_c<0>(_val) += _1]
                >> -(lit('<') >> type % ',' >> lit('>'));
        type.name("type");
        //BOOST_SPIRIT_DEBUG_NODE(type);

        // E.g.: @builtin(position)
        attribute =
                lit("@")
                >> identifier_noskip[at_c<0>(_val) = _1]
                >> -('(' >> expression[at_c<1>(_val) = _1] >> ')');
        //BOOST_SPIRIT_DEBUG_NODE(attribute);

        // E.g.: @builtin(position) position: vec4f
        struct_entry =
                *attribute[push_back(at_c<0>(_val), _1)]
                >> identifier[at_c<1>(_val) = _1]
                >> ':'
                >> type[at_c<2>(_val) = _1];
        struct_entry.name("struct_entry");
        //BOOST_SPIRIT_DEBUG_NODE(struct_entry);

        /*
         * E.g.:
         * struct VertexOutput {
         * 	   @builtin(position) position: vec4f,
         * 	   @location(0) color: vec3f,
         * };
         */
        _struct =
                lit("struct")
                >> identifier[at_c<0>(_val) = _1]
                >> '{'
                >> struct_entry[push_back(at_c<1>(_val), _1)] % ','
                >> -lit(',')
                >> '}'
                >> *lit(';');
        _struct.name("struct");
        //BOOST_SPIRIT_DEBUG_NODE(_struct);

        // E.g.: @group(0) @binding(0) var<storage,read> inputBuffer: array<f32,64>;
        variable =
                *attribute[push_back(at_c<0>(_val), _1)]
                >> lit("var")
                >> -('<' >> (identifier % ',') >> '>')[at_c<1>(_val) = _1]
                >> identifier[at_c<2>(_val) = _1]
                >> ':'
                >> type[at_c<3>(_val) = _1]
                >> +lit(';');
        variable.name("variable");
        //BOOST_SPIRIT_DEBUG_NODE(variable);

        // E.g.: const PI: f32 = 3.141;
        constant %=
                lit("const")
                >> identifier
                >> -(':' >> type)
                >> '='
                >> expression
                >> +lit(';');
        constant.name("constant");
        //BOOST_SPIRIT_DEBUG_NODE(constant);

        function_content =
                '{'
                >> *(char_ - char_("{}"))
                >> *(function_content >> *(char_ - char_("{}")))
                >> *(char_ - char_("{}"))
                >> '}';
        function_content.name("function_content");
        //BOOST_SPIRIT_DEBUG_NODE(function_content);

        // E.g.: @fragment fn fs_main(vertex_in: VertexOut) -> FragmentOut { ... }
        function =
                *attribute[push_back(at_c<0>(_val), _1)]
                >> lit("fn")
                >> identifier[at_c<1>(_val) = _1]
                >> '('
                >> struct_entry[push_back(at_c<2>(_val), _1)] % ','
                >> -lit(',')
                >> ')'
                >> -(lit("->") >> *attribute[push_back(at_c<3>(_val), _1)] >> type[at_c<4>(_val) = _1])
                >> function_content[at_c<5>(_val) = _1]
                >> *lit(';');
        constant.name("function");
        //BOOST_SPIRIT_DEBUG_NODE(function);

        directive =
                (lit("enable")[at_c<0>(_val) = "enable"] | lit("requires")[at_c<0>(_val) = "requires"] | lit("diagnostic")[at_c<0>(_val) = "diagnostic"])//[at_c<0>(_val) = _1]
                >> identifier[push_back(at_c<1>(_val), _1)] % ','
                >> +lit(';');
        directive.name("directive");
        //BOOST_SPIRIT_DEBUG_NODE(directive);

        content %= *(_struct | variable | constant | function | directive);
        content.name("content");
        //BOOST_SPIRIT_DEBUG_NODE(content);

        on_error<fail>
                (
                        content
                        , std::cout
                                << val("Error in wgsl_shaders_grammar. Expecting ")
                                << _4
                                << val(" here: \"")
                                << construct<std::string>(_3, _2)
                                << val("\".")
                                << std::endl
                );
    }

    /*
     * Building blocks.
     * - We don't evaluate expressions, so they are just a string.
     * - The identifier is the name of a struct, function for variable. It may not start with a number, and consists of
     *   alphabet chars and numbers.
     * - Type is the type of a variable, function parameter or function return value. It may contain "<" and ">".
     *   We do not deconstruct composite types such as vec3<f32> for now.
     */
    qi::rule<Iterator, std::string(), ascii::space_type> expression;
    qi::rule<Iterator, std::string(), ascii::space_type> identifier;
    qi::rule<Iterator, std::string(), ascii::space_type> identifier_noskip;
    qi::rule<Iterator, wgsl_type(), ascii::space_type> type;

    /*
     * Attributes contain information about variables and struct members, such as binding, location, offset...
     * See: https://www.w3.org/TR/WGSL/#attributes
     */
    qi::rule<Iterator, wgsl_attribute(), ascii::space_type> attribute;

    qi::rule<Iterator, wgsl_struct_entry(), ascii::space_type> struct_entry;
    qi::rule<Iterator, wgsl_struct(), ascii::space_type> _struct;

    qi::rule<Iterator, wgsl_variable(), ascii::space_type> variable;

    qi::rule<Iterator, wgsl_constant(), ascii::space_type> constant;

    qi::rule<Iterator, wgsl_function(), ascii::space_type> function;
    qi::rule<Iterator, std::string(), ascii::space_type> function_content;

    // https://www.w3.org/TR/WGSL/#directives
    qi::rule<Iterator, wgsl_directive(), ascii::space_type> directive;

    qi::rule<Iterator, wgsl_content(), ascii::space_type> content;
};

// Completely removes comments.
/*std::string removeCStyleComments(const std::string& stringWithComments) {
    bool isInLineComment = false;
    bool isInMultiLineComment = false;
    size_t strSize = stringWithComments.size();
    std::string stringWithoutComments;
    std::string::size_type currPos = 0;
    while (currPos < strSize) {
        char c0 = stringWithComments.at(currPos);
        char c1 = currPos + 1 < strSize ? stringWithComments.at(currPos + 1) : '\0';
        if (isInLineComment) {
            if (c0 == '\r' && c1 == '\n') {
                isInLineComment = false;
                currPos += 2;
            } else if (c0 == '\r' || c0 == '\n') {
                isInLineComment = false;
                currPos += 1;
            } else {
                currPos += 1;
            }
        } else if (isInMultiLineComment) {
            if (c0 == '*' && c1 == '/') {
                isInMultiLineComment = false;
                currPos += 2;
            } else {
                currPos += 1;
            }
        } else {
            if (c0 == '/' && c1 == '*') {
                isInMultiLineComment = true;
                currPos += 2;
            } else if (c0 == '/' && c1 == '/') {
                isInLineComment = true;
                currPos += 2;
            } else {
                stringWithoutComments += c0;
                currPos += 1;
            }
        }
    }
    return stringWithoutComments;
}*/

// Replaces any char in comment with space. This leaves char offsets valid.
std::string removeCStyleComments(const std::string& stringWithComments) {
    bool isInLineComment = false;
    bool isInMultiLineComment = false;
    size_t strSize = stringWithComments.size();
    std::string stringWithoutComments;
    std::string::size_type currPos = 0;
    while (currPos < strSize) {
        char c0 = stringWithComments.at(currPos);
        char c1 = currPos + 1 < strSize ? stringWithComments.at(currPos + 1) : '\0';
        if (isInLineComment) {
            if (c0 == '\r' && c1 == '\n') {
                isInLineComment = false;
                stringWithoutComments += "\r\n";
                currPos += 2;
            } else if (c0 == '\r' || c0 == '\n') {
                isInLineComment = false;
                stringWithoutComments += c0;
                currPos += 1;
            } else {
                stringWithoutComments += ' ';
                currPos += 1;
            }
        } else if (isInMultiLineComment) {
            if (c0 == '*' && c1 == '/') {
                isInMultiLineComment = false;
                stringWithoutComments += "  ";
                currPos += 2;
            } else {
                if (c0 == '\r' || c0 == '\n') {
                    stringWithoutComments += c0;
                } else {
                    stringWithoutComments += ' ';
                }
                currPos += 1;
            }
        } else {
            if (c0 == '/' && c1 == '*') {
                isInMultiLineComment = true;
                stringWithoutComments += "  ";
                currPos += 2;
            } else if (c0 == '/' && c1 == '/') {
                isInLineComment = true;
                stringWithoutComments += "  ";
                currPos += 2;
            } else {
                stringWithoutComments += c0;
                currPos += 1;
            }
        }
    }
    return stringWithoutComments;
}

/// Converts wgsl_entry objects to maps.
class wgsl_reflect_visitor : public boost::static_visitor<> {
public:
    wgsl_reflect_visitor(
            std::map<std::string, wgsl_struct>& structs,
            std::map<std::string, wgsl_variable>& variables,
            std::map<std::string, wgsl_constant>& constants,
            std::map<std::string, wgsl_function>& functions,
            std::map<std::string, std::set<std::string>>& directives)
            : structs(structs), variables(variables), constants(constants),
              functions(functions), directives(directives) {
    }
    void operator()(const wgsl_struct& _struct) const {
        structs.insert(std::make_pair(_struct.name, _struct));
    }
    void operator()(const wgsl_variable& variable) const {
        variables.insert(std::make_pair(variable.name, variable));
    }
    void operator()(const wgsl_constant& constant) const {
        constants.insert(std::make_pair(constant.name, constant));
    }
    void operator()(const wgsl_function& function) const {
        functions.insert(std::make_pair(function.name, function));
    }
    void operator()(const wgsl_directive& directive) const {
        for (const std::string& value : directive.values) {
            directives[directive.directive_type].insert(value);
        }
    }

private:
    std::map<std::string, wgsl_struct>& structs;
    std::map<std::string, wgsl_variable>& variables;
    std::map<std::string, wgsl_constant>& constants;
    std::map<std::string, wgsl_function>& functions;
    std::map<std::string, std::set<std::string>>& directives;
};

/**
 * Retrieves information about the line containing the char with the passed index.
 */
void getLineInfo(
        const std::string& fileContent, size_t charIdx, std::string& lineStr, size_t& lineIdx, size_t& charIdxInLine) {
    auto lineStart = fileContent.rfind('\n', charIdx);
    auto lineEnd = fileContent.find('\n', charIdx);
    if (lineStart == std::string::npos) {
        lineStart = 0;
    } else {
        lineStart++;
    }
    if (lineEnd == std::string::npos) {
        lineEnd = fileContent.size();
    }
    lineStr = fileContent.substr(lineStart, lineEnd - lineStart);
    charIdxInLine = charIdx - lineStart;
    lineIdx = size_t(std::count(fileContent.begin(), fileContent.begin() + charIdx, '\n')) + 1;
}

/// Processes vertex shader inputs (via function parameters) or fragment shader outputs (via return type).
bool processInOut(
        const std::map<std::string, wgsl_struct>& structs, std::string& errorString,
        const std::string& entryPointName, const std::string& inOutName,
        std::vector<InOutEntry>& inOutEntries, const wgsl_type& type, const std::vector<wgsl_attribute>& attributes) {
    // Is this a built-in type? If yes, check if it has the "location" attribute.
    if (isTypeBuiltin(type)) {
        const auto* locationAttribute = findAttributeByName(attributes, "location");
        if (locationAttribute) {
            InOutEntry inOutEntry{};
            inOutEntry.variableName = inOutName;
            inOutEntry.locationIndex = sgl::fromString<uint32_t>(locationAttribute->expression);
            inOutEntries.push_back(inOutEntry);
        }
        return true;
    }
    // If this is not a built-in type, it must be a valid struct type.
    auto itStruct = structs.find(type.name);
    if (itStruct == structs.end()) {
        errorString =
                "Found unresolved type \"" + type.name
                + "\" when parsing \"" + entryPointName + "\".";
        return false;
    }
    const auto& _struct = itStruct->second;

    // Iterate over all struct type entries and check if they have the "location" attribute set.
    for (const auto& entry : _struct.entries) {
        if (isTypeBuiltin(entry.type)) {
            const auto* locationAttribute = findAttributeByName(entry.attributes, "location");
            if (locationAttribute) {
                InOutEntry inOutEntry{};
                inOutEntry.variableName = entry.name;
                inOutEntry.locationIndex = sgl::fromString<uint32_t>(locationAttribute->expression);
                inOutEntries.push_back(inOutEntry);
            }
            continue;
        }
    }
    return true;
}

/// Returns the binding group entry type for a given WGSL type and "var" modifier.
static BindingEntryType getBindingEntryType(
        const wgsl_type& type, const boost::optional<std::vector<std::string>>& modifiersOpt) {
    // UNIFORM_BUFFER, TEXTURE, SAMPLER, STORAGE_BUFFER, STORAGE_TEXTURE
    BindingEntryType bindingEntryType = BindingEntryType::UNKNOWN;
    if (modifiersOpt.has_value()) {
        const auto& modifiers = modifiersOpt.get();
        if (std::find(modifiers.begin(), modifiers.end(), "uniform") != modifiers.end()) {
            // Example: var<uniform> settings: Settings;
            bindingEntryType = BindingEntryType::UNIFORM_BUFFER;
        } else if (std::find(modifiers.begin(), modifiers.end(), "storage") != modifiers.end()) {
            // Example: @group(0) @binding(0) var<storage,read> inputBuffer: array<f32,64>;
            // Example: @group(0) @binding(1) var<storage,read_write> outputBuffer: array<f32,64>;
            bindingEntryType = BindingEntryType::STORAGE_BUFFER;
        }
    } else {
        if (sgl::startsWith(type.name, "texture_storage_")) {
            // Example: @group(0) @binding(1) var nextMipLevel: texture_storage_2d<rgba8unorm,write>;
            bindingEntryType = BindingEntryType::STORAGE_TEXTURE;
        } else if (sgl::startsWith(type.name, "texture_")) {
            // Example: @group(0) @binding(1) var gradientTexture: texture_2d<f32>;
            bindingEntryType = BindingEntryType::TEXTURE;
        } else if (type.name == "sampler") {
            // Example: @group(0) @binding(2) var textureSampler: sampler;
            bindingEntryType = BindingEntryType::SAMPLER;
        }
    }
    return bindingEntryType;
}

bool wgslCodeReflect(const std::string& fileContent, ReflectInfo& reflectInfo, std::string& errorString) {
    std::string fileContentNoComments = removeCStyleComments(fileContent);

    // The grammar.
    wgsl_shaders_grammar<std::string::const_iterator> grammar;

    // The AST.
    wgsl_content content{};

    std::string::const_iterator iter = fileContentNoComments.begin();
    std::string::const_iterator end = fileContentNoComments.end();
    bool r = phrase_parse(iter, end, grammar, ascii::space, content);
    if (!r || iter != end) {
        auto charIdx = size_t(std::distance(std::string::const_iterator(fileContentNoComments.begin()), iter));
        std::string lineStr;
        size_t lineIdx = 0, charIdxInLine = 0;
        getLineInfo(fileContentNoComments, charIdx, lineStr, lineIdx, charIdxInLine);
        errorString =
                "Pattern matching failed at line " + std::to_string(lineIdx) + ":\n"
                + lineStr + "\n" + std::string(charIdxInLine, ' ') + "^~~~ HERE";
        return false;
    }

    std::map<std::string, wgsl_struct> structs;
    std::map<std::string, wgsl_variable> variables;
    std::map<std::string, wgsl_constant> constants;
    std::map<std::string, wgsl_function> functions;
    std::map<std::string, std::set<std::string>> directives;
    wgsl_reflect_visitor visitor(structs, variables, constants, functions, directives);
    for (const auto& entry : content) {
        boost::apply_visitor(visitor, entry);
    }

    // Debug output.
    /*for (auto& _struct : structs) {
        std::cout << "struct: " << _struct.first << std::endl;
    }
    for (auto& variable : variables) {
        std::cout << "variable: " << variable.first << std::endl;
    }
    for (auto& constant : constants) {
        std::cout << "constant: " << constant.first << std::endl;
    }
    for (auto& function : functions) {
        std::cout << "function: " << function.first << std::endl;
    }
    for (auto& directive : directives) {
        std::cout << "directive: " << directive.first << std::endl;
    }*/

    // Find all shader entry points.
    for (const auto& functionPair : functions) {
        const auto& function = functionPair.second;
        // Is this function a shader entry point, i.e., attribute "vertex", "fragment", or "compute"?
        ShaderType shaderType;
        bool foundShaderType = false;
        for (const auto& attribute : function.attributes) {
            if (attribute.name == "vertex") {
                shaderType = ShaderType::VERTEX;
                foundShaderType = true;
                break;
            } else if (attribute.name == "fragment") {
                shaderType = ShaderType::FRAGMENT;
                foundShaderType = true;
                break;
            } else if (attribute.name == "compute") {
                shaderType = ShaderType::COMPUTE;
                foundShaderType = true;
                break;
            }
        }
        if (!foundShaderType) {
            continue;
        }

        ShaderInfo shaderInfo{};
        shaderInfo.shaderType = shaderType;

        // Create reflection information on vertex shader inputs and fragment shader outputs.
        if (shaderType == ShaderType::VERTEX) {
            for (const auto& parameter : function.parameters) {
                if (!processInOut(
                        structs, errorString, function.name, parameter.name,
                        shaderInfo.inputs, parameter.type, parameter.attributes)) {
                    return false;
                }
            }
            std::sort(
                    shaderInfo.inputs.begin(), shaderInfo.inputs.end(),
                    [](const InOutEntry &entry0, const InOutEntry &entry1) { return entry0.locationIndex < entry1.locationIndex; });
        } else if (shaderType == ShaderType::FRAGMENT) {
            if (function.return_type.has_value()) {
                if (!processInOut(
                        structs, errorString, function.name, "",
                        shaderInfo.outputs, function.return_type.get(), function.return_type_attributes)) {
                    return false;
                }
            }
            std::sort(
                    shaderInfo.outputs.begin(), shaderInfo.outputs.end(),
                    [](const InOutEntry &entry0, const InOutEntry &entry1) { return entry0.locationIndex < entry1.locationIndex; });
        }

        reflectInfo.shaders.insert(std::make_pair(function.name, shaderInfo));
    }

    // Create reflection information on binding groups.
    for (const auto& variablePair : variables) {
        const auto& variable = variablePair.second;
        const auto* bindingAttribute = findAttributeByName(variable.attributes, "binding");
        if (bindingAttribute) {
            BindingEntry bindingEntry{};
            uint32_t groupIndex = 0;
            const auto* groupAttribute = findAttributeByName(variable.attributes, "group");
            if (groupAttribute) {
                groupIndex = sgl::fromString<uint32_t>(groupAttribute->expression);
            }
            bindingEntry.bindingIndex = sgl::fromString<uint32_t>(bindingAttribute->expression);
            bindingEntry.variableName = variable.name;
            bindingEntry.bindingEntryType = getBindingEntryType(variable.type, variable.modifiers);
            if (bindingEntry.bindingEntryType == BindingEntryType::UNKNOWN) {
                errorString =
                        "Could not resolve binding entry type for \"var " + variable.name + "\".";
                return false;
            }
            reflectInfo.bindingGroups[groupIndex].push_back(bindingEntry);
        }
    }
    for (auto& bindingGroup : reflectInfo.bindingGroups) {
        std::sort(
                bindingGroup.second.begin(), bindingGroup.second.end(),
                [](const BindingEntry &entry0, const BindingEntry &entry1) { return entry0.bindingIndex < entry1.bindingIndex; }
        );
    }

    return true;
}

}}
