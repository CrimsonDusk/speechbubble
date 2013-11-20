#include "context.h"
#include "channel.h"
#include "user.h"
#include "connection.h"
#include "mainwindow.h"
#include <QTextDocument>
#include <QTreeWidget>
#include <typeinfo>

static const QString g_HTMLColors[] =
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

static const int g_NumHTMLColors = (sizeof g_HTMLColors / sizeof *g_HTMLColors);

// =============================================================================
// -----------------------------------------------------------------------------
static void WriteColors (QString& out, QString color1, QString color2)
{	if (!color2.isEmpty())
	out += fmt ("<span style=\"color: %1; background-color: %2;\">", color1, color2);
	else
		out += fmt ("<span style=\"color: %1;\">", color1);
}

// =============================================================================
// Converts @in from IRC formatting into HTML formatting. If @allow_internals is
// set, special sequences are also replaced (e.g. "\\b" with bold)
// -----------------------------------------------------------------------------
static QString ConvertToHTML (QString in, bool allow_internals)
{	QString			color1,
						color2;
	bool				boldactive = false;
	const QRegExp	colormask ("(\\d\\d?)(;\\d\\d?)?"); // regex for colors, e.g. 4;12
	QString			out;
	bool				underlineactive = false;

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
			{	toggle (boldactive);

				if (boldactive)
					out += "<b>";
				else
					out += "</b>";
			} break;

			case UNDERLINE_CHAR:
			{	toggle (underlineactive);

				if (underlineactive)
					out += "<u>";
				else
					out += "</u>";
			} break;

			case COLOR_CHAR:
			{	if (colormask.indexIn (in.mid (i + 1)) != -1)
				{	assert (colormask.capturedTexts().size() == 3);
					QString	num1str = colormask.capturedTexts()[1],
								num2str = colormask.capturedTexts()[2];
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

					if (within_range (num1, 0, g_NumHTMLColors - 1) && within_range (num2, -1, g_NumHTMLColors - 1))
					{	color1 = g_HTMLColors[num1];
						color2 = (num2 != -1) ? g_HTMLColors[num2] : "";

						WriteColors (out, color1, color2);
						i += colormask.matchedLength();
					}
				}
				elif (!color1.isEmpty())
				{	out += "</span>";
					color1 = color2 = "";
				}
			} break;

			case NORMAL_CHAR:
			{	if (boldactive)
				{	out += "</b>";
					boldactive = false;
				}

				if (underlineactive)
				{	out += "</u>";
					underlineactive = false;
				}

				if (!color1.isEmpty())
				{	out += "</span>";
					color1 = color2 = "";
				}
			} break;

			case REVERSE_CHAR:
			{	if (color1.isEmpty())
					color1 = g_HTMLColors[0];
				else
					out += "</span>";

				if (color2.isEmpty())
					color2 = g_HTMLColors[1];

				color1.swap (color2);
				WriteColors (out, color1, color2);
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

Context* g_CurrentContext = null;
static QList<Context*> g_AllContexts;
static QMap<int, Context*> g_ContextsByID;

// =============================================================================
// -----------------------------------------------------------------------------
Context::Context (IRCConnection* conn) : QObject(), m_Type (EServerContext)
{	TargetUnion u;
	u.conn = conn;
	setTarget (u);
	setParent (null);
	CommonInit();
	win->addContext (this);
}

// =============================================================================
// -----------------------------------------------------------------------------
Context::Context (IRCChannel* channel) : QObject(), m_Type (EChannelContext)
{	TargetUnion u;
	u.chan = channel;
	setTarget (u);
	setParent (channel->getConnection()->getContext());
	CommonInit();
}

// =============================================================================
// -----------------------------------------------------------------------------
Context::Context (IRCUser* user) : QObject(), m_Type (EQueryContext)
{	TargetUnion u;
	u.user = user;
	setTarget (u);
	setParent (user->getConnection()->getContext());
	CommonInit();
}

// =============================================================================
// -----------------------------------------------------------------------------
void Context::CommonInit()
{	setID (1);
	while (g_ContextsByID.find (getID()) != g_ContextsByID.end())
		setID (getID() + 1);

	setTreeItem (new IRCContextTreeWidgetItem (this));
	setDocument (new QTextDocument);

	if (getParent())
		getParent()->addSubContext (this);

	g_AllContexts << this;
	g_ContextsByID[getID()] = this;

	/*
	log ("initialized context with id %1 (parent: %2)\n",
		getID(), parent() ? QString::number (parent()->getID()) : "none");
	*/
}

// =============================================================================
// -----------------------------------------------------------------------------
Context::~Context()
{	if (getParent())
		getParent()->forgetSubContext (this);

	g_AllContexts.removeOne (this);
	g_ContextsByID.remove (getID());
	delete getTreeItem();
}

// =============================================================================
// -----------------------------------------------------------------------------
const QList<Context*>& Context::getAllContexts() // [static]
{	return g_AllContexts;
}

// =============================================================================
// -----------------------------------------------------------------------------
void Context::UpdateTreeItem()
{	getTreeItem()->setText (0, getName());

	for (Context* sub : getSubcontexts())
		sub->UpdateTreeItem();
}

// =============================================================================
// -----------------------------------------------------------------------------
void Context::addSubContext (Context* child)
{	m_Subcontexts << child;
	child->setParent (this);
	getTreeItem()->addChild (child->getTreeItem());
}

// =============================================================================
// -----------------------------------------------------------------------------
void Context::forgetSubContext (Context* child)
{	m_Subcontexts.removeOne (child);
	child->setParent (null);
}

// =============================================================================
// -----------------------------------------------------------------------------
Context* Context::getFromTreeWidgetItem (QTreeWidgetItem* item)
{	IRCContextTreeWidgetItem* subitem;

#ifndef RELEASE
	subitem = dynamic_cast<IRCContextTreeWidgetItem*> (item);
	assert (subitem != null);
#else
	subitem = reinterpret_cast<IRCContextTreeWidgetItem*> (item);
#endif

	return subitem->getContext();
}

// =============================================================================
// -----------------------------------------------------------------------------
Context* Context::getCurrentContext() // [static]
{	return g_CurrentContext;
}

// =============================================================================
// -----------------------------------------------------------------------------
void Context::setCurrentContext (Context* context) // [static]
{	g_CurrentContext = context;
	win->updateOutputWidget();
}

// =============================================================================
// -----------------------------------------------------------------------------
QString Context::getName() const
{	switch (getType())
	{	case EChannelContext:
		{	return getTarget().chan->getName();
		} break;

		case EQueryContext:
		{	return getTarget().user->getNickname();
		} break;

		case EServerContext:
		{	return getTarget().conn->getHostname();
		} break;
	}

	return QString();
}

// =============================================================================
// -----------------------------------------------------------------------------
void Context::print (QString text, bool allow_internals)
{	setHTML (getHTML() + ConvertToHTML (text, allow_internals));
	getDocument()->setHtml (getHTML());
}

// =============================================================================
// -----------------------------------------------------------------------------
IRCConnection* Context::getConnection()
{	switch (getType())
	{	case EQueryContext:
		{	return getTarget().user->getConnection();
		} break;

		case EChannelContext:
		{	return getTarget().chan->getConnection();
		} break;

		case EServerContext:
		{	return getTarget().conn;
		} break;
	}

	return null;
}

// =============================================================================
// -----------------------------------------------------------------------------
void Context::writeIRCMessage (QString from, QString msg)
{	print (fmt ("[%1] <\\b%2\\b> %3\n",
		QTime::currentTime().toString (Qt::TextDate), from, msg), false);
}