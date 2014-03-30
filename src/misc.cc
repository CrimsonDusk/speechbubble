#include "misc.h"

QString subset (const QStringList& list, int a, int b)
{
	if (b == -1)
		b = list.size() - 1;

	assert (list.size() > a && list.size() > b && b >= a);
	QString out;

	for (auto it = list.begin() + a; it <= list.begin() + b; ++it)
	{
		if (!out.isEmpty())
			out += " ";

		out += *it;
	}

	return out;
}
