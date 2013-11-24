#include "user.h"
#include "channel.h"
#include "connection.h"

// =============================================================================
// -----------------------------------------------------------------------------
IRCUser::~IRCUser()
{	getConnection()->forgetUser (this);
}

// =============================================================================
// Determine status level of this user.
// -----------------------------------------------------------------------------
IRCChannel::EStatus IRCUser::getStatusInChannel (IRCChannel* chan)
{	return chan->getEffectiveStatusOf (this);
}

// =============================================================================
// -----------------------------------------------------------------------------
QString IRCUser::getUserhost() const
{	return fmt ("%1!%2@%3", getNickname(), getUsername(), getHostname());
}

// =============================================================================
// -----------------------------------------------------------------------------
QString IRCUser::getStringRep() const
{	return fmt ("%1 (%2)", getUserhost(), getRealname());
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCUser::addKnownChannel (IRCChannel* chan)
{	m_Channels << chan;
}

// =============================================================================
// -----------------------------------------------------------------------------
void IRCUser::dropKnownChannel (IRCChannel* chan)
{	m_Channels.removeOne (chan);

	// If this user left the last channel we are also in, forget this user now. We
	// won't know when he will disconnect. Obviously don't remove ourselves!
	if (this != getConnection()->getOurselves() && m_Channels.isEmpty())
		delete this;
}