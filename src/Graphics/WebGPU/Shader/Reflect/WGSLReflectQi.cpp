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

#include "WGSLReflectInternal.hpp"

namespace fusion = boost::fusion;
namespace phoenix = boost::phoenix;
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

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
        function.name("function");
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

bool wgslReflectParseQi(const std::string& fileContent, wgsl_content& content, std::string& errorString) {
    // The grammar.
    wgsl_shaders_grammar<std::string::const_iterator> grammar;

    std::string::const_iterator iter = fileContent.begin();
    std::string::const_iterator end = fileContent.end();
    bool r = phrase_parse(iter, end, grammar, ascii::space, content);
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
