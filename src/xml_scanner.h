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

	PROPERTY (private, const char*,	data,			setData,		STOCK_WRITE)
	PROPERTY (private, const char*,	position,		setPosition,	STOCK_WRITE)
	PROPERTY (private, QString,		token,			setToken,		STOCK_WRITE)
	PROPERTY (private, EToken,		tokenType,		setTokenType,	STOCK_WRITE)
	PROPERTY (private, bool,        isInsideTag,	setInsideTag,	STOCK_WRITE)
	PROPERTY (private, int,			lineNumber,		setLineNumber,	STOCK_WRITE)

public:
	XMLScanner (const char* data);

	bool scanNextToken();
	bool scanNextToken (EToken tok);
	void mustScanNext (EToken tok);

private:
	bool checkString (const char* c, bool peek = false);

	inline void increasePosition()
	{
		m_position++;
	}
};

#endif // LIBCOBALTCORE_XML_SCANNER_H
