#include <cstdio>
#include <qstringlist.h>
#include "main.h"
#include "xml_document.h"
#include "format.h"
#include "xml_scanner.h"
#include "misc.h"

// =============================================================================
// -----------------------------------------------------------------------------
static QString					g_XMLError;
static QVector<XMLNode*>	g_Stack;
static int						g_SaveStack;

static const struct
{	const char* decoded, *encoded;
} g_encodingConversions[] =
{	{"&",  "&amp;"},
	{"<",  "&lt;"},
	{">",  "&gt;"},
	{"\"", "&quot;"}
};

// =============================================================================
// -----------------------------------------------------------------------------
static XMLNode* topStackNode()
{	if (g_Stack.size() == 0)
		return null;

	return g_Stack.last();
}

// =============================================================================
// -----------------------------------------------------------------------------
XMLDocument::XMLDocument (XMLNode* root) :
	m_Root (root)
{	m_Header["version"] = "1.0";
	m_Header["encoding"] = "UTF-8";
}

// =============================================================================
// -----------------------------------------------------------------------------
XMLDocument::~XMLDocument()
{	delete m_Root;
}

// =============================================================================
// -----------------------------------------------------------------------------
XMLDocument* XMLDocument::newDocument (QString rootName)
{	return new XMLDocument (new XMLNode (rootName, null));
}

// =============================================================================
// -----------------------------------------------------------------------------
XMLDocument* XMLDocument::loadFromFile (QString fname)
{	FILE*			fp = null;
	long			fsize;
	char*			buf = null;
	XMLNode*		root = null;
	HeaderType	header;

	try
	{	if ((fp = fopen (fname.toStdString().c_str(), "r")) == null)
			throw XMLError (fmt ("couldn't open %1 for reading: %2", fname, strerror (errno)));

		fseek (fp, 0l, SEEK_END);
		fsize = ftell (fp);
		rewind (fp);
		buf = new char[fsize + 1];

		if ((long) fread (buf, 1, fsize, fp) < fsize)
			throw XMLError (fmt ("I/O error while opening %1", fname));

		buf[fsize] = '\0';
		fclose (fp);
		fp = null;
		XMLScanner scan (buf);
		scan.mustScanNext (XMLScanner::EHeaderStart);

		while (scan.scanNextToken (XMLScanner::ESymbol))
		{	QString attrname = scan.getToken();
			scan.mustScanNext (XMLScanner::EEquals);
			scan.mustScanNext (XMLScanner::EString);
			header[attrname] = scan.getToken();
		}

		scan.mustScanNext (XMLScanner::EHeaderEnd);

		if (header.find ("version") == header.end())
			throw XMLError ("No version defined in header!");

		while (scan.scanNextToken())
		{	switch (scan.getTokenType())
			{	case XMLScanner::ETagStart:
				{	scan.mustScanNext (XMLScanner::ESymbol);
					XMLNode* node = new XMLNode (scan.getToken(), topStackNode());

					if (g_Stack.size() == 0)
					{	if (root != null)
						{	// XML forbids having multiple roots
							delete node;
							throw XMLError ("Multiple root nodes");
						}

						root = node;
					}

					g_Stack << node;

					while (scan.scanNextToken (XMLScanner::ESymbol))
					{	QString attrname = scan.getToken();
						scan.mustScanNext (XMLScanner::EEquals);
						scan.mustScanNext (XMLScanner::EString);
						node->setAttribute (attrname, scan.getToken());
						assert (node->hasAttribute (attrname));
					}

					if (scan.scanNextToken (XMLScanner::ETagSelfCloser))
					{	XMLNode* popee;
						assert (pop (g_Stack, popee) && popee == node);
					}
					else
						scan.mustScanNext (XMLScanner::ETagEnd);
				} break;

				case XMLScanner::ETagCloser:
				{	scan.mustScanNext (XMLScanner::ESymbol);
					XMLNode* popee;

					if (!pop (g_Stack, popee) || popee->getName() != scan.getToken())
						throw XMLError ("Misplaced closing tag");

					scan.mustScanNext (XMLScanner::ETagEnd);
				} break;

				case XMLScanner::ECData:
				case XMLScanner::ESymbol:
				{	if (g_Stack.size() == 0)
						throw XMLError ("Misplaced CDATA/symbol");

						XMLNode* node = g_Stack[g_Stack.size() - 1];

					node->setIsCData (scan.getTokenType() == XMLScanner::ECData);
					node->setContents (node->getIsCData() ? decodeString (scan.getToken()) : scan.getToken());
				} break;

				case XMLScanner::EString:
				case XMLScanner::EHeaderStart:
				case XMLScanner::EHeaderEnd:
				case XMLScanner::EEquals:
				case XMLScanner::ETagSelfCloser:
				case XMLScanner::ETagEnd:
				{	throw XMLError (fmt ("Unexpected token '%1'", scan.getToken()));
				} break;
			}
		}
	} catch (XMLError& e)
	{	g_XMLError = e.getError();
		delete[] buf;
		delete root;

		if (fp)
			fclose (fp);

		return null;
	}

	delete[] buf;
	XMLDocument* doc = new XMLDocument (root);
	doc->setHeader (header);
	return doc;
}

