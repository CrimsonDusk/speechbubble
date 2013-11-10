#ifndef COIRC_MAIN_H
#define COIRC_MAIN_H

#ifdef IN_IDE_PARSER
#define __attribute__(A)
#endif

#include <cstdio>
#include <cstdlib>
#include <QString>
#include <QList>
#include <QStringList>
#include <QMap>
#include "macros.h"
#include "format.h"

// Even though we have our own assert in this file, we still need to include <cassert>.
// This is to ensure that no Qt headers, some of which do include <cassert>, wind up
// overriding our assert with the standard one.
#include <cassert>

static const std::nullptr_t null = nullptr;
extern const char* configname;
typedef QString str;

template<class T> using list = QList<T>;
template<class T, class R> using map = QMap<T, R>;
template<class T> using vector = QVector<T>;

#ifdef IN_IDE_PARSER
# error IN_IDE_PARSER defined - this is for KDevelop workarounds
typedef void FILE;
#endif


// Removes the last item of @list into @a
template<class T, class R> bool pop (R& list, T& a)
{	if (list.isEmpty())
		return false;

	a = list.last();
	list.remove (list.size() - 1);
	return true;
}

// Checks whether @a is within [@min, @max] inclusive
template<class T> static inline bool within_range (T a, T min, T max)
{	return (a >= min && a <= max);
}

// Toggles the value of @a
static inline bool& toggle (bool& a)
{	a = !a;
	return a;
}

#ifdef assert
#undef assert
#endif // assert

QString version_string();
void assertion_failure (const char* file, int line, const char* funcname, const char* expr); // crashcatcher.cc

#ifndef RELEASE
# define assert(A) (A) ? (void) 0 : assertion_failure (__FILE__, __LINE__, __PRETTY_FUNCTION__, #A)
#else
# define assert {}
#endif // RELEASE

#endif // COIRC_MAIN_H