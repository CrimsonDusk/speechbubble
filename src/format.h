#ifndef COBALTIRC_FORMAT_H
#define COBALTIRC_FORMAT_H

#include <QVector>
#include <QString>
#include <initializer_list>
#include "macros.h"

class StringFormatArg
{	PROPERTY (private, QString, Text)

	public:
		StringFormatArg (QString a) : m_Text (a) {}
		StringFormatArg (const char* a) : m_Text (a) {}
		StringFormatArg (int a) : m_Text (QString::number (a)) {}
		StringFormatArg (long a) : m_Text (QString::number (a)) {}
		StringFormatArg (uint a) : m_Text (QString::number (a)) {}
		StringFormatArg (ulong a) : m_Text (QString::number (a)) {}
		StringFormatArg (float a) : m_Text (QString::number (a)) {}
		StringFormatArg (double a) : m_Text (QString::number (a)) {}
		StringFormatArg (void* a) { m_Text.sprintf ("%p", a); }

		template<class T> StringFormatArg (QList<T> a)
		{	calibrateFromList (a);
		}

		template<class T> StringFormatArg (QVector<T> a)
		{	calibrateFromList (a);
		}

	private:
		template<class T> void calibrateFromList (T a)
		{	if (a.isEmpty())
			{	setText ("{}");
				return;
			}

			QString text = "{";

			const std::type_info& type = typeid (a[0]);
			const bool isStringType = type == typeid (QString) || type == typeid (const char*);

			for (const auto& item : a)
			{	if (&item != &a[0])
					text += ", ";

				if (isStringType)
					text += "\"" + StringFormatArg (item).getText() + "\"";
				else
					text += StringFormatArg (item).getText();
			}

			text += "}";
			setText (text);
		}
};

#ifndef IN_IDE_PARSER
# define fmt(...) formatString ({__VA_ARGS__})
# define log(...) logTo (stdout, formatString ({__VA_ARGS__}))
# define flog(F, ...) logTo (F, formatString ({__VA_ARGS__}))
#else
void log (void, ...);
void flog (FILE* fp, void, ...);
QString fmt (void, ...);
#endif

void logTo (FILE* fp, QString a);
QString formatString (const std::initializer_list<StringFormatArg>& args);

#endif // COBALTIRC_FORMAT_H