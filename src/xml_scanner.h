#ifndef LIBCOBALTCORE_XML_SCANNER_H
#define LIBCOBALTCORE_XML_SCANNER_H

#include "main.h"

class XMLScanner
{
public:
	enum EToken
	{
		EHeaderStart,
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

	PROPERTY (const char* data)
	PROPERTY (const char* position)
	PROPERTY (QString token)
	PROPERTY (EToken tokenType)
	PROPERTY (bool isInsideTag)
	PROPERTY (int lineNumber)
	CLASSDATA (XMLScanner)

public:
	XMLScanner (const char* data);

	bool scanNextToken();
	bool scanNextToken (EToken tok);
	void mustScanNext (EToken tok);

private:
	bool checkString (const char* c, bool peek = false);
};

#endif // LIBCOBALTCORE_XML_SCANNER_H
