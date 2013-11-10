#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
#include <QStringList>
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
	};

	struct ConfigData
	{	void* ptr;
		Type type;
		const char* name;
	};

	extern ConfigData g_config_data[COBALT_MAX_CONFIG];
	extern int g_cfg_data_cursor;

	// Type-definitions for the above enum list
	typedef int Int;
	typedef QString String;
	typedef float Float;
	typedef bool Bool;
	typedef QList<int> IntList;
	typedef QStringList StringList;
	typedef QMap<QString, QString> StringMap;

	// ------------------------------------------
	bool           load (QString fname);
	bool           save (QString fname);
	XMLDocument*   xml();

	class ConfigAdder
	{	public:
			// =============================================================================
			// We cannot just add config objects to a list or vector because that would rely
			// on the QList's c-tor being called before the configs' c-tors. With global
			// variables we cannot assume that!! Therefore we need to use a C-style array here.
			// -----------------------------------------------------------------------------
			template<class T> ConfigAdder (T* ptr, Type type, const char* name, const T& def)
			{	if (g_cfg_data_cursor == 0)
					memset (g_config_data, 0, sizeof g_config_data);

				assert (g_cfg_data_cursor < COBALT_MAX_CONFIG);
				ConfigData& i = g_config_data[g_cfg_data_cursor++];
				i.ptr = ptr;
				i.type = type;
				i.name = name;

				*ptr = def;
			}
	};
};

#endif // CONFIG_H
