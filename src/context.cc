#include "context.h"
#include "channel.h"
#include "user.h"
#include "connection.h"
#include "mainwindow.h"
#include "misc.h"
#include <QTextDocument>
#include <QTreeWidget>
#include <typeinfo>

static const QString gColorCodes[] =
{
	"#FFFFFF", // white
	"#000000", // black
	"#0000A0", // dark blue
	"#008000", // dark green
	"#A00000", // red
	"#800000", // dark red
	"#800080", // dark purple
	"#FF8000", // orange
	"#FFFF00", // yellow
	"#40FF00", // light green
	"#00FFFF", // cyan
	"#8080FF", // sky blue
	"#FF80FF", // light purple
	"#808080", // gray
	"#404040", // dark gray
};

static const int						gNumColorCodes = COUNT_OF (gColorCodes);
static QMap<QTreeWidgetItem*, Context*>	gContextsByTreeItem;
static Context*							gCurrentContext = null;
static QList<Context*>					gAllContexts;
static QMap<int, Context*>				gContextsByID;

// =============================================================================
//
static void writeColors (QString& out, QString color1, QString color2)
{
	if (!color2.isEmpty())
		out += format ("<span style=\"color: %1; background-color: %2;\">", color1, color2);
	else
		out += format ("<span style=\"color: %1;\">", color1);
}

// =============================================================================
//
// Converts @in from IRC formatting into HTML formatting. If @replaceEscapeCodes is
// set, special sequences are also replaced (e.g. "\\b" with bold)
//
enum EConversionFlag
{
	FReplaceEscapeCodes	= (1 << 0),
	FRemoveCodes		= (1 << 1),
};

