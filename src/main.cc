#include <QApplication>
#include "main.h"
#include "mainwindow.h"
#include "config.h"
#include "xml_document.h"
#include "crashcatcher.h"
#include "context.h"

const char* configname = UNIXNAME ".xml";

// =============================================================================
//
int main (int argc, char* argv[])
{
	QApplication app (argc, argv);
	initCrashCatcher();

	if (Config::loadFromFile (configname) == false)
		Config::saveToFile (configname);

	(new MainWindow)->show();
	Context::setCurrentContext (null);
	app.exec();
}

// =============================================================================
//
QString getVersionString()
{
#if VERSION_PATCH > 0
	return format ("%1.%2.%3", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
#else
	return format ("%1.%2", VERSION_MAJOR, VERSION_MINOR);
#endif
}
