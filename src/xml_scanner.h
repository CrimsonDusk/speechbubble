#ifndef LIBCOBALTCORE_XML_SCANNER_H
#define LIBCOBALTCORE_XML_SCANNER_H

#include "main.h"

class XMLScanner
{	public:
		enum Token
		{	EHeaderStart,
			EHeaderEnd,
			ETagCloser,
			ETagSelfCloser,
			ETagStart,
			ETagEnd,
			ECData,
			EEquals,
			ESymbol,
			EString
		};

	PROPERTY (private, const char*,  data);
	PROPERTY (private, const char*,  ptr);
	PROPERTY (private, QString,      token);
	PROPERTY (private, Token,        token_type);
	PROPERTY (private, bool,         inside_tag);
	PROPERTY (private, int,          line)

	public:
		XMLScanner (const char* data);

		bool next();
		bool next (Token tok);

	private:
		bool checkString (const char* c, bool peek = false);

		inline void incr_ptr()
		{	m_ptr++;
		}
};

#endif // LIBCOBALTCORE_XML_SCANNER_H
