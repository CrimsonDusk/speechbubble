#include <QApplication>
#include <typeinfo>
#include "main.h"
#include "mainwindow.h"
#include "config.h"
#include "xml_document.h"
#include "crashcatcher.h"
#include "context.h"

const char* configname = UNIXNAME ".xml";

// =============================================================================
// -----------------------------------------------------------------------------
int main (int argc, char* argv[])
{	QApplication app (argc, argv);
	init_crash_catcher();

	if (!Config::Load (configname))
	{	flog (stderr, "unable to load %1: %2\n", configname, XMLDocument::getParseError());
		return 1;
	}

	(new MainWindow)->show();
	Context::setCurrentContext (null);
	app.exec();
}

// =============================================================================
// -----------------------------------------------------------------------------
QString version_string()
{
#if VERSION_PATCH > 0
	return fmt ("%1.%2.%3", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
#else
	return fmt ("%1.%2", VERSION_MAJOR, VERSION_MINOR);
#endif
}