#include <cstdio>
#include <QVector>
#include <QStringList>
#include "main.h"
#include "xml_document.h"
#include "format.h"
#include "xml_scanner.h"
#include "misc.h"

// =============================================================================
//
static QString				g_errorString;
static QVector<XMLNode*>	g_stack;
static int					g_saveStack;

static const struct
{
	const char* decoded, *encoded;
} g_encodingConversions[] =
{
	{"&",  "&amp;"},
	{"<",  "&lt;"},
	{">",  "&gt;"},
	{"\"", "&quot;"}
};

// =============================================================================
//
static XMLNode* topStackNode()
{
	if (g_stack.size() == 0)
		return null;

	return g_stack.last();
}

// =============================================================================
//
XMLDocument::XMLDocument (XMLNode* root) :
	m_root (root)
{
	m_header["version"] = "1.0";
	m_header["encoding"] = "UTF-8";
}

// =============================================================================
//
XMLDocument::~XMLDocument()
{
	delete m_root;
}

// =============================================================================
//
XMLDocument* XMLDocument::newDocument (QString rootName)
{
	return new XMLDocument (new XMLNode (rootName, null));
}

// =============================================================================
//
XMLDocument* XMLDocument::loadFromFile (QString fname)
{
	FILE*			fp = null;
	long			fsize;
	char*			buf = null;
	XMLNode*		root = null;
	HeaderType		header;

	try
	{
		if ((fp = fopen (fname.toStdString().c_str(), "r")) == null)
			throw format ("couldn't open %1 for reading: %2", fname, strerror (errno));

		fseek (fp, 0l, SEEK_END);
		fsize = ftell (fp);
		rewind (fp);
		buf = new char[fsize + 1];

		if ((long) fread (buf, 1, fsize, fp) < fsize)
			throw format ("I/O error while opening %1", fname);

		buf[fsize] = '\0';
		fclose (fp);
		fp = null;
		XMLScanner scan (buf);
		scan.mustScanNext (XMLScanner::EHeaderStart);

		while (scan.scanNextToken (XMLScanner::ESymbol))
		{
			QString attrname = scan.token();
			scan.mustScanNext (XMLScanner::EEquals);
			scan.mustScanNext (XMLScanner::EString);
			header[attrname] = scan.token();
		}

		scan.mustScanNext (XMLScanner::EHeaderEnd);

		if (header.find ("version") == header.end())
			throw QString ("No version defined in header!");

		while (scan.scanNextToken())
		{
			switch (scan.tokenType())
			{
				case XMLScanner::ETagStart:
				{
					scan.mustScanNext (XMLScanner::ESymbol);
					XMLNode* node = new XMLNode (scan.token(), topStackNode());

					if (g_stack.size() == 0)
					{
						if (root != null)
						{
							// XML forbids having multiple roots
							delete node;
							throw std::logic_error ("Multiple root nodes");
						}

						root = node;
					}

					g_stack << node;

					while (scan.scanNextToken (XMLScanner::ESymbol))
					{
						QString attrname = scan.token();
						scan.mustScanNext (XMLScanner::EEquals);
						scan.mustScanNext (XMLScanner::EString);
						node->setAttribute (attrname, scan.token());
						assert (node->hasAttribute (attrname));
					}

					if (scan.scanNextToken (XMLScanner::ETagSelfCloser))
					{
						XMLNode* popee;
						assert (pop (g_stack, popee) && popee == node);
					}
					else
						scan.mustScanNext (XMLScanner::ETagEnd);
				}
				break;

				case XMLScanner::ETagCloser:
				{
					scan.mustScanNext (XMLScanner::ESymbol);
					XMLNode* popee;

					if (!pop (g_stack, popee) || popee->name() != scan.token())
						throw std::logic_error ("Misplaced closing tag");

					scan.mustScanNext (XMLScanner::ETagEnd);
				}
				break;

				case XMLScanner::ECData:
				case XMLScanner::ESymbol:
				{
					if (g_stack.size() == 0)
						throw std::logic_error ("Misplaced CDATA/symbol");

					XMLNode* node = g_stack[g_stack.size() - 1];

					node->setCData (scan.tokenType() == XMLScanner::ECData);
					node->setContents (node->isCData() ? decodeString (scan.token()) : scan.token());
				}
				break;

				case XMLScanner::EString:
				case XMLScanner::EHeaderStart:
				case XMLScanner::EHeaderEnd:
				case XMLScanner::EEquals:
				case XMLScanner::ETagSelfCloser:
				case XMLScanner::ETagEnd:
					throw format ("Unexpected token '%1'", scan.token());
					break;
			}
		}
	}
	catch (QString e)
	{
		g_errorString = e;
		delete[] buf;
		delete root;

		if (fp != null)
			fclose (fp);

		return null;
	}

	delete[] buf;
	XMLDocument* doc = new XMLDocument (root);
	doc->setHeader (header);
	return doc;
}

// =============================================================================
//
bool XMLDocument::saveToFile (QString fname) const
{
	FILE* fp;

	if ( (fp = fopen (fname.toStdString().c_str(), "w")) == null)
		return false;

	fprint (fp, "<?xml");

	for (auto it = header().begin(); it != header().end(); ++it)
		fprint (fp, " %1=\"%2\"", it.key(), it.value());

	fprint (fp, " ?>\n");
	g_saveStack = 0;
	writeNode (fp, root());
	fclose (fp);
	return true;
}

// =============================================================================
//
void XMLDocument::writeNode (FILE* fp, const XMLNode* node) const
{
	QString indent;

	for (int i = 0; i < g_saveStack; ++i)
		indent += "\t";

	fprint (fp, "%1<%2", indent, node->name());

	for (auto it = node->attributes().begin(); it != node->attributes().end(); ++it)
		fprint (fp, " %1=\"%2\"", encodeString (it.key()), encodeString (it.value()));

	if (node->isEmpty() && g_saveStack > 0)
	{
		fprint (fp, " />\n");
		return;
	}

	fprint (fp, ">");

	if (node->subNodes().size() > 0)
	{
		// Write nodes
		fprint (fp, "\n");

		for (const XMLNode * subnode : node->subNodes())
		{
			g_saveStack++;
			writeNode (fp, subnode);
			g_saveStack--;
		}

		fprint (fp, indent);
	}
	else
	{
		// Write content
		if (node->isCData())
			fprint (fp, "<![CDATA[%1]]>", node->contents());
		else
			fprint (fp, encodeString (node->contents()));
	}

	fprint (fp, "</%1>\n", node->name());
}

// =============================================================================
//
QString XMLDocument::encodeString (QString in)
{
	QString out (in);

	for (auto i : g_encodingConversions)
		out.replace (i.decoded, i.encoded);

	return out;
}

// =============================================================================
//
QString XMLDocument::decodeString (QString in)
{
	QString out (in);

	for (auto i : g_encodingConversions)
		out.replace (i.encoded, i.decoded);

	return out;
}

// =============================================================================
//
XMLNode* XMLDocument::findNodeByName (QString name) const
{
	return root()->findSubNode (name, true);
}

// =============================================================================
//
XMLNode* XMLDocument::navigateTo (const QStringList& path, bool allowMake) const
{
	XMLNode* node = root();

	for (QString name : path)
	{
		XMLNode* parent = node;
		node = parent->findSubNode (name);

		if (!node)
		{
			if (allowMake)
				node = new XMLNode (name, parent);
			else
				return null;
		}
	}

	return node;
}

// =============================================================================
//
QString XMLDocument::getParseError()
{
	return g_errorString;
}
