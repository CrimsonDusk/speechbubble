#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
#include <QStringList>
#include <QFont>
#include "main.h"

#define CONFIG(T, NAME, DEFAULT) namespace cfg { Config::T NAME = DEFAULT; } \
	Config::ConfigAdder zz_ConfigAdder_##NAME (&cfg::NAME, Config::T##Type, \
		#NAME, Config::T (DEFAULT));

#define EXTERN_CONFIG(T, NAME) namespace cfg { extern Config::T NAME; }
#define COBALT_MAX_CONFIG 512

class XMLDocument;

// =========================================================
namespace Config
{	enum Type
	{	IntType,
		StringType,
		FloatType,
		BoolType,
		IntListType,
		StringListType,
		StringMapType,
		FontType,
	};

	struct ConfigData
	{	void* ptr;
		Type type;
		const char* name;
	};

	extern ConfigData g_ConfigData[COBALT_MAX_CONFIG];
	extern int g_ConfigDataCursor;

	// Type-definitions for the above enum list
	typedef int Int;
	typedef QString String;
	typedef float Float;
	typedef bool Bool;
	typedef QList<int> IntList;
	typedef QStringList StringList;
	typedef QMap<QString, QString> StringMap;
	typedef QFont Font;

	// ------------------------------------------
	bool           Load (QString fname);
	bool           SaveTo (QString fname);
	XMLDocument*   xml();

	class ConfigAdder
	{	public:
			// =============================================================================
			// We cannot just add config objects to a list or vector because that would rely
			// on the QList's c-tor being called before the configs' c-tors. With global
			// variables we cannot assume that!! Therefore we need to use a C-style array here.
			// -----------------------------------------------------------------------------
			template<class T> ConfigAdder (T* ptr, Type type, const char* name, const T& def)
			{	if (g_ConfigDataCursor == 0)
				memset (g_ConfigData, 0, sizeof g_ConfigData);

				assert (g_ConfigDataCursor < COBALT_MAX_CONFIG);
				ConfigData& i = g_ConfigData[g_ConfigDataCursor++];
				i.ptr = ptr;
				i.type = type;
				i.name = name;

				*ptr = def;
			}
	};
};

#endif // CONFIG_H
