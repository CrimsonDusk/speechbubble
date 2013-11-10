#include "main.h"
#include "config.h"
#include "xml.h"
#include "format.h"

// =============================================================================
// -----------------------------------------------------------------------------
namespace Config
{	ConfigData              g_config_data[COBALT_MAX_CONFIG];
	int                     g_cfg_data_cursor = 0;
	static XMLDocument*     g_xml_document = null;

	// =============================================================================
	// -----------------------------------------------------------------------------
	static void saveToXML (QString name, void* ptr, Type type)
	{	XMLNode* node = g_xml_document->navigate_to (name.split ("_"), true);

		if (!node)
			node = new XMLNode (name, g_xml_document->root());

		switch (type)
		{	case IntType:
			{	node->set_contents (QString::number (*(reinterpret_cast<int*> (ptr))));
			} break;

			case StringType:
			{	node->set_contents (*(reinterpret_cast<QString*> (ptr)));
			} break;

			case FloatType:
			{	node->set_contents (QString::number (*(reinterpret_cast<float*> (ptr))));
			} break;

			case BoolType:
			{	node->set_contents (*(reinterpret_cast<bool*> (ptr)) ? "true" : "false");
			} break;

			case StringListType:
			{	for (XMLNode* subnode : node->nodes())
					delete subnode;

				for (QString item : *(reinterpret_cast<StringList*> (ptr)))
				{	XMLNode* subnode = new XMLNode ("item", node);
					subnode->set_contents (item);
				}
			} break;

			case IntListType:
			{	for (int item : *(reinterpret_cast<IntList*> (ptr)))
				{	XMLNode* subnode = new XMLNode ("item", node);
					subnode->set_contents (QString::number (item));
				}
			} break;

			case StringMapType:
			{	const StringMap& map = *(reinterpret_cast<StringMap*> (ptr));

				for (auto it = map.begin(); it != map.end(); ++it)
				{	XMLNode* subnode = new XMLNode (it.key(), node);
					subnode->set_contents (it.value());
				}
			} break;
		}
	}

	// =============================================================================
	// -----------------------------------------------------------------------------
	static void loadFromXML (void* ptr, Type type, XMLNode* node)
	{	switch (type)
		{	case IntType:
				*(reinterpret_cast<int*> (ptr)) = node->contents().toLong();
				break;

			case StringType:
				*(reinterpret_cast<QString*> (ptr)) = node->contents();
				break;

			case FloatType:
				*(reinterpret_cast<float*> (ptr)) = node->contents().toFloat();
				break;

			case BoolType:
			{	QString val = node->contents();
				bool& var = * (reinterpret_cast<bool*> (ptr));

				if (val == "true" || val == "1" || val == "on" || val == "yes")
					var = true;
				else
					var = false;
			} break;

			case StringListType:
			{	QStringList& var = *(reinterpret_cast<QStringList*> (ptr));

				for (const XMLNode *subnode : node->nodes())
					var << subnode->contents();
			} break;

			case IntListType:
			{	QList<int>& var = *(reinterpret_cast<QList<int>*> (ptr));

				for (const XMLNode *subnode : node->nodes())
					var << subnode->contents().toLong();
			} break;

			case StringMapType:
			{	StringMap& var = *(reinterpret_cast<StringMap*> (ptr));

				for (const XMLNode *subnode : node->nodes())
					var[subnode->name()] = subnode->contents();
			} break;
		}
	}

	// =============================================================================
	// Load the configuration from file
	// -----------------------------------------------------------------------------
	bool load (QString fname)
	{	log ("config::load: Loading configuration file from %1\n", fname);

		XMLDocument* doc = XMLDocument::load (fname);

		if (!doc)
			return false;

		for (auto& i : g_config_data)
		{	if (i.name == null)
				break;

			XMLNode* node = doc->navigate_to (QString (i.name).split ("_"));

			if (node)
				loadFromXML (i.ptr, i.type, node);
		}

		g_xml_document = doc;
		return true;
	}

	// =============================================================================
	// Save the configuration to disk
	// -----------------------------------------------------------------------------
	bool save (QString fname)
	{	if (g_xml_document == null)
			g_xml_document = XMLDocument::new_document ("config");

		log ("Saving configuration to %1...\n", fname);

		for (auto& i : g_config_data)
		{	if (i.name == null)
				break;

			saveToXML (i.name, i.ptr, i.type);
		}

		return g_xml_document->save (fname);
	}

	XMLDocument* xml()
	{	return g_xml_document;
	}
}
