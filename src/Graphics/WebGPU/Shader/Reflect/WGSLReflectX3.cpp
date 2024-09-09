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

#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include "WGSLReflectInternal.hpp"

namespace x3 = boost::spirit::x3;
namespace ascii = boost::spirit::x3::ascii;

BOOST_FUSION_ADAPT_STRUCT(
        sgl::webgpu::wgsl_type,
        name, template_parameters
)

BOOST_FUSION_ADAPT_STRUCT(
        sgl::webgpu::wgsl_attribute,
        name, expression
)
BOOST_FUSION_ADAPT_STRUCT(
        sgl::webgpu::wgsl_struct_entry,
        attributes, name, type
)
BOOST_FUSION_ADAPT_STRUCT(
        sgl::webgpu::wgsl_struct,
        name, entries
)
BOOST_FUSION_ADAPT_STRUCT(
        sgl::webgpu::wgsl_variable,
        attributes, modifiers, name, type
)
BOOST_FUSION_ADAPT_STRUCT(
        sgl::webgpu::wgsl_constant,
        (std::string, name)
        (sgl::webgpu::wgsl_type, type)
        (std::string, value)
)

BOOST_FUSION_ADAPT_STRUCT(
        sgl::webgpu::wgsl_function,
        attributes, name, parameters, return_type_attributes, return_type, function_content
)

BOOST_FUSION_ADAPT_STRUCT(
        sgl::webgpu::wgsl_directive,
        directive_type, values
)

