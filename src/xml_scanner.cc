#include "xml_scanner.h"
#include "xml.h"

static const char* g_xml_tokens[] =
{	"<?xml",     // HeaderStart
	"?>",        // HeaderEnd
	"</",        // TagCloser
	"/>",        // TagSelfCloser
	"<",         // TagStart
	">",         // TagEnd
};

static const char* g_cdata_start   = "<![CDATA[";
static const char* g_cdata_end     = "]]>";
static const char* g_comment_start = "<!--";
static const char* g_comment_end   = "-->";

// =============================================================================
// -----------------------------------------------------------------------------
XMLScanner::XMLScanner (const char* data) :
	m_data (data),
	m_ptr (&m_data[0]),
	m_inside_tag (false),
	m_line (0) {}

// =============================================================================
// -----------------------------------------------------------------------------
bool XMLScanner::checkString (const char* c, bool peek)
{	const bool r = strncmp (ptr(), c, strlen (c)) == 0;

	if (r && !peek)
		set_ptr (ptr() + strlen (c));

	return r;
}

// =============================================================================
// -----------------------------------------------------------------------------
bool XMLScanner::next()
{	set_token (str());

	while (isspace (*ptr()))
	{	if (*ptr() == '\n')
			set_line (line() + 1);

		incr_ptr();
	}

	if (*ptr() == '\0')
		return false;

	// Skip any comments
	while (checkString (g_comment_start))
		while (!checkString (g_comment_end))
			incr_ptr();

	// Check and parse CDATA
	if (checkString (g_cdata_start))
	{	while (!checkString (g_cdata_end))
		{	set_token (token() + *ptr());
			incr_ptr();
		}

		set_token_type (ECData);
		return true;
	}

	// Check "<", ">", "/>", ...
	for (int i = 0; i < (signed) (sizeof g_xml_tokens / sizeof * g_xml_tokens); ++i)
	{	if (checkString (g_xml_tokens[i]))
		{	set_token (g_xml_tokens[i]);
			set_token_type ((Token) i);

			// We need to keep track of when we're inside node tags so we can
			// stop on '=' signs for attributes when inside tags where '=' has
			// special meaning but not outside tags where it's just a glyph.
			if (i == ETagStart || i == EHeaderStart)
				set_inside_tag (true);
			elif (i == ETagEnd || ETagSelfCloser || i == EHeaderEnd)
				set_inside_tag (false);

			return true;
		}
	}

	// Check and parse string
	if (*ptr() == '\"')
	{	incr_ptr();

		while (*ptr() != '\"')
		{	if (!*ptr())
				return false;

			if (checkString ("\\\""))
			{	set_token (token() + "\"");
				continue;
			}

			set_token (token() + *ptr());
			incr_ptr();
		}

		set_token_type (EString);
		incr_ptr(); // skip the final quote
		return true;
	}

	set_token_type (ESymbol);

	while (*ptr() != '\0')
	{	if (inside_tag() && isspace (*ptr()))
			break;

		// Stop at '=' if inside tag
		if (inside_tag() && *ptr() == '=')
		{	if (token().length() > 0)
				break;
			else
			{	set_token_type (EEquals);
				set_token ("=");
				incr_ptr();
				return true;
			}
		}

		bool stopHere = false;

		for (int i = 0; i < (signed) (sizeof g_xml_tokens / sizeof * g_xml_tokens); ++i)
		{	if (checkString (g_xml_tokens[i], true))
			{	stopHere = true;
				break;
			}
		}

		if (stopHere)
			break;

		set_token (token() + *ptr());
		incr_ptr();
	}

	return true;
}

// =============================================================================
// -----------------------------------------------------------------------------
bool XMLScanner::next (XMLScanner::Token tok)
{	const char* old_pos = ptr();

	if (!next())
		return false;

	if (token_type() != tok)
	{	set_ptr (old_pos);
		return false;
	}

	return true;
}