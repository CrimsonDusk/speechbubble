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

CONFIG (String, quitmessage, "Bye!")
static QList<IRCConnection*> gConnections;
static const QRegExp gUserMask ("^:([^\\!]+)\\!([^@]+)@(.+)$");

// =============================================================================
//
IRCConnection::IRCConnection (QString host, quint16 port, QObject* parent) :
	QObject (parent),
	m_hostname (host),
	m_port (port),
	m_state (CNS_Disconnected),
	m_ourselves (null),
	m_socket (new QTcpSocket (this)),
	m_timer (new QTimer)
{
	Context* context = new Context (this);
	win->addContext (context);
	setContext (context);
	connect (m_timer, SIGNAL (timeout()), this, SLOT (tick()));
	connect (m_socket, SIGNAL (readyRead()), this, SLOT (readyRead()));
	gConnections << this;
}

// =============================================================================
//
IRCConnection::~IRCConnection()
{
	gConnections.removeOne (this);
}

// =============================================================================
//
void IRCConnection::tick() {}

// =============================================================================
//
void IRCConnection::write (QString text)
{
	m_socket->write (text.toUtf8());
	::print ("<- %1", text);
}

// =============================================================================
//
void IRCConnection::writeLogin()
{
	write (format ("USER %1 * * :%2\n", username(), realname()));
	write (format ("NICK %1\n", nickname()));
	setState (ERegistering);
	print (format (tr ("Registering as \\b%1:%2:%3..."), nickname(), username(), realname()));
	disconnect (m_socket, SIGNAL (connected()));
}

// =============================================================================
//
void IRCConnection::connectToServer()
{
	m_socket->connectToHost (hostname(), port());
	m_timer->start (100);
	setState (EConnecting);
	print (format (tr ("Connecting to \\b%1:%2\\o..."), hostname(), port()));
	connect (m_socket, SIGNAL (connected()), this, SLOT (writeLogin()));
	connect (m_socket, SIGNAL (error (QAbstractSocket::SocketError)),
			 this, SLOT (processConnectionError (QAbstractSocket::SocketError)));
}

// =============================================================================
//
void IRCConnection::disconnectFromServer (QString quitmessage)
{
	if (quitmessage.isEmpty())
		quitmessage = cfg::quitmessage;

	if (state() == EConnected)
		write (format ("QUIT :%1\n", quitmessage));

	m_socket->disconnectFromHost();
	m_timer->stop();
	setState (CNS_Disconnected);
}

// =============================================================================
//
void IRCConnection::print (QString msg)
{
	context()->print (msg);
}

// =============================================================================
//
void IRCConnection::readyRead() // [slot]
{
	QString data = lineWork() + QString (m_socket->readAll());
	data.replace ("\r", "");
	QStringList datalist = data.split ("\n");

	for (auto it = datalist.begin(); it < datalist.end() - 1; ++it)
		processMessage (*it);

	if (datalist.size() > 1)
		setLineWork (* (datalist.end() - 1));
	else
		setLineWork (lineWork() + data);
}

// =============================================================================
//
void IRCConnection::processConnectionError (QAbstractSocket::SocketError err) // [slot]
{
	(void) err;

	print (format (R"(\b\c4Connection error: %1)", m_socket->errorString ()));
	setState (CNS_Disconnected);
}

// =============================================================================
//
void IRCConnection::processMessage (QString msg)
{
	::print ("-> %1\n", msg);
	QStringList tokens = msg.split (" ", QString::SkipEmptyParts);

	if (tokens.size() < 2)
		return;

	if (tokens.size() > 1 && tokens[0] == "PING")
	{
		write ("PONG " + subset (tokens, 1) + "\n");
		return;
	}

	bool ok;
	int num = tokens[1].toInt (&ok);

	if (ok == true)
	{
		parseNumeric (msg, tokens, num);
		return;
	}

	if (tokens[1] == "JOIN")
		processJoin (msg, tokens);
	elif (tokens[1] == "PART")
		processPart (msg, tokens);
	elif (tokens[1] == "QUIT")
		processQuit (msg, tokens);
	elif (tokens[1] == "PRIVMSG")
		processPrivmsg (msg, tokens);
	elif (tokens[1] == "MODE")
		processMode (msg, tokens);
	elif (tokens[1] == "TOPIC")
		processTopicChange (msg, tokens);
}

// =============================================================================
//
void IRCConnection::processJoin (QString msg, QStringList tokens)
{
	if (tokens.size() != 3 || gUserMask.indexIn (tokens[0]) == -1)
	{
		warning (format ("Recieved illegible JOIN from server: %1", msg));
		return;
	}

	QString channame = tokens[2];
	QString joiner = gUserMask.capturedTexts() [1];

	if (Q_LIKELY (channame.startsWith (":")))
		channame.remove (0, 1);

	// Find the channel by name. Create it if we join it, but not if someone joins a
	// channel we know nothing about.
	IRCChannel* chan = findChannel (channame, joiner == ourselves()->nickname());

	if (!chan)
	{
		warning (format ("Recieved JOIN from %1 to unknown channel %2", joiner, channame));
		return;
	}

	// Find a data field for the newcomer. They can be totally new to us so we
	// can create a data field for them if we don't already have one.
	IRCUser* user = findUser (joiner, true);
	assert (joiner != ourselves()->nickname() || user == ourselves());

	if (chan->findUser (user) != null)
	{
		warning (format (tr ("%1 has apparently rejoined %2 without leaving it in the first place?"), joiner, channame));
		return;
	}

	chan->addUser (user);
	QString msgToPrint;

	if (user == ourselves())
		msgToPrint = format (tr ("-> Now talking in %1"), chan->name());
	else
		msgToPrint = format (tr ("-> %1 has joined %2"), user->nickname(), chan->name());

	chan->context()->print (msgToPrint);
}

