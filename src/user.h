#ifndef COBALTIRC_USER_H
#define COBALTIRC_USER_H

#include "main.h"
#include "channel.h"

class Context;
class Context;
class IRCConnection;

class IRCUser
{
public:
	enum Flag
	{
		FAway     		= (1 << 0),		// is /AWAY
		FIRCOp    		= (1 << 1),		// is an IRC Op
		FDoNotDelete	= (1 << 2),		// don't delete if last known channel goes
	};

	Q_DECLARE_FLAGS (Flags, Flag)

	// =========================================================================
	//
	PROPERTY (public,	QString,			nickname,	setNickname,	STOCK_WRITE)
	PROPERTY (public, 	QString,			username,	setUsername,	STOCK_WRITE)
	PROPERTY (public,	QString,			hostname,	setHostname,	STOCK_WRITE)
	PROPERTY (public,	QString,			realname,	setRealname,	STOCK_WRITE)
	PROPERTY (public,	Flags,				flags,		setFlags,		STOCK_WRITE)
	PROPERTY (private,	IRCConnection*,		connection,	setConnection,	STOCK_WRITE)
	PROPERTY (private,	QList<IRCChannel*>,	channels,	setChannels,	STOCK_WRITE)
	PROPERTY (public,	Context*,			context,	setContext,		STOCK_WRITE)

public:
	IRCUser (IRCConnection* conn) :
		m_flags (0),
		m_connection (conn) {}

	~IRCUser();

	void        addKnownChannel (IRCChannel* chan);
	void		checkForPruning();
	EStatus		getStatusInChannel (IRCChannel* chan);
	void		dropKnownChannel (IRCChannel* chan);
	QString		describe() const;
	QString		userHost() const;
};

Q_DECLARE_OPERATORS_FOR_FLAGS (IRCUser::Flags)

#endif // COBALT_IRC_USER_H
