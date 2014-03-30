#include "main.h"
#include "config.h"
#include "xml_document.h"
#include "format.h"

Config::ConfigData*		gConfigData = null;
static XMLDocument*     gXMLDocument = null;

// =============================================================================
//
static void freeConfigData()
{
	Config::ConfigData* i = gConfigData;

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
	XMLNode* node = gXMLDocument->navigateTo (name.split ("_"), true);

	if (!node)
		node = new XMLNode (name, gXMLDocument->root());

	switch (type)
	{
		case Config::EInt:
		{
			node->setContents (QString::number (*reinterpret_cast<int*> (ptr)));
			break;
		}

		case Config::EString:
		{
			node->setContents (*reinterpret_cast<QString*> (ptr));
			break;
		}

		case Config::EFloat:
		{
			node->setContents (QString::number (*reinterpret_cast<float*> (ptr)));
			break;
		}

		case Config::EBool:
		{
			node->setContents (*reinterpret_cast<bool*> (ptr) ? "true" : "false");
			break;
		}

		case Config::EStringList:
		{
			for (XMLNode* subnode : node->subNodes())
				delete subnode;

			for (QString item : *reinterpret_cast<QStringList*> (ptr))
			{
				XMLNode* subnode = new XMLNode ("item", node);
				subnode->setContents (item);
			}
			break;
		}

		case Config::EIntList:
		{
			for (int item : *reinterpret_cast<Config::IntList*> (ptr))
			{
				XMLNode* subnode = new XMLNode ("item", node);
				subnode->setContents (QString::number (item));
			}
			break;
		}

		case Config::EStringMap:
		{
			const Config::StringMap& map = *reinterpret_cast<Config::StringMap*> (ptr);

			for (auto it = map.begin(); it != map.end(); ++it)
			{
				XMLNode* subnode = new XMLNode (it.key(), node);
				subnode->setContents (it.value());
			}
			break;
		}

		case Config::EFont:
		{
			node->setContents (reinterpret_cast<QFont*> (ptr)->toString());
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
		*reinterpret_cast<int*> (ptr) = node->contents().toLong();
		break;

	case Config::EString:
		*reinterpret_cast<QString*> (ptr) = node->contents();
		break;

	case Config::EFloat:
		*reinterpret_cast<float*> (ptr) = node->contents().toFloat();
		break;

	case Config::EBool:
	{
		QString val = node->contents();
		bool& var = * reinterpret_cast<bool*> (ptr);
		var = (val == "true" || val == "1" || val == "on" || val == "yes");
	} break;

	case Config::EStringList:
	{
		QStringList& var = * reinterpret_cast<QStringList*> (ptr);

		for (const XMLNode * subnode : node->subNodes())
			var << subnode->contents();
	} break;

	case Config::EIntList:
	{
		QList<int>& var = * (reinterpret_cast<QList<int>*> (ptr));

		for (const XMLNode * subnode : node->subNodes())
			var << subnode->contents().toLong();
	} break;

	case Config::EStringMap:
	{
		Config::StringMap& var = * (reinterpret_cast<Config::StringMap*> (ptr));

		for (const XMLNode * subnode : node->subNodes())
			var[subnode->name()] = subnode->contents();
	} break;

	case Config::EFont:
	{
		reinterpret_cast<QFont*> (ptr)->fromString (node->contents());
	} break;
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
	i->next = gConfigData;
	gConfigData = i;

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

	if (!doc)
		return false;

	for (ConfigData* i = gConfigData; i; i = i->next)
	{
		XMLNode* node = doc->navigateTo (QString (i->name).split ("_"));

		if (node)
			loadFromXML (i->ptr, i->type, node);
	}

	gXMLDocument = doc;
	return true;
}

// =============================================================================
//
// Save the configuration to @fname
//
bool Config::saveToFile (const QString& fname)
{
	if (gXMLDocument == null)
		gXMLDocument = XMLDocument::newDocument ("config");

	print ("Saving configuration to %1...\n", fname);

	for (ConfigData* i = gConfigData; i; i = i->next)
		saveElementToXML (i->name, i->ptr, i->type);

	return gXMLDocument->saveToFile (fname);
}

// =============================================================================
//
// Get the XML document of the configuration file
//
XMLDocument* Config::getXMLDocument()
{
	return gXMLDocument;
}