// =============================================================================
//
void IRCConnection::processPart (QString msg, QStringList tokens)
{
	if (tokens.size() < 3 || gUserMask.indexIn (tokens[0]) == -1)
	{
		warning (format ("Recieved illegible PART from server: %1", msg));
		return;
	}

	QString partmsg;
	QString parter = gUserMask.capturedTexts() [1];
	QString channame = tokens[2];

	if (channame.startsWith (":"))
		channame.remove (0, 1);

	IRCUser* user = findUser (parter, false);
	IRCChannel* chan = findChannel (channame, false);

	if (user == null || chan == null)
	{
		warning (format (tr ("Recieved strange PART from server, apparently \"%1\" "
			"leaves \"%2\"? I don't know that user/channel."), parter, channame));
		return;
	}

	if (tokens.size() >= 4)
	{
		partmsg = subset (tokens, 3);

		if (Q_LIKELY (partmsg.startsWith (":")))
			partmsg.remove (0, 1);
	}

	chan->removeUser (user);

	// If we left the channel, drop it now
	if (user == ourselves())
		delete chan;
	else
		chan->context()->print (format (tr ("<- %1 has left %2%3"), parter, channame,
			(!partmsg.isEmpty() ? (": " + partmsg) : QString())));
}

// =============================================================================
//
void IRCConnection::processQuit (QString msg, QStringList tokens)
{
	if (tokens.size() < 2 || gUserMask.indexIn (tokens[0]) == -1)
	{
		warning (format (tr ("Recieved illegible QUIT from server: %1"), msg));
		return;
	}

	QString quitter = gUserMask.capturedTexts() [1];
	QString quitmessage;
	IRCUser* user = findUser (quitter, false);

	if (!user)
	{
		warning (format (tr ("Recieved strange QUIT from server: apparently some "
			"\"%1\" has quit IRC?"), quitter));
		return;
	}

	if (tokens.size() >= 3)
	{
		quitmessage = subset (tokens, 2);

		if (Q_LIKELY (quitmessage.startsWith (":")))
			quitmessage.remove (0, 1);
	}

	// Announce the quit in all channels he's in
	for (IRCChannel* chan : user->channels())
		chan->context()->print (format (tr ("<- %1 has disconnected%2"),
			quitter, (!quitmessage.isEmpty() ? ": " + quitmessage : QString())));

	delete user;
}

// =============================================================================
//
void IRCConnection::processPrivmsg (QString msg, QStringList tokens)
{
	if (tokens.size() < 4 || gUserMask.indexIn (tokens[0]) == -1)
	{
		warning (format (tr ("Recieved illegible PRIVMSG from server: %1"), msg));
		return;
	}

	QString usernick = gUserMask.capturedTexts()[1];
	IRCUser* user = findUser (usernick, false);
	Context* ctx = null;
	QString message = subset (tokens, 3);

	if (Q_LIKELY (message.startsWith (":")))
		message.remove (0, 1);

	// If the target field is our name, then this is a PM coming to us.
	if (tokens[2] == ourselves()->nickname())
	{
		// Ensure we have a data field for this person now
		if (user == null)
			user = findUser (usernick, true);

		// Also ensure we have a context
		if (user->context() == null)
			user->setContext (new Context (user));

		ctx = user->context();
	}
	else
	{
		IRCChannel* chan = findChannel (tokens[2], false);

		if (!chan)
		{
			warning (format (tr ("Recieved strange PRIVMSG from %1 to \"%2\": %3"),
				usernick, tokens[2], msg));
			return;
		}

		ctx = chan->context();
	}

	// Handle /me here
	if (message.startsWith ("\001ACTION "))
	{
		message.remove (0, strlen ("\001ACTION "));

		// /me is CTCP and should end with \001 but that's not always the case.
		if (message.endsWith ("\001"))
			message.chop (1);

		ctx->writeIRCAction (usernick, message);
		return;
	}

	ctx->writeIRCMessage (usernick, message);
}


// =============================================================================
//
void IRCConnection::processMode (QString msg, QStringList tokens)
{
	if (tokens.size() < 4 || gUserMask.indexIn (tokens[0]) == -1)
	{
		warning (format (tr ("Recieved illegible MODE from server: %1"), msg));
		return;
	}

	QString usernick = gUserMask.capturedTexts()[1];
	QString modestring = subset (tokens, 3, -1);
	IRCChannel* chan = findChannel (tokens[2], false);

	if (chan == null)
	{
		warning (format (tr ("Recieved strange MODE from server: %1"), subset (tokens, 2, -1)));
		return;
	}

	chan->applyModeString (modestring);
	chan->context()->print (format (tr ("* %1 has set mode %2"), usernick, modestring));
}