Q_DECLARE_FLAGS (FConversionFlags, EConversionFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS (FConversionFlags)

static QString convertToHTML (QString in, FConversionFlags flags)
{
	QString			color1,
					color2;
	bool			boldactive = false;
	const QRegExp	colorMask ("(\\d\\d?)(;\\d\\d?)?"); // regex for colors, e.g. 4;12
	QString			out;
	bool			underlineActive = false;

	// The string more than most probably does not end with a normalizer character,
	// so append one now. This ensures all tags are closed properly and nothing will
	// bleed out of this string into the document.
	in += NORMAL_STR;

	// Replace escape codes now. We allow this in internal printing, since it also uses
	// this function and writing "\\bargh" is more convenient than writing BOLD_STR
	// "argh". @replaceEscapeCodes is false in privmsg messages coming from the IRC
	// server so that if someone includes \u in their actual message it doesn't turn
	// into an underline formatting code.
	if (flags & FReplaceEscapeCodes)
	{
		in.replace ("\\b", BOLD_STR);
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
	{
		switch (in[i].toAscii())
		{
		case BOLD_CHAR:
		{
			toggle (boldactive);
			out += (boldactive) ? "<b>" : "</b>";
		} break;

		case UNDERLINE_CHAR:
		{
			toggle (underlineActive);
			out += (underlineActive) ? "<u>" : "</u>";
		} break;

		case COLOR_CHAR:
		{
			if (colorMask.indexIn (in.mid (i + 1)) != -1)
			{
				assert (colorMask.capturedTexts().size() == 3);
				QString	num1str = colorMask.capturedTexts() [1],
						num2str = colorMask.capturedTexts() [2];
				int		num1,
						num2 = -1;

				num1 = num1str.toInt();

				if (!num2str.isEmpty())
				{
					// The regex capture includes the separating ';' as well, rid
					// it now.
					assert (num2str[0] == ';');
					num2str.remove (0, 1);

					// Note: the regexp does not allow num2str to be -1 since
					// it only allows digits, so assert is enough here.
					num2 = num2str.toInt();
					assert (num2 != -1);
				}

				if (isWithinRange (num1, 0, gNumColorCodes - 1) && isWithinRange (num2, -1, gNumColorCodes - 1))
				{
					color1 = gColorCodes[num1];
					color2 = (num2 != -1) ? gColorCodes[num2] : "";

					writeColors (out, color1, color2);
					i += colorMask.matchedLength();
				}
			}

			elif (!color1.isEmpty())
			{
				out += "</span>";
				color1 = color2 = "";
			}
		} break;

		case NORMAL_CHAR:
		{
			if (boldactive)
			{
				out += "</b>";
				boldactive = false;
			}

			if (underlineActive)
			{
				out += "</u>";
				underlineActive = false;
			}

			if (!color1.isEmpty())
			{
				out += "</span>";
				color1 = color2 = "";
			}
		} break;

		case REVERSE_CHAR:
		{
			if (color1.isEmpty())
				color1 = gColorCodes[1];
			else
				out += "</span>";

			if (color2.isEmpty())
				color2 = gColorCodes[0];

			color1.swap (color2);
			writeColors (out, color1, color2);
		} break;

		case '\n':
		{
			out += "<br />";
		} break;

		default:
		{
			out += in[i];
		} break;
		}
	}

	return out;
}

// =============================================================================
//
Context::Context (IRCConnection* conn) :
	QObject(),
	m_type (CTX_Server)
{
	TargetUnion u;
	u.conn = conn;
	setTarget (u);
	setParent (null);
	commonInit();
	win->addContext (this);
}

// =============================================================================
//
Context::Context (IRCChannel* channel) :
	QObject(),
	m_type (CTX_Channel)
{
	TargetUnion u;
	u.chan = channel;
	setTarget (u);
	setParent (channel->connection()->context());
	commonInit();

	IRCChannel::connect (channel, SIGNAL (userlistChanged()), win, SLOT (updateUserlist()));
}

// =============================================================================
//
Context::Context (IRCUser* user) :
	QObject(),
	m_type (CTX_Query)
{
	TargetUnion u;
	u.user = user;
	setTarget (u);
	setParent (user->connection()->context());
	commonInit();
}

// =============================================================================
//
void Context::commonInit()
{
	setID (1);

	while (gContextsByID.find (id()) != gContextsByID.end())
		setID (id() + 1);

	setTreeItem (new QTreeWidgetItem);
	setDocument (new QTextDocument);

	if (parent() != null)
		parent()->addSubContext (this);

	gAllContexts << this;
	gContextsByID[id()] = this;
	gContextsByTreeItem[treeItem()] = this;
}

// =============================================================================
//
Context::~Context()
{
	if (parent())
		parent()->forgetSubContext (this);

	gAllContexts.removeOne (this);
	gContextsByID.remove (id());
	delete treeItem();
}

// =============================================================================
//
const QList<Context*>& Context::allContexts() // [static]
{
	return gAllContexts;
}

// =============================================================================
//
void Context::updateTreeItem()
{
	treeItem()->setText (0, name());

	for (Context * sub : subContexts())
		sub->updateTreeItem();
}

// =============================================================================
//
void Context::addSubContext (Context* child)
{
	m_subContexts << child;
	child->setParent (this);
	treeItem()->addChild (child->treeItem());
	updateTreeItem();
}

// =============================================================================
//
void Context::forgetSubContext (Context* child)
{
	m_subContexts.removeOne (child);
	child->setParent (null);
}

// =============================================================================
//
Context* Context::fromTreeWidgetItem (QTreeWidgetItem* item) // [static]
{
	return gContextsByTreeItem[item];
}

// =============================================================================
//
Context* Context::currentContext() // [static]
{
	return gCurrentContext;
}

// =============================================================================
//
void Context::setCurrentContext (Context* context) // [static]
{
	gCurrentContext = context;
	win->updateOutputWidget();
}

// =============================================================================
//
QString Context::name() const
{
	switch (type())
	{
	case CTX_Channel:
	{
		return target().chan->name();
		break;
	}

	case CTX_Query:
	{
		return target().user->nickname();
		break;
	}

	case CTX_Server:
	{
		return target().conn->hostname();
		break;
	}
	}

	return QString();
}

// =============================================================================
//
void Context::rawPrint (QString msg, bool replaceEscapeCodes)
{
	FConversionFlags flags = 0;

	if (replaceEscapeCodes)
		flags |= FReplaceEscapeCodes;

	setHTML (html() + convertToHTML (msg, flags));
	document()->setHtml (html());
}

// =============================================================================
//
void Context::printTimestamp()
{
	QString tstamp = QDateTime::currentDateTime().toString ("hh:mm:ss");
	rawPrint (format ("\\c2[%1]\\o ", tstamp), true);
}

// =============================================================================
//
void Context::print (QString text)
{
	printTimestamp();
	rawPrint (text + "\n", true);
}

// =============================================================================
//
IRCConnection* Context::connection()
{
	switch (type())
	{
	case CTX_Query:
		return target().user->connection();
		break;

	case CTX_Channel:
		return target().chan->connection();
		break;

	case CTX_Server:
		return target().conn;
		break;
	}

	return null;
}

// =============================================================================
//
void Context::writeIRCMessage (QString from, QString msg)
{
	printTimestamp();
	rawPrint (format ("<\\b%1\\b> ", from), true);
	rawPrint (msg + "\n", false);
}

// =============================================================================
//
void Context::writeIRCAction (QString from, QString msg)
{
	printTimestamp();
	rawPrint (format ("* \\b%1\\b ", from), true);
	rawPrint (msg + "\n", false);
}
