#include "format.h"
#include "main.h"

// =============================================================================
// -----------------------------------------------------------------------------
QString formatString (const std::initializer_list<StringFormatArg>& args)
{	QString text = args.begin()->text();

	for (auto it = args.begin() + 1; it != args.end(); ++it)
		text = text.arg (it->text());

	return text;
}

// =============================================================================
// -----------------------------------------------------------------------------
void log_to (FILE* fp, QString a)
{	fprintf (fp, "%s", a.toStdString().c_str());
}