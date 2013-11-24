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

#ifdef IN_IDE_PARSER
# error IN_IDE_PARSER defined - this is for KDevelop workarounds
typedef void FILE;
#endif

#ifdef assert
#undef assert
#endif // assert

QString getVersionString();
void assertionFailure (const char* file, int line, const char* funcname, const char* expr); // crashcatcher.cc

#ifndef RELEASE
# define assert(A) (A) ? (void) 0 : assertionFailure (__FILE__, __LINE__, __PRETTY_FUNCTION__, #A)
#else
# define assert {}
#endif // RELEASE

#define COUNT_OF(A) (sizeof A / sizeof *A)

#endif // COIRC_MAIN_H