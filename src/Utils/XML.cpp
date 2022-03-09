/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2016, Christoph Neuhauser
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

#include "XML.hpp"
#include <Math/Math.hpp>
#include <cstddef>

namespace sgl {

XMLNode* insertNodeCopy(XMLNode* node, XMLNode* parentAim) {
    XMLNode* clone = static_cast<XMLNode*>(node->ShallowClone(parentAim->GetDocument()));
    parentAim->InsertEndChild(clone);
    for (XMLNode* child = node->FirstChild(); child; child = child->NextSibling()) {
        insertNodeCopy(child, clone);
    }
    return clone;
}

// Copys and pastes "element" including all of its child elements to "parentAim"
// Returns the copied element.
XMLElement* insertElementCopy(XMLElement* element, XMLElement* parentAim) {
    return (XMLElement*)insertNodeCopy(element, parentAim);
}

// Returns the first child element of parent with the matching attribute "id"
XMLElement* getChildWithID(XMLElement* parent, const char* id) {
    for (XMLElement* child = parent->FirstChildElement(); child; child = child->NextSibling()->ToElement()) {
        if (child->Attribute("id")) {
            return child;
        }
        if (child->NextSibling() == nullptr)
            break;
    }
    return nullptr;
}

// Returns the first child element of parent with the matching attribute "attributeName"
XMLElement* firstChildWithAttribute(XMLElement* parent, const char* attributeName, const char* attributeValue) {
    for(XMLElement* child = parent->FirstChildElement(); child; child = child->NextSibling()->ToElement()) {
        if (child->Attribute(attributeName) && strcmp(child->Attribute(attributeName), attributeValue) == 0) {
            return child;
        }
        if (child->NextSibling() == nullptr)
            break;
    }
    return nullptr;
}


void pushAttributeNotEqual(XMLPrinter* printer, const char* key, const std::string& value, const std::string& standard) {
    if (value != standard) {
        printer->PushAttribute(key, value.c_str());
    }
}

void pushAttributeNotEqual(XMLPrinter* printer, const char* key, const float& value, const float& standard) {
    // Special case for floats: test if the values equal approximately
    if (!floatEquals(value, standard)) {
        printer->PushAttribute(key, value);
    }
}

}
