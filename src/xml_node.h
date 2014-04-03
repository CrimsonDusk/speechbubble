#ifndef XML_NODE_H
#define XML_NODE_H

#include "main.h"

// =============================================================================
//
class XMLNode
{
public:
	PROPERTY (QString contents)
	PROPERTY (QString name)
	PROPERTY (QList<XMLNode*> subNodes)
	PROPERTY (StringMap attributes)
	PROPERTY (bool isCData)
	PROPERTY (XMLNode* parent)
	CLASSDATA (XMLNode)

public:
	XMLNode (QString name, XMLNode* parent);
	~XMLNode();

	XMLNode*				addSubNode (QString name, QString cont);
	QString					getAttribute (QString name) const;
	void					dropNode (XMLNode* node);
	XMLNode*				findSubNode (QString fname, bool recursive = false);
	QList<XMLNode*>			getNodesByAttribute (QString attrname, QString attrvalue);
	XMLNode*				getOneNodeByAttribute (QString attrname, QString attrvalue);
	QList<XMLNode*>			getNodesByName (QString name);
	bool					hasAttribute (QString name) const;
	bool					isEmpty() const;
	void					setAttribute (QString name, QString data);
};

#endif // XML_NODE_H
