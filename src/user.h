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

	PROPERTY (public,  QString,					Nickname)
	PROPERTY (public,  QString,					Username)
	PROPERTY (public,  QString,					Hostname)
	PROPERTY (public,  QString,					Realname)
	PROPERTY (public,  QString,					Server)
	PROPERTY (public,  Flags,						Flags)
	PROPERTY (public,  Context*,					Context)
	PROPERTY (private, IRCConnection*,			Connection)
	PROPERTY (private, QList<IRCChannel*>,	Channels) 

	public:
		IRCUser (IRCConnection* conn) :
			m_Flags (0),
			m_Connection (conn) {}

		void               	addKnownChannel (IRCChannel* chan);
		IRCChannel::EStatus	getStatusInChannel (IRCChannel* chan);
		void        			dropKnownChannel (IRCChannel* chan);
		QString					getStringRep() const;
		QString					getUserhost() const;
};

Q_DECLARE_OPERATORS_FOR_FLAGS (IRCUser::Flags)

#endif // COBALT_IRC_USER_H
