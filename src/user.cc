#include "user.h"
#include "channel.h"
#include "connection.h"

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
{	return fmt ("%1 (%2@%3)", getNickname(), getUsername(), getHostname());
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
}