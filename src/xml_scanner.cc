#include "xml_scanner.h"
#include "xml_document.h"

static const char* g_XMLTokens[] =
{	"<?xml",			// HeaderStart
	"?>",				// HeaderEnd
	"</",				// TagCloser
	"/>",				// TagSelfCloser
	"<",				// TagStart
	">",				// TagEnd
	// -------------------
	"CData",			// ECData,
	"Equals sign",	// EEquals,
	"a symbol",		// ESymbol
	"a string",		// EString
};

// amount of tokens which are processed from tokens above
static const int g_numXMLProcessedTokens = 6;

static const char* g_CDataStart   = "<![CDATA[";
static const char* g_CDataEnd     = "]]>";
static const char* g_CommentStart = "<!--";
static const char* g_CommentEnd   = "-->";

// =============================================================================
// -----------------------------------------------------------------------------
XMLScanner::XMLScanner (const char* data) :
	m_Data (data),
	m_Position (&m_Data[0]),
	m_IsInsideTag (false),
	m_Line (0) {}

// =============================================================================
// -----------------------------------------------------------------------------
bool XMLScanner::checkString (const char* c, bool peek)
{	const bool r = strncmp (getPosition(), c, strlen (c)) == 0;

	if (r && !peek)
		setPosition (getPosition() + strlen (c));

	return r;
}

// =============================================================================
// -----------------------------------------------------------------------------
bool XMLScanner::scanNextToken()
{	setToken ("");

	while (isspace (*getPosition()))
	{	if (*getPosition() == '\n')
		setLine (getLine() + 1);

		m_Position++;
	}

	if (*getPosition() == '\0')
		return false;

	// Skip any comments
	while (checkString (g_CommentStart))
		while (!checkString (g_CommentEnd))
			m_Position++;

	// Check and parse CDATA
	if (checkString (g_CDataStart))
	{	while (!checkString (g_CDataEnd))
		{	setToken (getToken() + *getPosition());
			m_Position++;
		}

		setTokenType (ECData);
		return true;
	}

	// Check "<", ">", "/>", ...
	for (int i = 0; i < g_numXMLProcessedTokens; ++i)
	{	if (checkString (g_XMLTokens[i]))
		{	setToken (g_XMLTokens[i]);
			setTokenType ((EToken) i);

			// We need to keep track of when we're inside node tags so we can
			// stop on '=' signs for attributes when inside tags where '=' has
			// special meaning but not outside tags where it's just a glyph.
			if (i == ETagStart || i == EHeaderStart)
				setIsInsideTag (true);
			elif (i == ETagEnd || ETagSelfCloser || i == EHeaderEnd)
				setIsInsideTag (false);

			return true;
		}
	}

	// Check and parse string
	if (*getPosition() == '\"')
	{	m_Position++;

		while (*getPosition() != '\"')
		{	if (!*getPosition())
				return false;

			if (checkString ("\\\""))
			{	setToken (getToken() + "\"");
				continue;
			}

			setToken (getToken() + *getPosition());
			m_Position++;
		}

		setTokenType (EString);
		m_Position++; // skip the final quote
		return true;
	}

	setTokenType (ESymbol);

	while (*getPosition() != '\0')
	{	if (getIsInsideTag() && isspace (*getPosition()))
			break;

		// Stop at '=' if inside tag
		if (getIsInsideTag() && *getPosition() == '=')
		{	if (getToken().length() > 0)
				break;
			else
			{	setTokenType (EEquals);
				setToken ("=");
				m_Position++;
				return true;
			}
		}

		bool stopHere = false;

		for (int i = 0; i < (signed) (sizeof g_XMLTokens / sizeof * g_XMLTokens); ++i)
		{	if (checkString (g_XMLTokens[i], true))
			{	stopHere = true;
				break;
			}
		}

		if (stopHere)
			break;

		setToken (getToken() + *getPosition());
		m_Position++;
	}

	return true;
}

// =============================================================================
// -----------------------------------------------------------------------------
bool XMLScanner::scanNextToken (EToken tok)
{	const char* oldPosition = getPosition();

	if (!scanNextToken())
		return false;

	if (getTokenType() != tok)
	{	setPosition (oldPosition);
		return false;
	}

	return true;
}

// =============================================================================
// -----------------------------------------------------------------------------
void XMLScanner::mustScanNext (XMLScanner::EToken tok)
{	if (!scanNextToken (tok))
		throw XMLError (fmt ("Expected '%1', got '%2' instead", g_XMLTokens[tok], getToken()));
}