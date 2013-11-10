#ifndef COBALTIRC_USER_H
#define COBALTIRC_USER_H

#include "main.h"
#include "channel.h"

class Context;
class IRCConnection;

class IRCUser
{	public:
		enum Flag
		{	Away     = (1 << 0),    // is /AWAY
			IRCOp    = (1 << 1),    // is an IRC Op
		};

		Q_DECLARE_FLAGS (Flags, Flag)

	PROPERTY (public,  QString,            nick)
	PROPERTY (public,  QString,            user)
	PROPERTY (public,  QString,            host)
	PROPERTY (public,  QString,            name)
	PROPERTY (public,  QString,            server)
	PROPERTY (public,  QString,            account)
	PROPERTY (public,  Flags,              flags)
	PROPERTY (public,  Context*,        context)
	PROPERTY (private, IRCConnection*,     connection)
	PROPERTY (private, QList<IRCChannel*>, channels) 

	public:
		IRCUser (IRCConnection* conn) :
			m_flags (0),
			m_connection (conn) {}

		void               add_known_channel (IRCChannel* chan);
		IRCChannel::Status channel_status (IRCChannel* chan);
		void               del_known_channel (IRCChannel* chan);
		QString            string_rep() const;
		QString            userhost() const;

		Flags operator| (Flags f) const;
		Flags operator& (Flags f) const;
		Flags operator^ (Flags f) const;
		IRCUser operator|= (Flags f);
		IRCUser operator&= (Flags f);
		IRCUser operator^= (Flags f);
};

Q_DECLARE_OPERATORS_FOR_FLAGS (IRCUser::Flags)

#endif // COBALT_IRC_USER_H
