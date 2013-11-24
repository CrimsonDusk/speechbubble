#include <QTcpSocket>
#include <QTimer>
#include <QByteArray>
#include <QTextDocument>
#include "connection.h"
#include "context.h"
#include "channel.h"
#include "mainwindow.h"
#include "config.h"
#include "misc.h"
#include "user.h"

CONFIG (String, quitmessage, "Bye.")
static QList<IRCConnection*> g_connections;
static const QRegExp g_UserMask ("^:([^\\!]+)\\!([^@]+)@(.+)$");

// =============================================================================
// -----------------------------------------------------------------------------
IRCConnection::IRCConnection (QString host, quint16 port, QObject* parent) :
	QObject (parent),
	m_Hostname (host),
	m_Port (port),
	m_State (EDisconnected),
	m_Ourselves (null),
	m_socket (new QTcpSocket (this)),
	m_timer (new QTimer)
{
	Context* context = new Context (this);
	win->addContext (context);
	setContext (context);
	connect (m_timer, SIGNAL (timeout()), this, SLOT (tick()));
	connect (m_socket, SIGNAL (readyRead()), this, SLOT (readyRead()));
	g_connections << this;
}

// =============================================================================
// -----------------------------------------------------------------------------
IRCConnection::~IRCConnection()
{	g_connections.removeOne (this);
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::tick() {}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::write (QString text)
{	m_socket->write (text.toUtf8());
	log ("<- %1", text);
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::writeLogin()
{	write (fmt ("USER %1 * * :%2\n", getUser(), getName()));
	write (fmt ("NICK %1\n", getNick()));
	setState (ERegistering);
	print (fmt (tr ("Registering as \\b%1:%2:%3...\n"), getNick(), getUser(), getName()));
	disconnect (m_socket, SIGNAL (connected()));
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::connectToServer()
{	m_socket->connectToHost (getHostname(), getPort());
	m_timer->start (100);
	setState (EConnecting);
	print (fmt (tr ("Connecting to \\b%1:%2\\o...\n"), getHostname(), getPort()));
	connect (m_socket, SIGNAL (connected()), this, SLOT (writeLogin()));
	connect (m_socket, SIGNAL (error (QAbstractSocket::SocketError)),
		this, SLOT (processConnectionError (QAbstractSocket::SocketError)));
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::disconnectFromServer (QString quitmessage)
{	if (quitmessage.isEmpty())
		quitmessage = cfg::quitmessage;

	if (getState() == EConnected)
		write (fmt ("QUIT :%1\n", quitmessage));

	m_socket->disconnectFromHost();
	m_timer->stop();
	setState (EDisconnected);
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::print (QString msg, bool replaceEscapeCodes)
{	getContext()->print (msg, replaceEscapeCodes);
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::readyRead() // [slot]
{	QString data = getLinework() + QString (m_socket->readAll());
	data.replace ("\r", "");
	QStringList datalist = data.split ("\n");

	for (auto it = datalist.begin(); it < datalist.end() - 1; ++it)
		processMessage (*it);

	if (datalist.size() > 1)
		setLinework (*(datalist.end() - 1));
	else
		setLinework (getLinework() + data);
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::processConnectionError (QAbstractSocket::SocketError err) // [slot]
{	(void) err;

	print (fmt ("\\b\\c4Connection error: %1\n", m_socket->errorString ()));
	setState (EDisconnected);
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::processMessage (QString msg)
{	log ("-> %1\n", msg);
	QStringList tokens = msg.split (" ", QString::SkipEmptyParts);

	if (tokens.size() > 2 && tokens[1] == "PING")
	{	write (fmt ("PONG ", lrange (tokens, 2)));
		return;
	}

	if (tokens.size() <= 1)
		return;

	QString numstr = tokens[1];
	bool ok;
	int num = numstr.toInt (&ok);

	if (ok)
	{	parseNumeric (msg, tokens, num);
		return;
	}

	if (tokens[1] == "JOIN")
		processJoin (msg, tokens);
	elif (tokens[1] == "PART")
		processPart (msg, tokens);
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::processJoin (QString msg, QStringList tokens)
{	if (tokens.size() != 3 || g_UserMask.indexIn (tokens[0]) == -1)
	{	warning (fmt ("Recieved illegible JOIN from server: %1", msg));
		return;
	}

	QString channame = tokens[2];
	QString joiner = g_UserMask.capturedTexts() [1];

	if (Q_LIKELY (channame.startsWith (":")))
		channame.remove (0, 1);

	// Find the channel by name. Create it if we join it, but not if someone joins a
	// channel we know nothing about.
	IRCChannel* chan = findChannelByName (channame, joiner == getOurselves()->getNickname());

	if (!chan)
	{	warning (fmt ("Recieved JOIN from %1 to unknown channel %2", joiner, channame));
		return;
	}

	// Find a data field for the newcomer. They can be totally new to us so we
	// can create a data field for them if we don't already have one.
	IRCUser* user = findUserByNick (joiner, true);
	assert (joiner != getOurselves()->getNickname() || user == getOurselves());

	if (chan->findUser (user) != null)
	{	warning (fmt (tr ("%1 has apparently rejoined %2 without leaving it in the first place?"), joiner, channame));
		return;
	}

	chan->addUser (user);
	QString msgToPrint;

	if (user == getOurselves())
		msgToPrint = fmt (tr ("Now talking in %1"), chan->getName());
	else
		msgToPrint = fmt (tr ("%1 has joined %2"), user->getNickname(), chan->getName());

	chan->getContext()->writeIRCMessage (msgToPrint);
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::processPart (QString msg, QStringList tokens)
{	if (tokens.size() < 3 || g_UserMask.indexIn (tokens[0]) == -1)
	{	warning (fmt ("Recieved illegible PART from server: %1", msg));
		return;
	}

	QString partmsg;
	QString parter = g_UserMask.capturedTexts()[1];
	QString channame = tokens[2];

	if (channame.startsWith (":"))
		channame.remove (0, 1);

	IRCUser* user = findUserByNick (parter, false);
	IRCChannel* chan = findChannelByName (channame, false);

	if (!user || !chan)
	{	warning (fmt (tr ("Recieved strange PART from server, apparently \"%1\" "
			"leaves \"%2\"? I don't know that user/channel."), parter, channame));
		return;
	}

	if (tokens.size() >= 4)
	{	partmsg = lrange (tokens, 3);

		if (Q_LIKELY (partmsg.startsWith (":")))
			partmsg.remove (0, 1);
	}

	chan->removeUser (user);

	// If we left the channel, drop it now
	if (user == getOurselves())
		delete chan;
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::parseNumeric (QString msg, QStringList tokens, int num)
{	(void) msg;

	switch (num)
	{	case ERplWelcome:
			setState (EConnected);
			print ("\\b\\c3Connected!\n");

			if (getOurselves() == null)
			{	IRCUser* user = findUserByNick (getNick(), true);
				user->setUsername (getUser());
				user->setRealname (getName());
				setOurselves (user);
			}
		case ERplYourHost:
		case ERplCreated:
		case ERplMotdStart:
		case ERplMotd:
		case ERplEndOfMotd:
		{	QString msg = lrange (tokens, 3);

			if (msg[0] == QChar (':'))
				msg.remove (0, 1);

			print (fmt ("%1\n", msg));
		} break;
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
const QList<IRCConnection*>& IRCConnection::getAllConnections()
{	return g_connections;
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::addChannel (IRCChannel* a)
{	if (m_Channels.contains (a))
		return;

	m_Channels << a;
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::removeChannel (IRCChannel* a)
{	m_Channels.removeOne (a);
}

// =============================================================================
// -----------------------------------------------------------------------------
IRCChannel* IRCConnection::findChannelByName (QString name, bool createIfNeeded)
{	for (IRCChannel* chan : m_Channels)
		if (chan->getName() == name)
			return chan;

	if (createIfNeeded)
	{	IRCChannel* chan = new IRCChannel (this, name);
		getContext()->updateTreeItem();
		return chan;
	}

	return null;
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::warning (QString msg)
{	print (fmt ("\\c7\\bWarning:\\o %1\n", msg));
}

// =============================================================================
// -----------------------------------------------------------------------------
IRCUser* IRCConnection::findUserByNick (QString nickname, bool createIfNeeded)
{	for (IRCUser* user : m_Users)
		if (user->getNickname() == nickname)
			return user;

	if (createIfNeeded)
	{	IRCUser* user = new IRCUser (this);
		user->setNickname (nickname);
		m_Users << user;
		return user;
	}

	return null;
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::forgetUser (IRCUser* user)
{	m_Users.removeOne (user);
}