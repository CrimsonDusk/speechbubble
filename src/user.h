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

	NEW_PROPERTY (public,  QString,					Nickname)
	NEW_PROPERTY (public,  QString,					Username)
	NEW_PROPERTY (public,  QString,					Hostname)
	NEW_PROPERTY (public,  QString,					Realname)
	NEW_PROPERTY (public,  QString,					Server)
	NEW_PROPERTY (public,  Flags,						Flags)
	NEW_PROPERTY (public,  Context*,					Context)
	NEW_PROPERTY (private, IRCConnection*,			Connection)
	NEW_PROPERTY (private, QList<IRCChannel*>,	Channels) 

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
