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
	m_host (host),
	m_port (port),
	m_state (EDisconnected),
	m_socket (new QTcpSocket (this)),
	m_timer (new QTimer)
{
	Context* context = new Context (this);
	win->AddContext (context);
	set_context (context);
	connect (m_timer, SIGNAL (timeout()), this, SLOT (Tick()));
	connect (m_socket, SIGNAL(readyRead()), this, SLOT (ReadyRead()));
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
void IRCConnection::Tick() {}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::write (QString text)
{	m_socket->write (text.toUtf8());
	log ("<- %1", text);
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::login()
{	write (fmt ("USER %1 * * :%2\n", user(), name()));
	write (fmt ("NICK %1\n", nick()));
	set_state (ERegistering);
	Print (fmt (tr ("Registering as \\b%1:%2:%3...\n"), nick(), user(), name()));
	disconnect (m_socket, SIGNAL (connected()));
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::start()
{	m_socket->connectToHost (host(), port());
	m_timer->start (100);
	set_state (EConnecting);
	Print (fmt (tr ("Connecting \\rto\\r \\b%1:%2\\o...\n"), host(), port()));
	connect (m_socket, SIGNAL (connected()), this, SLOT (login()));
	connect (m_socket, SIGNAL (error (QAbstractSocket::SocketError)),
		this, SLOT (ConnectionError (QAbstractSocket::SocketError)));
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::stop()
{	if (state() == EConnected)
		write (fmt ("QUIT :%1\n", cfg::quitmessage));

	m_socket->disconnectFromHost();
	m_timer->stop();
	set_state (EDisconnected);
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::Print (QString msg, bool allow_internals)
{	context()->Print (msg, allow_internals);
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::ReadyRead() // [slot]
{	QString data = QString (m_socket->readAll());
	QStringList datalist = data.split ("\n", QString::SkipEmptyParts);

	for (auto it = datalist.begin(); it < datalist.end() - 1; ++it)
		Incoming (*it);

	if (datalist.size() > 1)
		m_linework = *(datalist.end() - 1);
	else
		m_linework += data;
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::ConnectionError (QAbstractSocket::SocketError err) // [slot]
{	(void) err;
	Print (fmt ("\\b\\c4Failed to connect: %1\n", m_socket->errorString ()));
	set_state (EDisconnected);
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::Incoming (QString msg)
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
		{	ParseNumeric (msg, tokens, num);
			return;
		}
	}
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCConnection::ParseNumeric (QString msg, QStringList tokens, int num)
{	switch (num)
	{	case ERplWelcome:
			set_state (EConnected);
			Print ("\\b\\c3Connected!\n");
		case ERplYourHost:
		case ERplCreated:
		case ERplMotdStart:
		case ERplMotd:
		case ERplEndOfMotd:
		{	str msg = lrange (tokens, 3);

			if (msg[0] == QChar (':'))
				msg.remove (0, 1);

			Print (fmt ("%1\n", msg));
		} break;
	}
}