#ifndef XML_NODE_H
#define XML_NODE_H

#include "main.h"

// =============================================================================
// -----------------------------------------------------------------------------
class XMLNode
{	public:
		typedef QMap<QString, QString> AttributeMap;

	PROPERTY (public,    QString,			Contents);
	PROPERTY (public,    QString,			Name)
	PROPERTY (protected, QList<XMLNode*>,	Subnodes);
	PROPERTY (protected, AttributeMap,		Attributes);
	PROPERTY (protected, bool,				IsCData);
	PROPERTY (protected, XMLNode*,			Parent);

	public:
		XMLNode (QString name, XMLNode* parent);
		~XMLNode();

		XMLNode*				addSubNode (QString name, QString cont);
		QString				getAttribute (QString name) const;
		void					dropNode (XMLNode* node);
		XMLNode*				findSubNode (QString fname, bool recursive = false);
		QList<XMLNode*>	getNodesByAttribute (QString attrname, QString attrvalue);
		XMLNode*				getOneNodeByAttribute (QString attrname, QString attrvalue);
		QList<XMLNode*>	getNodesByName (QString name);
		bool					hasAttribute (QString name) const;
		bool					isEmpty() const;
		void					setAttribute (QString name, QString data);

	protected:
		friend class XMLDocument;
};

#endif // XML_NODE_H