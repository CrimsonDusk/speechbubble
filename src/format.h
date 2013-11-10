#ifndef COBALTIRC_FORMAT_H
#define COBALTIRC_FORMAT_H

#include <QVector>
#include <QString>
#include <initializer_list>
#include "macros.h"

class StringFormatArg
{	PROPERTY (private, QString, text)

	public:
		StringFormatArg (QString a) : m_text (a) {}
		StringFormatArg (const char* a) : m_text (a) {}
		StringFormatArg (int a) : m_text (QString::number (a)) {}
		StringFormatArg (long a) : m_text (QString::number (a)) {}
		StringFormatArg (uint a) : m_text (QString::number (a)) {}
		StringFormatArg (ulong a) : m_text (QString::number (a)) {}
		StringFormatArg (float a) : m_text (QString::number (a)) {}
		StringFormatArg (double a) : m_text (QString::number (a)) {}
		StringFormatArg (void* a) { m_text.sprintf ("%p", a); }

		template<class T> StringFormatArg (QList<T> a)
		{	fromListType (a);
		}

		template<class T> StringFormatArg (QVector<T> a)
		{	fromListType (a);
		}

	private:
		template<class T> void fromListType (T a)
		{	if (a.isEmpty())
			{	m_text = "{}";
				return;
			}

			m_text = "{";

			const std::type_info& type = typeid (a[0]);
			const bool isStringType = type == typeid (QString) || type == typeid (const char*);

			for (const auto& item : a)
			{	if (&item != &a[0])
					m_text += ", ";

				if (isStringType)
					m_text += "\"" + StringFormatArg (item).text() + "\"";
				else
					m_text += StringFormatArg (item).text();
			}

			m_text += "}";
		}
};

#ifndef IN_IDE_PARSER
# define fmt(...) formatString ({__VA_ARGS__})
# define log(...) log_to (stdout, formatString ({__VA_ARGS__}))
# define flog(F, ...) log_to (F, formatString ({__VA_ARGS__}))
#else
void log (void, ...);
void flog (FILE* fp, void, ...);
QString fmt (void, ...);
#endif

void log_to (FILE* fp, QString a);
QString formatString (const std::initializer_list<StringFormatArg>& args);

#endif // COBALTIRC_FORMAT_H