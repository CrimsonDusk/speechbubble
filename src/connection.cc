#include <QTcpSocket>
#include <QTimer>
#include <QByteArray>
#include <QTextDocument>
#include "connection.h"
#include "context.h"
#include "channel.h"
#include "mainwindow.h"
#include "config.h"

CONFIG (String, quitmessage, "Bye.")
static QList<IRCConnection*> g_connections;

// =============================================================================
// Joins the indices [@a, @b] of @list
// -----------------------------------------------------------------------------
static str lrange (const QStringList& list, int a, int b = -1)
{	if (b == -1)
		b = list.size() - 1;

	assert (list.size() > a && list.size() > b && b > a);
	str out;

	for (auto it = list.begin() + a; it <= list.begin() + b; ++it)
	{	if (!out.isEmpty())
			out += " ";

		out += *it;
	}

	return out;
}

// =============================================================================
// -----------------------------------------------------------------------------
IRCConnection::IRCConnection (QString host, quint16 port, QObject* parent) :
	QObject (parent),
	m_Hostname (host),
	m_Port (port),
	m_State (EDisconnected),
	m_socket (new QTcpSocket (this)),
	m_timer (new QTimer)
{
	Context* context = new Context (this);
	win->addContext (context);
	setContext (context);
	connect (m_timer, SIGNAL (timeout()), this, SLOT (tick()));
	connect (m_socket, SIGNAL(readyRead()), this, SLOT (readyRead()));
	g_connections << this;

	IRCChannel* chan = new IRCChannel (this, "#argho");
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
	print (fmt (tr ("Connecting \\rto\\r \\b%1:%2\\o...\n"), getHostname(), getPort()));
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
void IRCConnection::print (QString msg, bool allow_internals)
{	getContext()->print (msg, allow_internals);
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::readyRead() // [slot]
{	QString data = getLinework() + QString (m_socket->readAll());
	QStringList datalist = data.split ("\n", QString::SkipEmptyParts);

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

	if (tokens.size() > 1)
	{	str numstr = tokens[1];
		bool ok;
		int num = numstr.toInt (&ok);

		if (ok)
		{	parseNumeric (msg, tokens, num);
			return;
		}
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::parseNumeric (QString msg, QStringList tokens, int num)
{	switch (num)
	{	case ERplWelcome:
			setState (EConnected);
			print ("\\b\\c3Connected!\n");
		case ERplYourHost:
		case ERplCreated:
		case ERplMotdStart:
		case ERplMotd:
		case ERplEndOfMotd:
		{	str msg = lrange (tokens, 3);

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