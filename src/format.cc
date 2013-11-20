#include "format.h"
#include "main.h"

// =============================================================================
// -----------------------------------------------------------------------------
QString formatString (const std::initializer_list<StringFormatArg>& args)
{	QString text = args.begin()->getText();

	for (auto it = args.begin() + 1; it != args.end(); ++it)
		text = text.arg (it->getText());

	return text;
}

// =============================================================================
// -----------------------------------------------------------------------------
void logTo (FILE* fp, QString a)
{	fprintf (fp, "%s", a.toUtf8().constData());
}