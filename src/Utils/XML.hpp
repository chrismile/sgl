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

#ifndef UTILS_XML_HPP_
#define UTILS_XML_HPP_

#include <string>
#include <cassert>
#include <functional>
#include <tinyxml2.h>

using namespace tinyxml2;

namespace sgl {

/*! Copys and pastes "element" including all of its child elements to "parentAim"
 * Returns the copied element. */
DLL_OBJECT XMLElement *insertElementCopy(XMLElement *element, XMLElement *parentAim);

//! Returns the first child element of parent with the matching attribute "id"
DLL_OBJECT XMLElement *getChildWithID(XMLElement *parent, const char *id);

//! Returns the first child element of parent with the matching attribute "attributeName"
DLL_OBJECT XMLElement *firstChildWithAttribute(XMLElement *parent, const char *attributeName, const char *attributeValue);

/*! Pushes the "key" with the desired value on the XMLPrinter stack if "value" doesn't equal "standard"
 * Example: pushAttributeNotEqual(printer, "damping", damping, 0.0f); */
template<class T>
void pushAttributeNotEqual(XMLPrinter *printer, const char *key, const T &value, const T &standard)
{
    if (value != standard) {
        printer->PushAttribute(key, value);
    }
}
DLL_OBJECT void pushAttributeNotEqual(
        XMLPrinter *printer, const char *key, const std::string &value, const std::string &standard);
DLL_OBJECT void pushAttributeNotEqual(
        XMLPrinter *printer, const char *key, const float &value, const float &standard);



//! Classes for easily iterating over XMLElements

typedef std::function<bool(tinyxml2::XMLElement*)> XMLItFilterFunc;
struct DLL_OBJECT XMLItFilter {
public:
    XMLItFilter(std::function<bool(tinyxml2::XMLElement*)> f) { filterFunc = f; }
    XMLItFilter() : XMLItFilter([](tinyxml2::XMLElement* e) { return true; }) {}
    bool operator()(tinyxml2::XMLElement* element) { return filterFunc(element); }

private:
    XMLItFilterFunc filterFunc;
};

//! E.g.: Name equals X, Attribute Y equals X
inline XMLItFilter XMLNameFilter(const std::string& name) {
    return XMLItFilter([name](tinyxml2::XMLElement* e) -> bool { return name == e->Name(); });
}
inline XMLItFilter XMLAttributeFilter(const std::string& attrName, const std::string& attrVal) {
    return XMLItFilter([attrName,attrVal](tinyxml2::XMLElement* e) -> bool {
        return attrVal == e->Attribute(attrName.c_str());
    });
}
inline XMLItFilter XMLAttributePresenceFilter(const std::string& attrName) {
    return XMLItFilter([attrName](tinyxml2::XMLElement* e) -> bool {
        return e->Attribute(attrName.c_str()) != NULL;
    });
}


class DLL_OBJECT XMLIterator : public std::iterator<std::input_iterator_tag, tinyxml2::XMLElement> {
private:
    tinyxml2::XMLElement *element;
    XMLItFilter filter;

public:
    XMLIterator(tinyxml2::XMLElement* e, XMLItFilter f) : filter(f) {
        element = e->FirstChildElement();
        if (element != NULL && !filter(element)) {
            operator++();
        }
    }
    XMLIterator(tinyxml2::XMLElement* e) : XMLIterator(e, XMLItFilter()) { }
    XMLIterator(const XMLIterator& otherIt) : XMLIterator(otherIt.element) {}

    XMLIterator& operator++() {
        do {
            tinyxml2::XMLNode* next = element->NextSibling();
            element = next ? next->ToElement() : NULL;
        } while (element != NULL && !filter(element));
        return *this;
    }
    bool operator==(const XMLIterator& rhs) { return element==rhs.element; }
    bool operator!=(const XMLIterator& rhs) { return element!=rhs.element; }
    tinyxml2::XMLElement* operator*() { assert(element); return element; }
    tinyxml2::XMLElement* operator->() { assert(element); return element; }
    bool isValid() { return element != NULL; }
};

}

/*! UTILS_XML_HPP_ */
#endif