// =============================================================================
//
void IRCConnection::processTopicChange (QString msg, QStringList tokens)
{
	IRCChannel* chan;

	if (tokens.size() < 4 || (chan = findChannel (tokens[2], false)) == null)
	{
		warning (format (tr ("Recieved illegible TOPIC from server: %1"), msg));
		return;
	}

	QString newtopic = subset (tokens, 3, -1);

	if (newtopic.startsWith (":"))
		newtopic.remove (0, 1);

	QString setterDescription;

	if (gUserMask.indexIn (tokens[0]) != -1)
	{
		setterDescription = format ("%1 (%2@%3)",
			gUserMask.capturedTexts()[1],
			gUserMask.capturedTexts()[2],
			gUserMask.capturedTexts()[3]);
	}
	else
	{
		setterDescription = tokens[0];

		if (Q_LIKELY (setterDescription.startsWith (":")))
			setterDescription.remove (0, 1);
	}

	chan->setTopic (newtopic);
	chan->context()->print (format (tr ("* %1 has set the channel topic to: %2"), setterDescription, newtopic));
}

// =============================================================================
//
void IRCConnection::parseNumeric (QString msg, QStringList tokens, int num)
{
	(void) msg;

	switch (num)
	{
		case Reply_Welcome:
			setState (EConnected);
			print ("\\b\\c3Connected!");

			if (ourselves() == null)
			{
				IRCUser* user = findUser (nickname(), true);
				user->setUsername (username());
				user->setRealname (realname());
				setOurselves (user);
			}
		case Reply_YourHost:
		case Reply_Created:
		case Reply_MotdStart:
		case Reply_Motd:
		case Reply_EndOfMotd:
		{
			QString msg = subset (tokens, 3);

			if (msg[0] == QChar (':'))
				msg.remove (0, 1);

			print (msg);
		} break;

		case Reply_NameReply:
		{
			IRCChannel* chan;

			if (tokens.size() < 6 || (chan = findChannel (tokens[4], false)) == null)
				return;

			QString namestring = subset (tokens, 5, -1);

			if (Q_LIKELY (namestring.startsWith (":")))
				namestring.remove (0, 1);

			chan->addNames (namestring.split (" "));
		} break;

		case Reply_EndOfNames:
		{
			IRCChannel* chan;

			if (tokens.size() < 4 || (chan = findChannel (tokens[3], false)) == null)
				return;

			chan->namesDone();
		} break;

		case Reply_Topic:
		{
			IRCChannel* chan;

			if (tokens.size() < 4 || (chan = findChannel (tokens[3], false)) == null)
				return;

			QString topic = subset (tokens, 4, -1);

			if (Q_LIKELY (topic.startsWith (":")))
				topic.remove (0, 1);

			chan->setTopic (topic);
			chan->context()->print (format (tr ("* Channel topic is: %1"), topic));
		} break;

		case Reply_TopicSetAt:
		{
			IRCChannel* chan;

			if (tokens.size() != 6 || (chan = findChannel (tokens[3], false)) == null)
				return;

			bool ok;
			int time = tokens[5].toLong (&ok);

			if (!ok)
			{
				warning (format (tr ("Bad timestamp for topic of %1, got: %2"), tokens[3], tokens[5]));
				return;
			}

			chan->context()->print (format (tr ("* Topic was set by %1 on %2"), tokens[4],
				QDateTime::fromTime_t (time).toString (Qt::TextDate)));
		} break;
	}
}

// =============================================================================
//
const QList<IRCConnection*>& IRCConnection::getAllConnections()
{
	return gConnections;
}

// =============================================================================
//
void IRCConnection::addChannel (IRCChannel* a)
{
	if (m_channels.contains (a))
		return;

	m_channels << a;
}

// =============================================================================
//
void IRCConnection::removeChannel (IRCChannel* a)
{
	m_channels.removeOne (a);
}

// =============================================================================
//
IRCChannel* IRCConnection::findChannel (QString name, bool createIfNeeded)
{
	for (IRCChannel* chan : channels())
		if (chan->name() == name)
			return chan;

	if (createIfNeeded)
	{
		IRCChannel* chan = new IRCChannel (this, name);
		context()->updateTreeItem();
		return chan;
	}

	return null;
}

// =============================================================================
//
void IRCConnection::warning (QString msg)
{
	print (format ("\\c7\\bWarning:\\o %1", msg));
}

// =============================================================================
//
IRCUser* IRCConnection::findUser (QString nickname, bool createIfNeeded)
{
	for (IRCUser * user : m_users)
		if (user->nickname() == nickname)
			return user;

	if (createIfNeeded)
	{
		IRCUser* user = new IRCUser (this);
		user->setNickname (nickname);
		m_users << user;
		return user;
	}

	return null;
}

// =============================================================================
//
void IRCConnection::forgetUser (IRCUser* user)
{
	m_users.removeOne (user);
}
