#include "context.h"
#include "channel.h"
#include "user.h"
#include "connection.h"
#include "mainwindow.h"
#include <QTextDocument>
#include <QTreeWidget>
#include <typeinfo>

static const int g_num_html_colors = 16;
static const QString g_html_colors[g_num_html_colors] =
{	"#FFFFFF", // white
	"#000000", // black
	"#0000A0", // dark blue
	"#008000", // dark green
	"#A00000", // red
	"#800000", // dark red
	"#800080", // dark purple
	"#80FF00", // orange
	"#FFFF00", // yellow
	"#40FF00", // light green
	"#00FFFF", // cyan
	"#8080FF", // sky blue
	"#FF80FF", // light purple
	"#808080", // gray
	"#404040", // dark gray
};

// =============================================================================
// -----------------------------------------------------------------------------
static void WriteColors (QString& out, QString color_1, QString color_2)
{	if (!color_2.isEmpty())
	out += fmt ("<span style=\"color: %1; background-color: %2;\">", color_1, color_2);
	else
		out += fmt ("<span style=\"color: %1;\">", color_1);
}

// =============================================================================
// Converts @in from IRC formatting into HTML formatting. If @allow_internals is
// set, special sequences are also replaced (e.g. "\\b" with bold)
// -----------------------------------------------------------------------------
static QString ConvertToHTML (QString in, bool allow_internals)
{	QString			color_1,
						color_2;
	bool				bold_active = false;
	const QRegExp	color_mask ("(\\d\\d?)(;\\d\\d?)?"); // regex for colors, e.g. 4;12
	QString			out;
	bool				underline_active = false;

	// The string more than most probably does not end with a normalizer character,
	// so append one now. This ensures all tags are closed properly and nothing will
	// bleed out of this string into the document.
	in += NORMAL_STR;

	// If we allow internal characters, replace them now. Internal codes are allowed in
	// internal printing, since it also uses this function and writing "\\bargh" is more
	// convenient than writing BOLD_STR "argh". @allow_internals is false in privmsg
	// messages coming from the IRC server so that if someone includes \u in their actual
	// message it doesn't turn into an underline formatting code.
	if (allow_internals)
	{	in.replace ("\\b", BOLD_STR);
		in.replace ("\\c", COLOR_STR);
		in.replace ("\\o", NORMAL_STR);
		in.replace ("\\r", REVERSE_STR);
		in.replace ("\\u", UNDERLINE_STR);
	}

	// Replace some special characters with entities so the HTML doesn't get messed up.
	// note: `&` first! otherwise `<` will turn into &amp;lt;
	in.replace ("&", "&amp;");
	in.replace ("<", "&lt;");
	in.replace (">", "&gt;");

	// Now handle mIRC-like formatting
	for (int i = 0; i < in.size(); ++i)
	{	switch (in[i].toAscii())
		{	case BOLD_CHAR:
			{	toggle (bold_active);

				if (bold_active)
					out += "<b>";
				else
					out += "</b>";
			} break;

			case UNDERLINE_CHAR:
			{	toggle (underline_active);

				if (underline_active)
					out += "<u>";
				else
					out += "</u>";
			} break;

			case COLOR_CHAR:
			{	if (color_mask.indexIn (in.mid (i + 1)) != -1)
				{	assert (color_mask.capturedTexts().size() == 3);
					QString	num1str = color_mask.capturedTexts()[1],
								num2str = color_mask.capturedTexts()[2];
					int		num1,
								num2 = -1;

					num1 = num1str.toInt();

					if (!num2str.isEmpty())
					{	// The regex capture includes the separating ';' as well, rid
						// it now.
						if (Q_LIKELY (num2str[0] == ';'))
							num2str.remove (0, 1);

						// note: the regexp does not allow num2str to be -1 since
						// it only allows digits, so we don't need to worry about
						// someone passing \\c4;-1 and causing unwanted behavior.
						num2 = num2str.toInt();
						assert (num2 != -1);
					}

					if (within_range (num1, 0, 15) && within_range (num2, -1, 15))
					{	color_1 = g_html_colors[num1];
						color_2 = (num2 != -1) ? g_html_colors[num2] : "";

						WriteColors (out, color_1, color_2);
						i += color_mask.matchedLength();
					}
				}
				elif (!color_1.isEmpty())
				{	out += "</span>";
					color_1 = color_2 = "";
				}
			} break;

			case NORMAL_CHAR:
			{	if (bold_active)
				{	out += "</b>";
					bold_active = false;
				}

				if (underline_active)
				{	out += "</u>";
					underline_active = false;
				}

				if (!color_1.isEmpty())
				{	out += "</span>";
					color_1 = color_2 = "";
				}
			} break;

			case REVERSE_CHAR:
			{	if (color_1.isEmpty())
					color_1 = g_html_colors[0];
				else
					out += "</span>";

				if (color_2.isEmpty())
					color_2 = g_html_colors[1];

				color_1.swap (color_2);
				WriteColors (out, color_1, color_2);
			} break;

			// for internal use:
			case '\n':
			{	out += "<br />";
			} break;

			default:
			{	out += in[i];
			} break;
		}
	}

	return out;
}

