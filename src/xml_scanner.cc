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
static const char*	g_CDataEnd     = "]]>";
static const char*	gCommentStart = "<!--";
static const char*	gCommentEnd   = "-->";

// =============================================================================
//
XMLScanner::XMLScanner (const char* data) :
	data (data),
	position (&data[0]),
	isInsideTag (false),
	lineNumber (0) {}

// =============================================================================
//
bool XMLScanner::checkString (const char* c, bool peek)
{
	const bool r = strncmp (position, c, strlen (c)) == 0;

	if (r && peek == false)
		position += strlen (c);

	return r;
}

// =============================================================================
//
bool XMLScanner::scanNextToken()
{
	token = "";

	while (isspace (*position))
	{
		if (*position == '\n')
			lineNumber++;

		position++;
	}

	if (*position == '\0')
		return false;

	// Skip any comments
	while (checkString (gCommentStart))
		while (checkString (gCommentEnd) == false)
			position++;

	// Check and parse CDATA
	if (checkString (gCDataStart))
	{
		while (!checkString (g_CDataEnd))
		{
			token = token + *position;
			position++;
		}

		tokenType = ECData;
		return true;
	}

	// Check "<", ">", "/>", ...
	for (int i = 0; i < gNumNamedTokens; ++i)
	{
		if (checkString (g_XMLTokens[i]))
		{
			token = g_XMLTokens[i];
			tokenType = static_cast<EToken> (i);

			// We need to keep track of when we're inside node tags so we can
			// stop on '=' signs for attributes when inside tags where '=' has
			// special meaning but not outside tags where it's just a glyph.
			if (i == ETagStart || i == EHeaderStart)
				isInsideTag = true;
			elif (i == ETagEnd || ETagSelfCloser || i == EHeaderEnd)
				isInsideTag = false;

			return true;
		}
	}

	// Check and parse string
	if (*position == '\"')
	{
		position++;

		while (*position != '\"')
		{
			if (*position == '\0')
				throw std::runtime_error ("unterminated string");

			if (checkString ("\\\""))
			{
				token = token + "\"";
				continue;
			}

			token = token + *position;
			position++;
		}

		tokenType = EString;
		position++; // skip the final quote
		return true;
	}

	tokenType = ESymbol;

	while (*position != '\0')
	{
		if (isInsideTag && isspace (*position))
			break;

		// Stop at '=' if inside tag
		if (isInsideTag && *position == '=')
		{
			if (token.length() > 0)
				break;
			else
			{
				tokenType = EEquals;
				token = "=";
				position++;
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

		token = token + *position;
		position++;
	}

	return true;
}

// =============================================================================
//
bool XMLScanner::scanNextToken (EToken tok)
{
	const char* oldPosition = position;

	if (scanNextToken() == false)
		return false;

	if (tokenType != tok)
	{
		position = oldPosition;
		return false;
	}

	return true;
}

// =============================================================================
//
void XMLScanner::mustScanNext (XMLScanner::EToken tok)
{
	if (!scanNextToken (tok))
		throw format ("Expected '%1', got '%2' instead", g_XMLTokens[tok], token);
}
