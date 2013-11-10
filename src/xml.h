#ifndef LIBCOBALT_XML_H
#define LIBCOBALT_XML_H

#include <climits>
#include <qmap.h>
#include "main.h"

class XMLNode;

// =============================================================================
// -----------------------------------------------------------------------------
class XMLDocument
{	public:
		typedef QMap<QString, QString> HeaderType;

	PROPERTY (protected, HeaderType, header)
	PROPERTY (private,   XMLNode*,   root)

	public:
		XMLDocument (XMLNode* root = null);
		~XMLDocument();

		XMLNode*                find_node_by_name (QString name) const;
		XMLNode*                navigate_to (const QStringList& path, bool allowMake = false) const;
		bool                    save (QString fname) const;
		void                    set_root (XMLNode* root);

		static QString          encode (QString in);
		static QString          decode (QString in);
		static QString          get_parse_error();
		static XMLDocument*     load (QString fname);
		static XMLDocument*     new_document (QString rootName);

	private:
		void                    writeNode (FILE* fp, const XMLNode* node) const;
};

// =============================================================================
// -----------------------------------------------------------------------------
class XMLNode
{	public:
		typedef QMap<QString, QString> AttributesType;

	PROPERTY (public,    QString,         contents);
	PROPERTY (public,    QString,         name)
	PROPERTY (protected, QList<XMLNode*>, nodes);
	PROPERTY (protected, AttributesType,  attributes);
	PROPERTY (protected, bool,            is_cdata);
	PROPERTY (protected, XMLNode*,        parent);

	public:
		XMLNode (QString name, XMLNode* parent);
		~XMLNode();

		XMLNode*				addSubNode (QString name, QString cont = "");
		QString				attribute (QString name) const;
		void					dropNode (XMLNode* node);
		XMLNode*				findSubNode (QString fname, bool recursive = false);
		QList<XMLNode*>	getNodesByAttribute (QString attrname, QString attrvalue);
		XMLNode*				getOneNodeByAttribute (QString attrname, QString attrvalue);
		QList<XMLNode*>	getNodesByName (QString name);
		bool					hasAttribute (QString name) const;
		bool					empty() const;
		void					setAttribute (QString name, QString data);

	protected:
		friend class XMLDocument;
};

#endif // LIBCOBALT_XML_H