// =============================================================================
// -----------------------------------------------------------------------------
bool XMLDocument::saveToFile (QString fname) const
{	FILE* fp;

	if ((fp = fopen (fname.toStdString().c_str(), "w")) == null)
		return false;

	flog (fp, "<?xml");

	for (auto it = getHeader().begin(); it != getHeader().end(); ++it)
		flog (fp, " %1=\"%2\"", it.key(), it.value());

	flog (fp, " ?>\n");
	g_SaveStack = 0;
	writeNode (fp, getRoot());
	fclose (fp);
	return true;
}

// =============================================================================
// -----------------------------------------------------------------------------
void XMLDocument::writeNode (FILE* fp, const XMLNode* node) const
{	QString indent;

	for (int i = 0; i < g_SaveStack; ++i)
		indent += "\t";

	flog (fp, "%1<%2", indent, node->getName());

	for (auto it = node->getAttributes().begin(); it != node->getAttributes().end(); ++it)
		flog (fp, " %1=\"%2\"", encodeString (it.key()), encodeString (it.value()));

	if (node->isEmpty() && g_SaveStack > 0)
	{	flog (fp, " />\n");
		return;
	}

	flog (fp, ">");

	if (node->getSubnodes().size() > 0)
	{	// Write nodes
		flog (fp, "\n");

		for (const XMLNode* subnode : node->getSubnodes())
		{	g_SaveStack++;
			writeNode (fp, subnode);
			g_SaveStack--;
		}

		flog (fp, indent);
	}
	else
	{	// Write content
		if (node->getIsCData())
			flog (fp, "<![CDATA[%1]]>", node->getContents());
		else
			flog (fp, encodeString (node->getContents()));
	}

	flog (fp, "</%1>\n", node->getName());
}

// =============================================================================
// -----------------------------------------------------------------------------
QString XMLDocument::encodeString (QString in)
{	QString out (in);

	for (auto i : g_encodingConversions)
		out.replace (i.decoded, i.encoded);

	return out;
}

// =============================================================================
// -----------------------------------------------------------------------------
QString XMLDocument::decodeString (QString in)
{	QString out (in);

	for (auto i : g_encodingConversions)
		out.replace (i.encoded, i.decoded);

	return out;
}

// =============================================================================
// -----------------------------------------------------------------------------
XMLNode* XMLDocument::findNodeByName (QString name) const
{	return getRoot()->findSubNode (name, true);
}

// =============================================================================
// -----------------------------------------------------------------------------
XMLNode* XMLDocument::navigateTo (const QStringList& path, bool allowMake) const
{	XMLNode* node = getRoot();

	for (QString name : path)
	{	XMLNode* parent = node;
		node = parent->findSubNode (name);

		if (!node)
		{	if (allowMake)
				node = new XMLNode (name, parent);
			else
				return null;
		}
	}

	return node;
}

// =============================================================================
// -----------------------------------------------------------------------------
QString XMLDocument::getParseError()
{	return g_XMLError;
}