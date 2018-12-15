/*
 * XML.cpp
 *
 *  Created on: 02.10.2016
 *      Author: Christoph Neuhauser
 */

#include "XML.hpp"
#include <Math/Math.hpp>
#include <cstddef>

namespace sgl {

XMLNode *insertNodeCopy(XMLNode *node, XMLNode *parentAim)
{
	XMLNode *clone = static_cast<XMLNode*>(node->ShallowClone(parentAim->GetDocument()));
	parentAim->InsertEndChild(clone);
	for (XMLNode *child = node->FirstChild(); child; child = child->NextSibling()) {
		insertNodeCopy(child, clone);
	}
	return clone;
}

// Copys and pastes "element" including all of its child elements to "parentAim"
// Returns the copied element.
XMLElement *insertElementCopy(XMLElement *element, XMLElement *parentAim)
{
	return (XMLElement*)insertNodeCopy(element, parentAim);
}

// Returns the first child element of parent with the matching attribute "id"
XMLElement *getChildWithID(XMLElement *parent, const char *id)
{
	for (XMLElement *child = parent->FirstChildElement(); child; child = child->NextSibling()->ToElement()) {
		if (child->Attribute("id")) {
			return child;
		}
		if (child->NextSibling() == 0)
			break;
	}
	return NULL;
}

// Returns the first child element of parent with the matching attribute "attributeName"
XMLElement *firstChildWithAttribute(XMLElement *parent, const char *attributeName, const char *attributeValue)
{
	for(XMLElement *child = parent->FirstChildElement(); child; child = child->NextSibling()->ToElement()) {
		if (child->Attribute(attributeName) && strcmp(child->Attribute(attributeName), attributeValue) == 0) {
			return child;
		}
		if (child->NextSibling() == 0)
			break;
	}
	return NULL;
}


void pushAttributeNotEqual(XMLPrinter *printer, const char *key, const std::string &value, const std::string &standard)
{
	if (value != standard) {
		printer->PushAttribute(key, value.c_str());
	}
}

void pushAttributeNotEqual(XMLPrinter *printer, const char *key, const float &value, const float &standard)
{
	// Special case for floats: test if the values equal approximately
	if (!floatEquals(value, standard)) {
		printer->PushAttribute(key, value);
	}
}

}
