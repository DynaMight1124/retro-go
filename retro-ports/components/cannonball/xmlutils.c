/***************************************************************************
    Xml Helper Functions

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "xmlutils.h"
#include "utils.h"

XMLNode* GetXmlDocNode(XMLDoc* doc, const char* path)
{
    XMLSearch search;
    XMLSearch_init_from_XPath(path, &search);

    int loop = 0;

    for (loop = 0; loop < doc->n_nodes; loop++)
    {
        XMLNode* node = XMLSearch_next(doc->nodes[loop], &search);

        if (node != NULL)
        {
            return node;
        }
    }

    return NULL;
}


const char* GetXMLDocValueString(XMLDoc* doc, const char* path, const char* defaultValue)
{
    XMLNode* node = GetXmlDocNode(doc, path);
    
    if (node != 0)
    {
        return node->text;
    }   

    return defaultValue;
}

int GetXMLDocValueInt(XMLDoc* doc, const char* path, int defaultValue)
{
    XMLNode* node = GetXmlDocNode(doc, path);

    if (node != 0)
    {
        int value;
        sscanf(node->text, "%d", &value);
        return value;
    }   

    return defaultValue;
}


const char* GetXmlDocAttributeString(XMLDoc* doc, const char* path, const char* attribute, const char* defaultValue)
{
    XMLNode* node = GetXmlDocNode(doc, path);

    if (node)
    {
        SXML_CHAR* attributeValue;
        XMLNode_get_attribute_with_default(node, attribute, &attributeValue, defaultValue);
        return attributeValue;
    }

    return defaultValue;
}


int GetXmlDocAttributeInt(XMLDoc* doc, const char* path, const char* attribute, int defaultValue)
{
    const char* result = GetXmlDocAttributeString(doc, path, attribute, Utils_int_to_string(defaultValue));

    int value;
    sscanf(result, "%d", &value);
    return value;
}


void AddXmlHeader(XMLDoc* doc)
{
    XMLNode* node = XMLNode_alloc();
    XMLNode_set_tag(node, "xml version=\"1.0\" encoding=\"utf-8\"");
    XMLNode_set_type(node, TAG_INSTR);
    XMLDoc_add_node(doc, node);
}

XMLNode* AddXmlFatherNode(XMLDoc* doc, const char* nodeName)
{
    XMLNode* node = XMLNode_alloc();
    XMLNode_set_tag(node, nodeName);
    XMLNode_set_type(node, TAG_FATHER);
    XMLDoc_add_node(doc, node);
    return node;
}

void AddXmlChildNodeRootString(XMLDoc* doc, const char* nodeName, const char* text)
{
    XMLNode* node = XMLNode_alloc();
    XMLNode_set_tag(node, nodeName);
    XMLNode_set_text(node, text);
    XMLDoc_add_child_root(doc, node);
}

void AddXmlChildNodeRootInt(XMLDoc* doc, const char* nodeName, int value)
{
    AddXmlChildNodeRootString(doc, nodeName, Utils_int_to_string(value));
}

XMLNode* AddNode(XMLDoc* doc, XMLNode* parent, const char* nodeName)
{
    XMLNode* node = XMLNode_alloc();
    XMLNode_set_tag(node, nodeName);

    if (parent == NULL)
    {
        parent = doc->nodes[doc->i_root];
    }

    XMLNode_add_child(parent, node);

    return node;
}

XMLNode* AddNodeString(XMLDoc* doc, XMLNode* parent, const char* nodeName, const char* text)
{
    XMLNode* node = AddNode(doc, parent, nodeName);
    XMLNode_set_text(node, text);

    return node;
}

XMLNode* AddNodeInt(XMLDoc* doc, XMLNode* parent, const char* nodeName, int value)
{
    return AddNodeString(doc, parent, nodeName, Utils_int_to_string(value));
}

XMLNode* AddNodeAttributeString(XMLDoc* doc, XMLNode* parent, const char* nodeName, const char* attributeName, const char* text)
{
    XMLNode* node = AddNode(doc, parent, nodeName);

    XMLNode_set_attribute(node, attributeName, text);

    return node;
}

XMLNode* AddNodeAttributeInt(XMLDoc* doc, XMLNode* parent, const char* nodeName, const char* attributeName, int value)
{
    return AddNodeAttributeString(doc, parent, nodeName, attributeName, Utils_int_to_string(value));
}