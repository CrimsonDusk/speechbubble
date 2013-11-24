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

	PROPERTY (private, const char*,  Data);
	PROPERTY (private, const char*,  Position);
	PROPERTY (private, QString,      Token);
	PROPERTY (private, EToken,       TokenType);
	PROPERTY (private, bool,         IsInsideTag);
	PROPERTY (private, int,          Line)

	public:
		XMLScanner (const char* data);

		bool scanNextToken();
		bool scanNextToken (EToken tok);
		void mustScanNext (EToken tok);

	private:
		bool checkString (const char* c, bool peek = false);
};

#endif // LIBCOBALTCORE_XML_SCANNER_H
