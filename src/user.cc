#include "user.h"
#include "channel.h"
#include "connection.h"

// =============================================================================
//
IRCUser::~IRCUser()
{
	flags |= FDoNotDelete;
	connection->forgetUser (this);

	for (IRCChannel* chan : channels)
		chan->removeUser (this);
}

// =============================================================================
//
// Determine status level of this user.
//
EStatus IRCUser::getStatusInChannel (IRCChannel* chan)
{
	return chan->getEffectiveStatusOf (this);
}

// =============================================================================
//
QString IRCUser::getUserHost() const
{
	return format ("%1!%2@%3", nickname, username, hostname);
}

// =============================================================================
//
QString IRCUser::describe() const
{
	return format ("%1 (%2)", getUserHost(), realname);
}

// =============================================================================
//
void IRCUser::addKnownChannel (IRCChannel* chan)
{
	channels << chan;
}

// =============================================================================
//
void IRCUser::dropKnownChannel (IRCChannel* chan)
{
	channels.removeOne (chan);
	checkForPruning();
}

// =============================================================================
//
// Prune this user if there's no reason to remember them anymore.
//
void IRCUser::checkForPruning()
{
	if ((this != connection->ourselves) &&
		(channels.isEmpty()) &&
		(context == null) &&
		(flags & FDoNotDelete) == 0)
	{
		delete this;
	}
}
