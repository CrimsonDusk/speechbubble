#ifndef LIBCOBALTCORE_XML_SCANNER_H
#define LIBCOBALTCORE_XML_SCANNER_H

#include "main.h"

class XMLScanner
{	public:
		enum EToken
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

	NEW_PROPERTY (private, const char*,  Data);
	NEW_PROPERTY (private, const char*,  Position);
	NEW_PROPERTY (private, QString,      Token);
	NEW_PROPERTY (private, EToken,       TokenType);
	NEW_PROPERTY (private, bool,         IsInsideTag);
	NEW_PROPERTY (private, int,          Line)

	public:
		XMLScanner (const char* data);

		bool scanNextToken();
		bool scanNextToken (EToken tok);

	private:
		bool checkString (const char* c, bool peek = false);
};

#endif // LIBCOBALTCORE_XML_SCANNER_H
