#include "xml_scanner.h"
#include "xml_document.h"

static const char* g_XMLTokens[] =
{
	"<?xml",			// HeaderStart
	"?>",				// HeaderEnd
	"</",				// TagCloser
	"/>",				// TagSelfCloser
	"<",				// TagStart
	">",				// TagEnd
	// ----------------------------------
	"CData",			// ECData,
	"Equals sign",		// EEquals,
	"a symbol",			// ESymbol
	"a string",			// EString
};

// Amount of tokens which are processed from tokens above
static const int	gNumNamedTokens = 6;
static const char*	gCDataStart   = "<![CDATA[";
static const char*	gCDataEnd     = "]]>";
static const char*	gCommentStart = "<!--";
static const char*	gCommentEnd   = "-->";

// =============================================================================
//
XMLScanner::XMLScanner (const char* data) :
	m_data (data),
	m_position (&m_data[0]),
	m_isInsideTag (false),
	m_lineNumber (0) {}

// =============================================================================
//
bool XMLScanner::checkString (const char* c, bool peek)
{
	const bool r = strncmp (position(), c, strlen (c)) == 0;

	if (r && !peek)
		setPosition (position() + strlen (c));

	return r;
}

// =============================================================================
//
bool XMLScanner::scanNextToken()
{
	setToken ("");

	while (isspace (*position()))
	{
		if (*position() == '\n')
			setLineNumber (lineNumber() + 1);

		increasePosition();
	}

	if (*position() == '\0')
		return false;

	// Skip any comments
	while (checkString (gCommentStart))
		while (!checkString (gCommentEnd))
			increasePosition();

	// Check and parse CDATA
	if (checkString (gCDataStart))
	{
		while (!checkString (gCDataEnd))
		{
			setToken (token() + *position());
			increasePosition();
		}

		setTokenType (ECData);
		return true;
	}

	// Check "<", ">", "/>", ...
	for (int i = 0; i < gNumNamedTokens; ++i)
	{
		if (checkString (g_XMLTokens[i]))
		{
			setToken (g_XMLTokens[i]);
			setTokenType ((EToken) i);

			// We need to keep track of when we're inside node tags so we can
			// stop on '=' signs for attributes when inside tags where '=' has
			// special meaning but not outside tags where it's just a glyph.
			if (i == ETagStart || i == EHeaderStart)
				setInsideTag (true);
			elif (i == ETagEnd || ETagSelfCloser || i == EHeaderEnd)
				setInsideTag (false);

			return true;
		}
	}

	// Check and parse string
	if (*position() == '\"')
	{
		increasePosition();

		while (*position() != '\"')
		{
			if (!*position())
				return false;

			if (checkString ("\\\""))
			{
				setToken (token() + "\"");
				continue;
			}

			setToken (token() + *position());
			increasePosition();
		}

		setTokenType (EString);
		increasePosition(); // skip the final quote
		return true;
	}

	setTokenType (ESymbol);

	while (*position() != '\0')
	{
		if (isInsideTag() && isspace (*position()))
			break;

		// Stop at '=' if inside tag
		if (isInsideTag() && *position() == '=')
		{
			if (token().length() > 0)
				break;
			else
			{
				setTokenType (EEquals);
				setToken ("=");
				increasePosition();
				return true;
			}
		}

		bool stopHere = false;

		for (int i = 0; i < (signed) (sizeof g_XMLTokens / sizeof * g_XMLTokens); ++i)
		{
			if (checkString (g_XMLTokens[i], true))
			{
				stopHere = true;
				break;
			}
		}

		if (stopHere)
			break;

		setToken (token() + *position());
		increasePosition();
	}

	return true;
}

// =============================================================================
//
bool XMLScanner::scanNextToken (EToken tok)
{
	const char* oldPosition = position();

	if (!scanNextToken())
		return false;

	if (tokenType() != tok)
	{
		setPosition (oldPosition);
		return false;
	}

	return true;
}

// =============================================================================
//
void XMLScanner::mustScanNext (XMLScanner::EToken tok)
{
	if (!scanNextToken (tok))
		throw format ("Expected '%1', got '%2' instead", g_XMLTokens[tok], token());
}
