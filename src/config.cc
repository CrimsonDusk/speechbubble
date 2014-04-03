#include "main.h"
#include "config.h"
#include "xml_document.h"
#include "format.h"

Config::ConfigData*		g_configData = null;
static XMLDocument*     g_XMLDocument = null;

// =============================================================================
//
static void freeConfigData()
{
	Config::ConfigData* i = g_configData;

	while (i != null)
	{
		Config::ConfigData* old = i;
		i = i->next;
		delete old;
	}
}

// =============================================================================
//
// Save the configuration element @ptr with name @name and data type @type
// to the XML document.
//
static void saveElementToXML (QString name, void* ptr, Config::EDataType type)
{
	XMLNode* node = g_XMLDocument->navigateTo (name.split ("_"), true);

	if (!node)
		node = new XMLNode (name, g_XMLDocument->root);

	switch (type)
	{
		case Config::EInt:
		{
			node->contents = QString::number (*reinterpret_cast<int*> (ptr));
			break;
		}

		case Config::EString:
		{
			node->contents = *reinterpret_cast<QString*> (ptr);
			break;
		}

		case Config::EFloat:
		{
			node->contents = QString::number (*reinterpret_cast<float*> (ptr));
			break;
		}

		case Config::EBool:
		{
			node->contents = *reinterpret_cast<bool*> (ptr) ? "true" : "false";
			break;
		}

		case Config::EStringList:
		{
			for (XMLNode* subnode : node->subNodes)
				delete subnode;

			for (QString item : *reinterpret_cast<QStringList*> (ptr))
			{
				XMLNode* subnode = new XMLNode ("item", node);
				subnode->contents = item;
			}
			break;
		}

		case Config::EIntList:
		{
			for (int item : *reinterpret_cast<Config::IntList*> (ptr))
			{
				XMLNode* subnode = new XMLNode ("item", node);
				subnode->contents = QString::number (item);
			}
			break;
		}

		case Config::EStringMap:
		{
			const Config::StringMap& map = *reinterpret_cast<Config::StringMap*> (ptr);

			for (auto it = map.begin(); it != map.end(); ++it)
			{
				XMLNode* subnode = new XMLNode (it.key(), node);
				subnode->contents = it.value();
			}
			break;
		}

		case Config::EFont:
		{
			node->contents = reinterpret_cast<QFont*> (ptr)->toString();
			break;
		}
	}
}

// =============================================================================
//
// Load the configuration element @ptr with type @type from XML node @node
//
static void loadFromXML (void* ptr, Config::EDataType type, XMLNode* node)
{
	switch (type)
	{
		case Config::EInt:
			*reinterpret_cast<int*> (ptr) = node->contents.toLong();
			break;

		case Config::EString:
			*reinterpret_cast<QString*> (ptr) = node->contents;
			break;

		case Config::EFloat:
			*reinterpret_cast<float*> (ptr) = node->contents.toFloat();
			break;

		case Config::EBool:
		{
			QString val = node->contents;
			*reinterpret_cast<bool*> (ptr) = (val == "true" || val == "1" || val == "on" || val == "yes");
			break;
		}

		case Config::EStringList:
		{
			for (const XMLNode* subnode : node->subNodes)
				reinterpret_cast<QStringList*> (ptr)->append (subnode->contents);
			break;
		}

		case Config::EIntList:
		{
			for (const XMLNode* subnode : node->subNodes)
				reinterpret_cast<QList<int>*> (ptr)->append (subnode->contents.toLong());
			break;
		}

		case Config::EStringMap:
		{
			for (const XMLNode * subnode : node->subNodes)
				(*reinterpret_cast<Config::StringMap*> (ptr))[subnode->name] = subnode->contents;
			break;
		}

		case Config::EFont:
		{
			reinterpret_cast<QFont*> (ptr)->fromString (node->contents);
			break;
		}
	}
}

// =============================================================================
//
Config::ConfigAdder::ConfigAdder (void* ptr, Config::EDataType type, const char* name)
{
	ConfigData* i = new ConfigData;
	i->ptr = ptr;
	i->type = type;
	i->name = name;
	i->next = g_configData;
	g_configData = i;

	atexit (&freeConfigData);
}

// =============================================================================
//
// Load the configuration from @fname
//
bool Config::loadFromFile (const QString& fname)
{
	print ("config::load: Loading configuration file from %1\n", fname);
	XMLDocument* doc = XMLDocument::loadFromFile (fname);

	if (doc == null)
		return false;

	for (ConfigData* i = g_configData; i; i = i->next)
	{
		XMLNode* node = doc->navigateTo (QString (i->name).split ("_"));

		if (node)
			loadFromXML (i->ptr, i->type, node);
	}

	g_XMLDocument = doc;
	return true;
}

// =============================================================================
//
// Save the configuration to @fname
//
bool Config::saveToFile (const QString& fname)
{
	if (g_XMLDocument == null)
		g_XMLDocument = XMLDocument::newDocument ("config");

	print ("Saving configuration to %1...\n", fname);

	for (ConfigData* i = g_configData; i != null; i = i->next)
		saveElementToXML (i->name, i->ptr, i->type);

	return g_XMLDocument->saveToFile (fname);
}

// =============================================================================
//
// Get the XML document of the configuration file
//
XMLDocument* Config::getXMLDocument()
{
	return g_XMLDocument;
}