namespace sgl { namespace webgpu {

namespace parser {
namespace x3 = boost::spirit::x3;
namespace ascii = boost::spirit::x3::ascii;

using x3::int_;
using x3::double_;
using x3::lexeme;
using ascii::char_;
using ascii::lit;

x3::rule<class expression, std::string> const expression = "expression";
x3::rule<class identifier, std::string> const identifier = "identifier";
x3::rule<class identifier_noskip, std::string> const identifier_noskip = "identifier_noskip";
x3::rule<class type, wgsl_type> const type = "type";

/*
 * Attributes contain information about variables and struct members, such as binding, location, offset...
 * See: https://www.w3.org/TR/WGSL/#attributes
 */
x3::rule<class attribute, wgsl_attribute> const attribute = "attribute";

x3::rule<class struct_entry, wgsl_struct_entry> const struct_entry = "struct_entry";
x3::rule<class _struct, wgsl_struct> const _struct = "struct";

x3::rule<class variable, wgsl_variable> const variable = "variable";

x3::rule<class constant, wgsl_constant> const constant = "constant";

x3::rule<class function, wgsl_function> const function = "function";
x3::rule<class function_content, std::string> const function_content = "function_content";

// https://www.w3.org/TR/WGSL/#directives
x3::rule<class directive, wgsl_directive> const directive = "directive";

x3::rule<class content, wgsl_content> const content = "content";

auto const expression_def = *char_("a-zA-Z_0-9\\-\\*\\+/\\.");
auto const identifier_def = char_("a-zA-Z_") >> *char_("a-zA-Z_0-9\\-");
auto const identifier_noskip_def = lexeme[char_("a-zA-Z_") >> *char_("a-zA-Z_0-9\\-")];
auto const type_def =
        +char_("a-zA-Z_0-9")
        >> -(x3::rule<struct _, std::vector<wgsl_type>> {} = lit('<') >> type % ',' >> lit('>'));
// E.g.: @builtin(position)
auto const attribute_def = lit("@") >> identifier_noskip >> -('(' >> expression >> ')');
// E.g.: @builtin(position) position: vec4f
auto const struct_entry_def = *attribute >> identifier >> ':' >> type;
/*
 * E.g.:
 * struct VertexOutput {
 * 	   @builtin(position) position: vec4f,
 * 	   @location(0) color: vec3f,
 * };
 */
auto const _struct_def =
        lit("struct")
        >> identifier
        >> '{'
        >> struct_entry % ','
        >> -lit(',')
        >> '}'
        >> *lit(';');
// E.g.: @group(0) @binding(0) var<storage,read> inputBuffer: array<f32,64>;
auto const variable_def =
        *attribute
        >> lit("var")
        >> -(x3::rule<struct _, std::vector<std::string>> {} = '<' >> (identifier % ',') >> '>')
        >> identifier
        >> ':'
        >> type
        >> +lit(';');
// E.g.: const PI: f32 = 3.141;
auto const constant_def =
        lit("const")
        >> identifier
        >> -(':' >> type)
        >> '='
        >> expression
        >> +lit(';');
auto const function_content_def =
        '{'
        >> *(char_ - char_("{}"))
        >> *(function_content >> *(char_ - char_("{}")))
        >> *(char_ - char_("{}"))
        >> '}';
// E.g.: @fragment fn fs_main(vertex_in: VertexOut) -> FragmentOut { ... }
auto unpack_function_attr = [](auto& ctx){ _val(ctx).attributes.push_back(_attr(ctx)); };
auto unpack_function_name = [](auto& ctx){ _val(ctx).name = _attr(ctx); };
auto unpack_function_params = [](auto& ctx){ _val(ctx).parameters.push_back(_attr(ctx)); };
auto unpack_function_retatts = [](auto& ctx){ _val(ctx).return_type_attributes.push_back(_attr(ctx)); };
auto unpack_function_rettype = [](auto& ctx){ _val(ctx).return_type = _attr(ctx); };
auto unpack_function_content = [](auto& ctx){ _val(ctx).function_content = _attr(ctx); };
auto const function_def =
        *attribute[unpack_function_attr]
        >> lit("fn")
        >> identifier[unpack_function_name]
        >> '('
        >> -(struct_entry[unpack_function_params] % ',')
        >> -lit(',')
        >> ')'
        >> -(lit("->") >> *attribute[unpack_function_retatts] >> type[unpack_function_rettype])
        >> function_content[unpack_function_content]
        >> *lit(';');
auto set_string_enable = [](auto& ctx){ _val(ctx).directive_type = "enable"; };
auto set_string_requires = [](auto& ctx){ _val(ctx).directive_type = "requires"; };
auto set_string_diagnostic = [](auto& ctx){ _val(ctx).directive_type = "diagnostic"; };
auto unpack_directive_values = [](auto& ctx){ return _val(ctx).values.push_back(_attr(ctx)); };
auto const directive_def =
        (lit("enable")[set_string_enable]
            | lit("requires")[set_string_requires]
            | lit("diagnostic")[set_string_diagnostic])
        >> identifier[unpack_directive_values] % ','
        >> +lit(';');
auto const content_def = *(x3::rule<struct _, wgsl_entry> {} = _struct | variable | constant | function | directive);

//BOOST_SPIRIT_DEFINE(
//        expression, identifier, identifier_noskip, type,
//        attribute, struct_entry, _struct, variable, constant, function, function_content, directive, content);
BOOST_SPIRIT_DEFINE(expression)
BOOST_SPIRIT_DEFINE(identifier)
BOOST_SPIRIT_DEFINE(identifier_noskip)
BOOST_SPIRIT_DEFINE(type)
BOOST_SPIRIT_DEFINE(attribute)
BOOST_SPIRIT_DEFINE(struct_entry)
BOOST_SPIRIT_DEFINE(_struct)
BOOST_SPIRIT_DEFINE(variable)
BOOST_SPIRIT_DEFINE(constant)
BOOST_SPIRIT_DEFINE(function)
BOOST_SPIRIT_DEFINE(function_content)
BOOST_SPIRIT_DEFINE(directive)
BOOST_SPIRIT_DEFINE(content);
//BOOST_SPIRIT_DEFINE(content);

}

bool wgslReflectParseX3(const std::string& fileContent, wgsl_content& content, std::string& errorString) {
    std::string::const_iterator iter = fileContent.begin();
    std::string::const_iterator end = fileContent.end();

    bool r = phrase_parse(iter, end, parser::content, ascii::space, content);
    if (!r || iter != end) {
        auto charIdx = size_t(std::distance(std::string::const_iterator(fileContent.begin()), iter));
        std::string lineStr;
        size_t lineIdx = 0, charIdxInLine = 0;
        getLineInfo(fileContent, charIdx, lineStr, lineIdx, charIdxInLine);
        errorString =
                "Pattern matching failed at line " + std::to_string(lineIdx) + ":\n"
                + lineStr + "\n" + std::string(charIdxInLine, ' ') + "^~~~ HERE";
        return false;
    }

    return true;
}

}}
