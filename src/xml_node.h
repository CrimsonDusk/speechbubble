#ifndef XML_NODE_H
#define XML_NODE_H

#include "main.h"

// =============================================================================
//
class XMLNode
{
public:
	using AttributeMap = QMap<QString, QString>;

	PROPERTY (public,    QString,			contents,	setContents,	STOCK_WRITE);
	PROPERTY (public,    QString,			name,		setName,		STOCK_WRITE)
	PROPERTY (protected, QList<XMLNode*>,	subNodes,	setSubNodes,	STOCK_WRITE);
	PROPERTY (protected, AttributeMap,		attributes,	setAttributes,	STOCK_WRITE);
	PROPERTY (protected, bool,				isCData,	setCData,		STOCK_WRITE);
	PROPERTY (protected, XMLNode*,			parent,		setParent,		STOCK_WRITE);

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

protected:
	friend class XMLDocument;
};

#endif // XML_NODE_H
