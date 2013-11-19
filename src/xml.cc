#include <cstdio>
#include <qstringlist.h>
#include "main.h"
#include "xml.h"
#include "format.h"
#include "xml_scanner.h"

static QString              g_xml_error;
static QVector<XMLNode*>    g_stack;
static int                  g_save_stack;

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
{	if (g_stack.size() == 0)
		return null;

	return g_stack.last();
}

// =============================================================================
// -----------------------------------------------------------------------------
#define XML_ERROR(...) { \
	g_xml_error = fmt ("Line %1: ", scan.line()) + fmt (__VA_ARGS__) + fmt (" (token: '%1')", scan.token()); \
	delete[] buf; \
	delete root; \
	return null; }

#define XML_MUST_GET(N) \
	if (!scan.next (XMLScanner::E##N)) XML_ERROR ("Expected " #N)

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
XMLDocument* XMLDocument::NewDocument (QString rootName)
{	return new XMLDocument (new XMLNode (rootName, null));
}

// =============================================================================
// -----------------------------------------------------------------------------
XMLDocument* XMLDocument::LoadFromFile (QString fname)
{	FILE*			fp;
	long			fsize;
	char*			buf;
	XMLNode*		root = null;
	HeaderType	header;

	if ((fp = fopen (fname.toStdString().c_str(), "r")) == null)
	{	
		g_xml_error = fmt ("couldn't open %1 for reading: %2", fname, strerror (errno));
		return null;
	}

	fseek (fp, 0l, SEEK_END);
	fsize = ftell (fp);
	rewind (fp);
	buf = new char[fsize + 1];

	if ((long) fread (buf, 1, fsize, fp) < fsize)
	{	g_xml_error = fmt ("I/O error while opening %1", fname);
		fclose (fp);
		return null;
	}

	buf[fsize] = '\0';

	fclose (fp);
	XMLScanner scan (buf);
	XML_MUST_GET (HeaderStart)

	while (scan.next (XMLScanner::ESymbol))
	{	QString attrname = scan.token();
		XML_MUST_GET (Equals)
		XML_MUST_GET (String)
		header[attrname] = scan.token();
	}

	XML_MUST_GET (HeaderEnd)

	/*
	if (header.find ("version") == header.end())s
		XML_ERROR ("No version defined in header!");
	*/

	while (scan.next())
	{	switch (scan.token_type())
		{	case XMLScanner::ETagStart:
			{	XML_MUST_GET (Symbol)
				XMLNode* node = new XMLNode (scan.token(), topStackNode());

				if (g_stack.size() == 0)
				{	if (root != null)
					{	// XML forbids having multiple roots
						delete node;
						XML_ERROR ("Multiple root nodes")
					}

					root = node;
				}

				g_stack << node;

				while (scan.next (XMLScanner::ESymbol))
				{	QString attrname = scan.token();
					XML_MUST_GET (Equals)
					XML_MUST_GET (String)
					node->setAttribute (attrname, scan.token());
					assert (node->hasAttribute (attrname) == true);
				}

				if (scan.next (XMLScanner::ETagSelfCloser))
				{	XMLNode* popee;
					assert (pop (g_stack, popee) && popee == node);
				}
				else
					XML_MUST_GET (TagEnd)
			} break;

			case XMLScanner::ETagCloser:
			{	XML_MUST_GET (Symbol)
				XMLNode* popee;

				if (!pop (g_stack, popee) || popee->name() != scan.token())
					XML_ERROR ("Misplaced closing tag")

				XML_MUST_GET (TagEnd)
			} break;

			case XMLScanner::ECData:
			case XMLScanner::ESymbol:
			{	if (g_stack.size() == 0)
					XML_ERROR ("Misplaced CDATA/symbol")

					XMLNode* node = g_stack[g_stack.size() - 1];

				node->set_is_cdata (scan.token_type() == XMLScanner::ECData);
				node->set_contents (node->is_cdata() ? Decode (scan.token()) : scan.token());
			} break;

			case XMLScanner::EString:
			case XMLScanner::EHeaderStart:
			case XMLScanner::EHeaderEnd:
			case XMLScanner::EEquals:
			case XMLScanner::ETagSelfCloser:
			case XMLScanner::ETagEnd:
			{	XML_ERROR ("Unexpected token '%1'", scan.token());
			} break;
		}
	}

	XMLDocument* doc = new XMLDocument (root);
	doc->SetHeader (header);
	return doc;
}

// =============================================================================
// -----------------------------------------------------------------------------
bool XMLDocument::save (QString fname) const
{	FILE* fp;

	if ((fp = fopen (fname.toStdString().c_str(), "w")) == null)
		return false;

	flog (fp, "<?xml");

	for (auto it = Header().begin(); it != Header().end(); ++it)
		flog (fp, " %1=\"%2\"", it.key(), it.value());

	flog (fp, " ?>\n");
	g_save_stack = 0;
	writeNode (fp, Root());
	fclose (fp);
	return true;
}

// =============================================================================
// -----------------------------------------------------------------------------
void XMLDocument::writeNode (FILE* fp, const XMLNode* node) const
{	QString indent;

	for (int i = 0; i < g_save_stack; ++i)
		indent += "\t";

	flog (fp, "%1<%2", indent, node->name());

	for (auto it = node->attributes().begin(); it != node->attributes().end(); ++it)
		flog (fp, " %1=\"%2\"", Encode (it.key()), Encode (it.value()));

	if (node->empty() && g_save_stack > 0)
	{	flog (fp, " />\n");
		return;
	}

	flog (fp, ">");

	if (node->nodes().size() > 0)
	{	// Write nodes
		flog (fp, "\n");

		for (const XMLNode* subnode : node->nodes())
		{	g_save_stack++;
			writeNode (fp, subnode);
			g_save_stack--;
		}

		flog (fp, indent);
	}
	else
	{	// Write content
		if (node->is_cdata())
			flog (fp, "<![CDATA[%1]]>", node->contents());
		else
			flog (fp, Encode (node->contents()));
	}

	flog (fp, "</%1>\n", node->name());
}

// =============================================================================
// -----------------------------------------------------------------------------
QString XMLDocument::Encode (QString in)
{	QString out (in);
	
	for (auto i : g_encodingConversions)
		out.replace (i.decoded, i.encoded);
	
	return out;
}

// =============================================================================
// -----------------------------------------------------------------------------
QString XMLDocument::Decode (QString in)
{	QString out (in);
	
	for (auto i : g_encodingConversions)
		out.replace (i.encoded, i.decoded);
	
	return out;
}

// =============================================================================
// -----------------------------------------------------------------------------
XMLNode* XMLDocument::FindNodeByName (QString name) const
{	return Root()->findSubNode (name, true);
}

// =============================================================================
// -----------------------------------------------------------------------------
XMLNode* XMLDocument::NavigateTo (const QStringList& path, bool allowMake) const
{	XMLNode* node = Root();

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
QString XMLDocument::GetParseError()
{	return g_xml_error;
}

// =============================================================================
// -----------------------------------------------------------------------------
XMLNode::XMLNode (QString name, XMLNode* parent) :
	m_name (name),
	m_is_cdata (false),
	m_parent (parent)
{
	if (parent)
		parent->m_nodes << this;
}

// =============================================================================
// -----------------------------------------------------------------------------
XMLNode::~XMLNode()
{	for (XMLNode* node : m_nodes)
		delete node;

	if (m_parent)
		m_parent->dropNode (this);
}

// =============================================================================
// -----------------------------------------------------------------------------
QString XMLNode::attribute (QString name) const
{	if (hasAttribute (name))
		return attributes()[name];

	return QString();
}

// =============================================================================
// -----------------------------------------------------------------------------
void XMLNode::dropNode (XMLNode* node)
{	m_nodes.removeOne (node);
}

// =============================================================================
// -----------------------------------------------------------------------------
bool XMLNode::hasAttribute (QString name) const
{	return attributes().find (name) != attributes().end();
}

// =============================================================================
// -----------------------------------------------------------------------------
void XMLNode::setAttribute (QString name, QString data)
{	m_attributes[name] = data;
}

// =============================================================================
// -----------------------------------------------------------------------------
XMLNode* XMLNode::findSubNode (QString fname, bool recursive)
{	for (XMLNode* node : nodes())
	{	if (node->name() == fname)
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

	for (XMLNode* node : nodes())
		if (node->name() == name)
			matches << node;

	return matches;
}

// =============================================================================
// -----------------------------------------------------------------------------
bool XMLNode::empty() const
{	return contents().isEmpty() && nodes().isEmpty();
}

// =============================================================================
// -----------------------------------------------------------------------------
XMLNode* XMLNode::addSubNode (QString name, QString cont)
{	XMLNode* node = new XMLNode (name, this);

	if (cont.length() > 0)
		node->set_contents (cont);

	return node;
}

// =============================================================================
// -----------------------------------------------------------------------------
QList<XMLNode*> XMLNode::getNodesByAttribute (QString attrname, QString attrvalue)
{	QList<XMLNode*> matches;

	for (XMLNode* node : nodes())
		if (node->attribute (attrname) == attrvalue)
			matches << node;

	return matches;
}

// =============================================================================
// -----------------------------------------------------------------------------
XMLNode* XMLNode::getOneNodeByAttribute (QString attrname, QString attrvalue)
{	for (XMLNode* node : nodes())
		if (node->attribute (attrname) == attrvalue)
			return node;

	return null;
}
