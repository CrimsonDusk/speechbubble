#ifndef LIBCOBALT_XML_H
#define LIBCOBALT_XML_H

#include "main.h"
#include "xml_node.h"

// =============================================================================
//
class XMLDocument
{
public:
	typedef QMap<QString, QString> HeaderType;

	PROPERTY (protected, HeaderType, header,	setHeader,	STOCK_WRITE)
	PROPERTY (private,   XMLNode*,   root,		setRoot,	STOCK_WRITE)

public:
	XMLDocument (XMLNode* root = null);
	~XMLDocument();

	XMLNode*                findNodeByName (QString name) const;
	XMLNode*                navigateTo (const QStringList& path, bool allowMake = false) const;
	bool                    saveToFile (QString fname) const;

	static QString          encodeString (QString in);
	static QString          decodeString (QString in);
	static QString          getParseError();
	static XMLDocument*     loadFromFile (QString fname);
	static XMLDocument*     newDocument (QString rootName);

private:
	void                    writeNode (FILE* fp, const XMLNode* node) const;
};

#endif // LIBCOBALT_XML_H
