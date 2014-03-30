#ifndef MISC_H
#define MISC_H

#include "main.h"
#include "channel.h"

//! \file misc.h

//!
//! Gets a subset of \c list from \c a to \c b. If \c b is not passed, this joins
//! indices from \c a to end of list.
//!
QString subset (const QStringList& list, int a, int b = -1);

//!
//! Removes the last item of \c list into \c a
//!
template<class T, class R>
bool pop (R& list, T& a)
{
	if (list.isEmpty())
		return false;

	a = list.last();
	list.remove (list.size() - 1);
	return true;
}

//!
//! Checks whether \c a is within \c min and \c max inclusive
//!
template<class T>
static inline bool isWithinRange (T a, T min, T max)
{
	return (a >= min && a <= max);
}

//!
//! Toggles the value of \c a
//!
static inline void toggle (bool& a)
{
	a = !a;
}

#endif // MISC_H