Context* g_current_context = null;
static QList<Context*> g_contexts;
static QMap<int, Context*> g_contexts_by_id;

// =============================================================================
// -----------------------------------------------------------------------------
Context::Context (IRCConnection* conn) : QObject(), m_type (EServerContext)
{	m_target.conn = conn;
	set_parent (null);
	common_init();
	win->AddContext (this);
}

// =============================================================================
// -----------------------------------------------------------------------------
Context::Context (IRCChannel* channel) : QObject(), m_type (EChannelContext)
{	m_target.chan = channel;
	set_parent (channel->connection()->context());
	common_init();
}

// =============================================================================
// -----------------------------------------------------------------------------
Context::Context (IRCUser* user) : QObject(), m_type (EQueryContext)
{	m_target.user = user;
	set_parent (user->connection()->context());
	common_init();
}

// =============================================================================
// -----------------------------------------------------------------------------
void Context::common_init()
{	set_id (1);
	while (g_contexts_by_id.find (id()) != g_contexts_by_id.end())
		set_id (id() + 1);

	set_treeitem (new IRCContextTreeWidgetItem (this));
	set_document (new QTextDocument);

	if (parent())
		parent()->add_subcontext (this);

	g_contexts << this;
	g_contexts_by_id[id()] = this;

	/*
	log ("initialized context with id %1 (parent: %2)\n",
		id(), parent() ? QString::number (parent()->id()) : "none");
	*/
}

// =============================================================================
// -----------------------------------------------------------------------------
Context::~Context()
{	if (parent())
		parent()->forget_subcontext (this);

	g_contexts.removeOne (this);
	g_contexts_by_id.remove (id());
	delete treeitem();
}

// =============================================================================
// -----------------------------------------------------------------------------
const QList<Context*>& Context::all_contexts() // [static]
{	return g_contexts;
}

// =============================================================================
// -----------------------------------------------------------------------------
void Context::update_tree_item()
{	treeitem()->setText (0, name());

	for (Context* sub : subcontexts())
		sub->update_tree_item();
}

// =============================================================================
// -----------------------------------------------------------------------------
void Context::add_subcontext (Context* child)
{	m_subcontexts << child;
	child->set_parent (this);
	treeitem()->addChild (child->treeitem());
}

// =============================================================================
// -----------------------------------------------------------------------------
void Context::forget_subcontext (Context* child)
{	m_subcontexts.removeOne (child);
	child->set_parent (null);
}

// =============================================================================
// -----------------------------------------------------------------------------
Context* Context::from_tree_widget_item (QTreeWidgetItem* item)
{	IRCContextTreeWidgetItem* subitem = dynamic_cast<IRCContextTreeWidgetItem*> (item);
	assert (subitem != null);
	return subitem->context();
}

// =============================================================================
// -----------------------------------------------------------------------------
Context* Context::CurrentContext()
{	return g_current_context;
}

// =============================================================================
// -----------------------------------------------------------------------------
void Context::set_current_context (Context* context)
{	g_current_context = context;
	win->UpdateOutputWidget();
}

// =============================================================================
// -----------------------------------------------------------------------------
QString Context::name() const
{	switch (type())
	{	case EChannelContext:
		{	return target().chan->name();
		} break;

		case EQueryContext:
		{	return target().user->nick();
		} break;

		case EServerContext:
		{	return target().conn->host();
		} break;
	}

	return QString();
}

// =============================================================================
// -----------------------------------------------------------------------------
void Context::Print (QString text, bool allow_internals)
{	m_html += ConvertToHTML (text, allow_internals);
	m_document->setHtml (m_html);
}

// =============================================================================
// -----------------------------------------------------------------------------
IRCConnection* Context::GetConnection()
{	switch (type())
	{	case EQueryContext:
		{	return target().user->connection();
		} break;

		case EChannelContext:
		{	return target().chan->connection();
		} break;

		case EServerContext:
		{	return target().conn;
		} break;
	}

	return null;
}

// =============================================================================
// -----------------------------------------------------------------------------
void Context::WriteIRCMessage (QString from, QString msg)
{	Print (fmt ("[%1] <\\b%2\\b> %3\n",
		QTime::currentTime().toString (Qt::TextDate), from, msg), false);
}