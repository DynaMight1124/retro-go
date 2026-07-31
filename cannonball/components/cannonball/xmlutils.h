/***************************************************************************
    Xml Helper Functions

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include "stdint.h"
#include "thirdparty/sxmlc/sxmlc.h"
#include "thirdparty/sxmlc/sxmlsearch.h"

const char* GetXMLDocValueString(XMLDoc* doc, const char* path, const char* defaultValue);
int GetXMLDocValueInt(XMLDoc* doc, const char* path, int defaultValue);
const char* GetXmlDocAttributeString(XMLDoc* doc, const char* path, const char* attribute, const char* defaultValue);
int GetXmlDocAttributeInt(XMLDoc* doc, const char* path, const char* attribute, int defaultValue);
void AddXmlHeader(XMLDoc* doc);
XMLNode* AddXmlFatherNode(XMLDoc* doc, const char* nodeName);
void AddXmlChildNodeRootString(XMLDoc* doc, const char* nodeName, const char* text);
void AddXmlChildNodeRootInt(XMLDoc* doc, const char* nodeName, int value);
XMLNode* AddNode(XMLDoc* doc, XMLNode* parent, const char* nodeName);
XMLNode* AddNodeString(XMLDoc* doc, XMLNode* parent, const char* nodeName, const char* text);
XMLNode* AddNodeInt(XMLDoc* doc, XMLNode* parent, const char* nodeName, int value);
XMLNode* AddNodeAttributeString(XMLDoc* doc, XMLNode* parent, const char* nodeName, const char* attributeName, const char* text);
XMLNode* AddNodeAttributeInt(XMLDoc* doc, XMLNode* parent, const char* nodeName, const char* attributeName, int value);
