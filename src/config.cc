#include "main.h"
#include "config.h"
#include "xml.h"
#include "format.h"

// =============================================================================
// -----------------------------------------------------------------------------
namespace Config
{	ConfigData              g_ConfigData[COBALT_MAX_CONFIG];
	int                     g_ConfigDataCursor = 0;
	static XMLDocument*     g_XMLDocument = null;

	// =============================================================================
	// -----------------------------------------------------------------------------
	static void SaveToXML (QString name, void* ptr, Type type)
	{	XMLNode* node = g_XMLDocument->NavigateTo (name.split ("_"), true);

		if (!node)
			node = new XMLNode (name, g_XMLDocument->Root());

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
	static void LoadFromXML (void* ptr, Type type, XMLNode* node)
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
				bool& var = *(reinterpret_cast<bool*> (ptr));

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
	bool Load (QString fname)
	{	log ("config::load: Loading configuration file from %1\n", fname);

		XMLDocument* doc = XMLDocument::LoadFromFile (fname);

		if (!doc)
			return false;

		for (auto& i : g_ConfigData)
		{	if (i.name == null)
				break;

			XMLNode* node = doc->NavigateTo (QString (i.name).split ("_"));

			if (node)
				LoadFromXML (i.ptr, i.type, node);
		}

		g_XMLDocument = doc;
		return true;
	}

	// =============================================================================
	// Save the configuration to disk
	// -----------------------------------------------------------------------------
	bool SaveTo (QString fname)
	{	if (g_XMLDocument == null)
			g_XMLDocument = XMLDocument::NewDocument ("config");

		log ("Saving configuration to %1...\n", fname);

		for (auto& i : g_ConfigData)
		{	if (i.name == null)
				break;

			SaveToXML (i.name, i.ptr, i.type);
		}

		return g_XMLDocument->save (fname);
	}

	XMLDocument* xml()
	{	return g_XMLDocument;
	}
}
