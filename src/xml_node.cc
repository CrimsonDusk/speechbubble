#include "xml_node.h"
#include "xml_document.h"

// =============================================================================
// -----------------------------------------------------------------------------
XMLNode::XMLNode (QString name, XMLNode* parent) :
	m_Name (name),
	m_IsCData (false),
	m_Parent (parent)
{
	if (parent)
		parent->m_Subnodes << this;
}

// =============================================================================
// -----------------------------------------------------------------------------
XMLNode::~XMLNode()
{	for (XMLNode* node : getSubnodes())
		delete node;

	if (getParent())
		getParent()->dropNode (this);
}

// =============================================================================
// -----------------------------------------------------------------------------
QString XMLNode::getAttribute (QString name) const
{	if (hasAttribute (name))
		return getAttributes()[name];

	return "";
}

// =============================================================================
// -----------------------------------------------------------------------------
void XMLNode::dropNode (XMLNode* node)
{	m_Subnodes.removeOne (node);
}

// =============================================================================
// -----------------------------------------------------------------------------
bool XMLNode::hasAttribute (QString name) const
{	return getAttributes().find (name) != getAttributes().end();
}

// =============================================================================
// -----------------------------------------------------------------------------
void XMLNode::setAttribute (QString name, QString data)
{	m_Attributes[name] = data;
}

// =============================================================================
// -----------------------------------------------------------------------------
XMLNode* XMLNode::findSubNode (QString fname, bool recursive)
{	for (XMLNode* node : getSubnodes())
	{	if (node->getName() == fname)
			return node;

		XMLNode* target;

		if (recursive && (target = node->findSubNode (fname)) != null)
			return target;
	}

	return null;
}

// =============================================================================
// -----------------------------------------------------------------------------
QList<XMLNode*> XMLNode::getNodesByName (QString name)
{	QList<XMLNode*> matches;

	for (XMLNode* node : getSubnodes())
		if (node->getName() == name)
			matches << node;

	return matches;
}

// =============================================================================
// -----------------------------------------------------------------------------
bool XMLNode::isEmpty() const
{	return getContents().isEmpty() && getSubnodes().isEmpty();
}

// =============================================================================
// -----------------------------------------------------------------------------
XMLNode* XMLNode::addSubNode (QString name, QString cont)
{	XMLNode* node = new XMLNode (name, this);

	if (cont.length() > 0)
		node->setContents (cont);

	return node;
}

// =============================================================================
// -----------------------------------------------------------------------------
QList<XMLNode*> XMLNode::getNodesByAttribute (QString attrname, QString attrvalue)
{	QList<XMLNode*> matches;

	for (XMLNode* node : getSubnodes())
		if (node->getAttribute (attrname) == attrvalue)
			matches << node;

	return matches;
}

// =============================================================================
// -----------------------------------------------------------------------------
XMLNode* XMLNode::getOneNodeByAttribute (QString attrname, QString attrvalue)
{	for (XMLNode* node : getSubnodes())
		if (node->getAttribute (attrname) == attrvalue)
			return node;

	return null;
}