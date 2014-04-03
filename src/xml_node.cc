#include "xml_node.h"
#include "xml_document.h"

// =============================================================================
//
XMLNode::XMLNode (QString name, XMLNode* parent) :
	name (name),
	isCData (false),
	parent (parent)
{
	if (parent)
		parent->subNodes << this;
}

// =============================================================================
//
XMLNode::~XMLNode()
{
	for (XMLNode* node : subNodes)
		delete node;

	if (parent)
		parent->dropNode (this);
}

// =============================================================================
//
QString XMLNode::getAttribute (QString name) const
{
	if (hasAttribute (name))
		return attributes[name];

	return "";
}

// =============================================================================
//
void XMLNode::dropNode (XMLNode* node)
{
	subNodes.removeOne (node);
}

// =============================================================================
//
bool XMLNode::hasAttribute (QString name) const
{
	return attributes.find (name) != attributes.end();
}

// =============================================================================
//
void XMLNode::setAttribute (QString name, QString data)
{
	attributes[name] = data;
}

// =============================================================================
//
XMLNode* XMLNode::findSubNode (QString fname, bool recursive)
{
	for (XMLNode* node : subNodes)
	{
		if (node->name == fname)
			return node;

		XMLNode* target;

		if (recursive && (target = node->findSubNode (fname)) != null)
			return target;
	}

	return null;
}

// =============================================================================
//
QList<XMLNode*> XMLNode::getNodesByName (QString name)
{
	QList<XMLNode*> matches;

	for (XMLNode* node : subNodes)
		if (node->name == name)
			matches << node;

	return matches;
}

// =============================================================================
//
bool XMLNode::isEmpty() const
{
	return contents.isEmpty() && subNodes.isEmpty();
}

// =============================================================================
//
XMLNode* XMLNode::addSubNode (QString name, QString cont)
{
	XMLNode* node = new XMLNode (name, this);

	if (cont.length() > 0)
		node->contents = cont;

	return node;
}

// =============================================================================
//
QList<XMLNode*> XMLNode::getNodesByAttribute (QString attrname, QString attrvalue)
{
	QList<XMLNode*> matches;

	for (XMLNode* node : subNodes)
		if (node->getAttribute (attrname) == attrvalue)
			matches << node;

	return matches;
}

// =============================================================================
//
XMLNode* XMLNode::getOneNodeByAttribute (QString attrname, QString attrvalue)
{
	for (XMLNode* node : subNodes)
		if (node->getAttribute (attrname) == attrvalue)
			return node;

	return null;
}
