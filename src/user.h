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
	PROPERTY (QString nickname)
	PROPERTY (QString username)
	PROPERTY (QString hostname)
	PROPERTY (QString realname)
	PROPERTY (Flags flags)
	PROPERTY (IRCConnection* connection)
	PROPERTY (QList<IRCChannel*> channels)
	PROPERTY (Context* context)
	CLASSDATA (IRCUser)

public:
	IRCUser (IRCConnection* conn) :
		flags (0),
		connection (conn) {}

	~IRCUser();

	void        addKnownChannel (IRCChannel* chan);
	void		checkForPruning();
	EStatus		getStatusInChannel (IRCChannel* chan);
	void		dropKnownChannel (IRCChannel* chan);
	QString		describe() const;
	QString		getUserHost() const;
};

Q_DECLARE_OPERATORS_FOR_FLAGS (IRCUser::Flags)

#endif // COBALT_IRC_USER_H
