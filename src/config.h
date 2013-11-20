#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
#include <QStringList>
#include <QFont>
#include "main.h"

#define CONFIG(T, NAME, DEFAULT) namespace cfg { Config::T NAME = DEFAULT; } \
	Config::ConfigAdder zz_ConfigAdder_##NAME (&cfg::NAME, Config::E##T, #NAME);

#define EXTERN_CONFIG(T, NAME) namespace cfg { extern Config::T NAME; }
#define MAX_CONFIG 512

class XMLDocument;

// =========================================================
namespace Config
{	enum EDataType
	{	EInt,
		EString,
		EFloat,
		EBool,
		EIntList,
		EStringList,
		EStringMap,
		EFont,
	};

	struct ConfigData
	{	void* ptr;
		EDataType type;
		const char* name;
		ConfigData* next;
	};

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
	bool           loadFromFile (QString fname);
	bool           saveToFile (QString fname);
	void				freeConfigData();
	XMLDocument*   xml();

	class ConfigAdder
	{	public:
			ConfigAdder (void* ptr, EDataType type, const char* name);
	};
};

#endif // CONFIG_H
