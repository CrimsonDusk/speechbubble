#ifndef LIBCOBALT_XML_H
#define LIBCOBALT_XML_H

#include "main.h"
#include "xml_node.h"

// =============================================================================
// -----------------------------------------------------------------------------
class XMLDocument
{	public:
		typedef QMap<QString, QString> HeaderType;

	PROPERTY (protected, HeaderType, Header)
	PROPERTY (private,   XMLNode*,   Root)

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

// =============================================================================
// -----------------------------------------------------------------------------
class XMLError : public std::exception
{	PROPERTY (private, QString,	Error)
public:
	XMLError (QString err) : m_Error (err) {}
	
	inline const char* what() const throw()
	{	return getError().toLocal8Bit().constData();
	}
};

#endif // LIBCOBALT_XML_H
