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
static QList<IRCConnection*> g_allConnections;
static const QRegExp g_userMask ("^:([^\\!]+)\\!([^@]+)@(.+)$");

// =============================================================================
//
IRCConnection::IRCConnection (QString host, quint16 port, QObject* parent) :
	QObject (parent),
	hostname (host),
	port (port),
	state (CNS_Disconnected),
	ourselves (null),
	socket (new QTcpSocket (this)),
	timer (new QTimer)
{
	context = new Context (this);
	win->addContext (context);
	connect (timer, SIGNAL (timeout()), this, SLOT (tick()));
	connect (socket, SIGNAL (readyRead()), this, SLOT (readyRead()));
	g_allConnections << this;
}

// =============================================================================
//
IRCConnection::~IRCConnection()
{
	g_allConnections.removeOne (this);
}

// =============================================================================
//
void IRCConnection::tick() {}

// =============================================================================
//
void IRCConnection::write (QString text)
{
	socket->write (text.toUtf8());
	::print ("<- %1", text);
}

// =============================================================================
//
void IRCConnection::writeLogin()
{
	write (format ("USER %1 * * :%2\n", username, realname));
	write (format ("NICK %1\n", nickname));
	state = ERegistering;
	print (format (tr ("Registering as \\b%1:%2:%3..."), nickname, username, realname));
	disconnect (socket, SIGNAL (connected()));
}

// =============================================================================
//
void IRCConnection::connectToServer()
{
	socket->connectToHost (hostname, port);
	timer->start (100);
	state = EConnecting;
	print (format (tr ("Connecting to \\b%1:%2\\o..."), hostname, port));
	connect (socket, SIGNAL (connected()), this, SLOT (writeLogin()));
	connect (socket, SIGNAL (error (QAbstractSocket::SocketError)),
		this, SLOT (processConnectionError (QAbstractSocket::SocketError)));
}

// =============================================================================
//
void IRCConnection::disconnectFromServer (QString quitmessage)
{
	if (quitmessage.isEmpty())
		quitmessage = cfg::quitmessage;

	if (state == EConnected)
		write (format ("QUIT :%1\n", quitmessage));

	socket->disconnectFromHost();
	timer->stop();
	state = CNS_Disconnected;
}

// =============================================================================
//
void IRCConnection::print (QString msg)
{
	context->print (msg);
}

// =============================================================================
//
void IRCConnection::readyRead() // [slot]
{
	QString data = lineWork + QString (socket->readAll());
	data.replace ("\r", "");
	QStringList datalist = data.split ("\n");

	for (auto it = datalist.begin(); it < datalist.end() - 1; ++it)
		processMessage (*it);

	if (datalist.size() > 1)
		lineWork = datalist.last();
	else
		lineWork += data;
}

// =============================================================================
//
void IRCConnection::processConnectionError (QAbstractSocket::SocketError err) // [slot]
{
	(void) err;

	print (format (R"(\b\c4Connection error: %1)", socket->errorString()));
	state = CNS_Disconnected;
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
	if (tokens.size() != 3 || g_userMask.indexIn (tokens[0]) == -1)
	{
		warning (format ("Recieved illegible JOIN from server: %1", msg));
		return;
	}

	QString channame = tokens[2];
	QString joiner = g_userMask.capturedTexts() [1];

	if (Q_LIKELY (channame.startsWith (":")))
		channame.remove (0, 1);

	// Find the channel by name. Create it if we join it, but not if someone joins a
	// channel we know nothing about.
	IRCChannel* chan = findChannel (channame, joiner == ourselves->nickname);

	if (chan == null)
	{
		warning (format ("Recieved JOIN from %1 to unknown channel %2", joiner, channame));
		return;
	}

	// Find a data field for the newcomer. They can be totally new to us so we
	// can create a data field for them if we don't already have one.
	IRCUser* user = findUser (joiner, true);
	assert (joiner != ourselves->nickname || user == ourselves);

	if (chan->findUser (user) != null)
	{
		warning (format (tr ("%1 has apparently rejoined %2 without leaving it in the first place?"), joiner, channame));
		return;
	}

	chan->addUser (user);
	QString msgToPrint;

	if (user == ourselves)
		msgToPrint = format (tr ("-> Now talking in %1"), chan->name);
	else
		msgToPrint = format (tr ("-> %1 has joined %2"), user->nickname, chan->name);

	chan->context->print (msgToPrint);
}

