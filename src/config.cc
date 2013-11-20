#include "main.h"
#include "config.h"
#include "xml_document.h"
#include "format.h"

Config::ConfigData* g_ConfigData = null;

// =============================================================================
// -----------------------------------------------------------------------------
Config::ConfigAdder::ConfigAdder(void* ptr, Config::EDataType type, const char* name)
{	ConfigData* i = new ConfigData;
	i->ptr = ptr;
	i->type = type;
	i->name = name;
	i->next = g_ConfigData;
	g_ConfigData = i;

	atexit (&Config::freeConfigData);
}

// =============================================================================
// -----------------------------------------------------------------------------
void Config::freeConfigData()
{	ConfigData* i = g_ConfigData;

	while (i != null)
	{	ConfigData* old = i;
		i = i->next;
		delete old;
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
namespace Config
{	int                     g_ConfigDataCursor = 0;
	static XMLDocument*     g_XMLDocument = null;

	// =============================================================================
	// Save the configuration element @ptr with name @name and data type @type
	// to the XML document.
	// -----------------------------------------------------------------------------
	static void saveToXML (QString name, void* ptr, EDataType type)
	{	XMLNode* node = g_XMLDocument->navigateTo (name.split ("_"), true);

		if (!node)
			node = new XMLNode (name, g_XMLDocument->getRoot());

		switch (type)
		{	case EInt:
			{	node->setContents (QString::number (*(reinterpret_cast<int*> (ptr))));
			} break;

			case EString:
			{	node->setContents (*(reinterpret_cast<QString*> (ptr)));
			} break;

			case EFloat:
			{	node->setContents (QString::number (*(reinterpret_cast<float*> (ptr))));
			} break;

			case EBool:
			{	node->setContents (*(reinterpret_cast<bool*> (ptr)) ? "true" : "false");
			} break;

			case EStringList:
			{	for (XMLNode* subnode : node->getSubnodes())
					delete subnode;

				for (QString item : *(reinterpret_cast<StringList*> (ptr)))
				{	XMLNode* subnode = new XMLNode ("item", node);
					subnode->setContents (item);
				}
			} break;

			case EIntList:
			{	for (int item : *(reinterpret_cast<IntList*> (ptr)))
				{	XMLNode* subnode = new XMLNode ("item", node);
					subnode->setContents (QString::number (item));
				}
			} break;

			case EStringMap:
			{	const StringMap& map = *(reinterpret_cast<StringMap*> (ptr));

				for (auto it = map.begin(); it != map.end(); ++it)
				{	XMLNode* subnode = new XMLNode (it.key(), node);
					subnode->setContents (it.value());
				}
			} break;

			case EFont:
			{	node->setContents (reinterpret_cast<QFont*> (ptr)->toString());
			} break;
		}
	}

	// =============================================================================
	// Load the configuration element @ptr with type @type from XML node @node
	// -----------------------------------------------------------------------------
	static void loadFromXML (void* ptr, EDataType type, XMLNode* node)
	{	switch (type)
		{	case EInt:
				*(reinterpret_cast<int*> (ptr)) = node->getContents().toLong();
				break;

			case EString:
				*(reinterpret_cast<QString*> (ptr)) = node->getContents();
				break;

			case EFloat:
				*(reinterpret_cast<float*> (ptr)) = node->getContents().toFloat();
				break;

			case EBool:
			{	QString val = node->getContents();
				bool& var = *(reinterpret_cast<bool*> (ptr));

				if (val == "true" || val == "1" || val == "on" || val == "yes")
					var = true;
				else
					var = false;
			} break;

			case EStringList:
			{	QStringList& var = *(reinterpret_cast<QStringList*> (ptr));

				for (const XMLNode *subnode : node->getSubnodes())
					var << subnode->getContents();
			} break;

			case EIntList:
			{	QList<int>& var = *(reinterpret_cast<QList<int>*> (ptr));

				for (const XMLNode *subnode : node->getSubnodes())
					var << subnode->getContents().toLong();
			} break;

			case EStringMap:
			{	StringMap& var = *(reinterpret_cast<StringMap*> (ptr));

				for (const XMLNode *subnode : node->getSubnodes())
					var[subnode->getName()] = subnode->getContents();
			} break;

			case EFont:
			{	reinterpret_cast<QFont*> (ptr)->fromString (node->getContents());
			} break;
		}
	}

	// =============================================================================
	// Load the configuration from @fname
	// -----------------------------------------------------------------------------
	bool loadFromFile (QString fname)
	{	log ("config::load: Loading configuration file from %1\n", fname);

		XMLDocument* doc = XMLDocument::loadFromFile (fname);

		if (!doc)
			return false;

		for (ConfigData* i = g_ConfigData; i; i = i->next)
		{	XMLNode* node = doc->navigateTo (QString (i->name).split ("_"));

			if (node)
				loadFromXML (i->ptr, i->type, node);
		}

		g_XMLDocument = doc;
		return true;
	}

	// =============================================================================
	// Save the configuration to @fname
	// -----------------------------------------------------------------------------
	bool saveToFile (QString fname)
	{	if (g_XMLDocument == null)
			g_XMLDocument = XMLDocument::newDocument ("config");

		log ("Saving configuration to %1...\n", fname);

		for (ConfigData* i = g_ConfigData; i; i = i->next)
			saveToXML (i->name, i->ptr, i->type);

		return g_XMLDocument->saveToFile (fname);
	}
	
	// =============================================================================
	// -----------------------------------------------------------------------------
	XMLDocument* xml()
	{	return g_XMLDocument;
	}
}
