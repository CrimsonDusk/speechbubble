#ifndef XML_NODE_H
#define XML_NODE_H

#include "main.h"

// =============================================================================
// -----------------------------------------------------------------------------
class XMLNode
{	public:
		typedef QMap<QString, QString> AttributeMap;

	NEW_PROPERTY (public,    QString,			Contents);
	NEW_PROPERTY (public,    QString,			Name)
	NEW_PROPERTY (protected, QList<XMLNode*>,	Subnodes);
	NEW_PROPERTY (protected, AttributeMap,		Attributes);
	NEW_PROPERTY (protected, bool,				IsCData);
	NEW_PROPERTY (protected, XMLNode*,			Parent);

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