// =============================================================================
//
void IRCConnection::processPart (QString msg, QStringList tokens)
{
	if (tokens.size() < 3 || g_userMask.indexIn (tokens[0]) == -1)
	{
		warning (format ("Recieved illegible PART from server: %1", msg));
		return;
	}

	QString partmsg;
	QString parter = g_userMask.capturedTexts()[1];
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
	if (user == ourselves)
		delete chan;
	else
		chan->context->print (format (tr ("<- %1 has left %2%3"), parter, channame,
			(!partmsg.isEmpty() ? (": " + partmsg) : QString())));
}

// =============================================================================
//
void IRCConnection::processQuit (QString msg, QStringList tokens)
{
	if (tokens.size() < 2 || g_userMask.indexIn (tokens[0]) == -1)
	{
		warning (format (tr ("Recieved illegible QUIT from server: %1"), msg));
		return;
	}

	QString quitter = g_userMask.capturedTexts() [1];
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
	for (IRCChannel* chan : user->channels)
		chan->context->print (format (tr ("<- %1 has disconnected%2"),
			quitter, (!quitmessage.isEmpty() ? ": " + quitmessage : QString())));

	delete user;
}

// =============================================================================
//
void IRCConnection::processPrivmsg (QString msg, QStringList tokens)
{
	if (tokens.size() < 4 || g_userMask.indexIn (tokens[0]) == -1)
	{
		warning (format (tr ("Recieved illegible PRIVMSG from server: %1"), msg));
		return;
	}

	QString usernick = g_userMask.capturedTexts()[1];
	IRCUser* user = findUser (usernick, false);
	Context* ctx = null;
	QString message = subset (tokens, 3);

	if (Q_LIKELY (message.startsWith (":")))
		message.remove (0, 1);

	if (tokens[2][0] == '#')
	{
		IRCChannel* chan = findChannel (tokens[2], false);

		if (chan != null)
			ctx = chan->context;
	}
	elif (message.startsWith ("\001") &&
		message.startsWith ("\001ACTION", Qt::CaseInsensitive) == false)
	{
		::print ("recognized as non-action CTCP\n");

		// This is a CTCP message, we can use laxer rules on the context;
		// use the server context if we aren't using a subcontext, unless this
		// is a channel CTCP in which case we need to use the channel context
		// rules (in which case the block above will have caught it) and unless
		// this is /me in which case it should be treated like a normal chat
		// message as far as the context goes.
		if ((ctx = Context::currentContext())->getConnection() != this)
			ctx = context;
	}
	elif (tokens[2] == ourselves->nickname)
	{
		// If the target field is our name, then this is a PM coming to us.
		// Ensure we have a data field for this person now
		if (user == null)
			user = findUser (usernick, true);

		// Also ensure we have a context
		if (user->context == null)
			user->context = new Context (user);

		ctx = user->context;
		::print ("context is user %1's context %2\n", user->nickname, ctx);
	}
	else
	{
		warning (format (tr ("Recieved strange PRIVMSG from %1 to \"%2\": %3"),
			usernick, tokens[2], msg));
		return;
	}

	// Handle CTCP here
	if (message.startsWith ("\001"))
	{
		message.remove (0, 1);

		// CTCP commands should end with \001 but that's not always the case
		if (message.endsWith ("\001"))
			message.chop (1);

		QStringList ctcptokens = message.split (" ");

		if (ctcptokens.isEmpty() == false)
		{
			QString ctcpcmd = ctcptokens.first().toLower();

			if (ctcpcmd == "version")
			{
				write (format ("NOTICE %1 :\001VERSION " APPNAME " %2\001\n",
					usernick, getVersionString()));
			}
			elif (ctcpcmd == "time")
			{
				write (format ("NOTICE %1 :\001TIME %2\001\n",
					usernick, QDateTime::currentDateTime().toString (Qt::TextDate)));
			}
			elif (ctcpcmd == "ping")
			{
				write (format ("NOTICE %1 :\001PING %2\001\n",
					usernick, message.mid (strlen ("PING "))));
			}
			elif (ctcpcmd == "action")
			{
				// ACTION aka /me
				message.remove (0, strlen ("ACTION "));
				ctx->writeIRCAction (usernick, message);
			}

			if (ctcpcmd != "action")
				ctx->print (format ("\\c1Recieved CTCP request from %1: %2",
					usernick, message));
		}
		return;
	}

	ctx->writeIRCMessage (usernick, message);
}


// =============================================================================
//
void IRCConnection::processMode (QString msg, QStringList tokens)
{
	if (tokens.size() < 4 || g_userMask.indexIn (tokens[0]) == -1)
	{
		warning (format (tr ("Recieved illegible MODE from server: %1"), msg));
		return;
	}

	QString usernick = g_userMask.capturedTexts()[1];
	QString modestring = subset (tokens, 3, -1);
	IRCChannel* chan = findChannel (tokens[2], false);

	if (chan == null)
	{
		warning (format (tr ("Recieved strange MODE from server: %1"), subset (tokens, 2, -1)));
		return;
	}

	chan->applyModeString (modestring);
	chan->context->print (format (tr ("* %1 has set mode %2"), usernick, modestring));
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

	if (g_userMask.indexIn (tokens[0]) != -1)
	{
		setterDescription = format ("%1 (%2@%3)",
			g_userMask.capturedTexts()[1],
			g_userMask.capturedTexts()[2],
			g_userMask.capturedTexts()[3]);
	}
	else
	{
		setterDescription = tokens[0];

		if (Q_LIKELY (setterDescription.startsWith (":")))
			setterDescription.remove (0, 1);
	}

	chan->topic = newtopic;
	chan->context->print (format (tr ("* %1 has set the channel topic to: %2"), setterDescription, newtopic));
}

// =============================================================================
//
void IRCConnection::parseNumeric (QString msg, QStringList tokens, int num)
{
	(void) msg;

	switch (num)
	{
		case Reply_Welcome:
			state = EConnected;
			print ("\\b\\c3Connected!");

			if (ourselves == null)
			{
				IRCUser* user = findUser (nickname, true);
				user->username = username;
				user->realname = realname;
				ourselves = user;
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

			chan->topic = topic;
			chan->context->print (format (tr ("* Channel topic is: %1"), topic));
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

			chan->context->print (format (tr ("* Topic was set by %1 on %2"), tokens[4],
				QDateTime::fromTime_t (time).toString (Qt::TextDate)));
		} break;
	}
}

// =============================================================================
//
const QList<IRCConnection*>& IRCConnection::getAllConnections()
{
	return g_allConnections;
}

// =============================================================================
//
void IRCConnection::addChannel (IRCChannel* a)
{
	if (channels.contains (a))
		return;

	channels << a;
}

// =============================================================================
//
void IRCConnection::removeChannel (IRCChannel* a)
{
	channels.removeOne (a);
}

// =============================================================================
//
IRCChannel* IRCConnection::findChannel (QString name, bool createIfNeeded)
{
	for (IRCChannel* chan : channels)
		if (chan->name == name)
			return chan;

	if (createIfNeeded)
	{
		IRCChannel* chan = new IRCChannel (this, name);
		context->updateTreeItem();
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
	for (IRCUser* user : users)
		if (user->nickname == nickname)
			return user;

	if (createIfNeeded)
	{
		IRCUser* user = new IRCUser (this);
		user->nickname = nickname;
		users << user;
		return user;
	}

	return null;
}

// =============================================================================
//
void IRCConnection::forgetUser (IRCUser* user)
{
	users.removeOne (user);
}
