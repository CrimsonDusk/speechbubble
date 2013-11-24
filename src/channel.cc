#include "channel.h"
#include "user.h"
#include "connection.h"
#include "context.h"
#include "moc_channel.cpp"

IRCChannel::IRCChannel (IRCConnection* conn, QString name) :
	m_Name (name),
	m_JoinTime (QTime::currentTime()),
	m_Connection (conn)
{	setContext (new Context (this));
	m_Connection->addChannel (this);
}

IRCChannel::~IRCChannel()
{	delete getContext();

	for (const IRCChannel::Entry& e : getUserlist())
	{	IRCUser* user = e.getUserInfo();
		user->dropKnownChannel (this);
	}
}

IRCChannel::Entry* IRCChannel::addUser (IRCUser* info)
{	Entry e (info, FNormal);
	info->addKnownChannel (this);
	m_Userlist << e;
	emit userlistChanged();
	return &(m_Userlist.last());
}

void IRCChannel::removeUser (IRCUser* info)
{	info->dropKnownChannel (this);

	for (const Entry& e : getUserlist())
	{	if (e.getUserInfo() == info)
		{	m_Userlist.removeOne (e);
			emit userlistChanged();
			return;
		}
	}
}

IRCChannel::Entry* IRCChannel::findUserByName (QString name)
{	for (Entry& e : m_Userlist)
	{	if (e.getUserInfo()->getNickname() == name)
			return &e;
	}

	return null;
}

IRCChannel::Entry* IRCChannel::findUser (IRCUser* info)
{	for (Entry& e : m_Userlist)
	{	if (e.getUserInfo() == info)
			return &e;
	}

	return null;
}

bool IRCChannel::Entry::operator== (const IRCChannel::Entry& other) const
{	return (getUserInfo() == other.getUserInfo()) && (getStatus() == other.getStatus());
}

IRCChannel::FStatusFlags IRCChannel::getStatusOf (IRCUser* info)
{	Entry* e = findUser (info);

	if (!e)
		return FNormal;

	return e->getStatus();
}

IRCChannel::EStatus IRCChannel::getEffectiveStatusOf (IRCUser* info)
{	Entry* e = findUser (info);

	if (!e)
		return FNormal;

	FStatusFlags mode = e->getStatus();
	return effectiveStatus (mode);
}

IRCChannel::EStatus IRCChannel::effectiveStatus (IRCChannel::FStatusFlags mode)
{	if (mode & FOwner)  return FOwner;
	if (mode & FAdmin)  return FAdmin;
	if (mode & FOp)     return FOp;
	if (mode & FHalfOp) return FHalfOp;
	if (mode & FVoiced) return FVoiced;

	return FNormal;
}

QString IRCChannel::getStatusName (IRCChannel::FStatusFlags mode)
{	switch (effectiveStatus (mode))
	{	case FOwner:
			return "Owner";

		case FAdmin:
			return "Admin";

		case FOp:
			return "Operator";

		case FHalfOp:
			return "Half-operator";

		case FVoiced:
			return "Voiced";

		case FNormal:
			return "User";
	}

	return "User";
}

const ChannelModeInfo g_ChannelModeInfo[] =
{
#define CHANMODE(C, N, HASARG) \
	{ C, EChanMode##N, #N, HASARG },
	CHANMODE ('A', AllInvite,      false)
	CHANMODE ('b', Ban,            true)
	CHANMODE ('B', BlockCaps,      false)
	CHANMODE ('c', BlockColors,    false)
	CHANMODE ('C', BlockCTCP,      false)
	CHANMODE ('D', DelayedJoin,    false)
	CHANMODE ('d', DelayedMessage, true)
	CHANMODE ('e', BanExempt,      true)
	CHANMODE ('f', FloodKick,      true)
	CHANMODE ('F', NickFlood,      false)
	CHANMODE ('g', WordCensor,     true)
	CHANMODE ('G', NetworkCensor,  false)
	CHANMODE ('h', ChanHalfOp,     true)
	CHANMODE ('H', History,        true)
	CHANMODE ('i', InviteOnly,     false)
	CHANMODE ('I', InviteExcept,   true)
	CHANMODE ('j', JoinFlood,      true)
	CHANMODE ('J', KickNoRejoin,   true)
	CHANMODE ('k', Locked,         true)
	CHANMODE ('K', NoKnock,        false)
	CHANMODE ('l', UserLimit,      true)
	CHANMODE ('L', Redirect,       true)
	CHANMODE ('m', Moderated,      false)
	CHANMODE ('M', RegisterSpeak,  false)
	CHANMODE ('n', NoExtMessages,  false)
	CHANMODE ('N', NoNickChanges,  false)
	CHANMODE ('o', ChanOp,         true)
	CHANMODE ('O', OpersOnly,      false)
	CHANMODE ('p', Private,        false)
	CHANMODE ('P', Permanent,      false)
	CHANMODE ('q', ChanOwner,      true)
	CHANMODE ('Q', NoKick,         false)
	CHANMODE ('R', RegisterJoin,   false)
	CHANMODE ('s', Secret,         false)
	CHANMODE ('S', StripColors,    false)
	CHANMODE ('t', TopicLock,      false)
	CHANMODE ('T', BlockNotice,    false)
	CHANMODE ('u', Auditorium,     false)
	CHANMODE ('v', UserVoice,      false)
	CHANMODE ('w', AutoOp,         true)
	CHANMODE ('y', IRCOpInChannel, false)
	CHANMODE ('Y', IRCOpOJoin,     false)
	CHANMODE ('X', GenericRestrict, true)
	CHANMODE ('z', Secure,         false)
	CHANMODE ('Z', NamedMode,      true)
};

IRCChannel::FStatusFlags IRCChannel::getStatusFlag (char c)
{	switch (c)
	{	case 'q':
			return FOwner;

		case 'a':
			return FAdmin;

		case 'o':
			return FOp;

		case 'h':
			return FHalfOp;

		case 'v':
			return FVoiced;
	}

	return 0;
}

void IRCChannel::applyModeString (QString text)
{	bool neg = false;
	QStringList args = text.split (" ", QString::SkipEmptyParts);
	uint argidx = 0;
	QString modestring = args[0];
	args.erase (0);

	for (char c : modestring.toUtf8())
	{	if (c == '+')
		{	// +abc
			neg = false;
			continue;
		} elif (c == '-')
		{	// -abc
			neg = true;
			continue;
		}

		// Handle status modes. Who on earth thought it was a good
		// idea to have them as channel modes?
		if (c == 'o' || c == 'v' || c == 'h' || c == 'a' || c == 'q')
		{	if (args.isEmpty())
				continue;

			// All of these require an argument
			QString arg = args.last();
			args.removeLast();

			Entry* e = findUserByName (arg);

			if (!e)
				continue;

			FStatusFlags status = e->getStatus();
			FStatusFlags flag = getStatusFlag (c);

			if (!neg)
				status |= flag;
			else
				status &= ~flag;

			e->setStatus (status);
			emit userlistChanged();
			continue;
		}

		for (const ChannelModeInfo& it : g_ChannelModeInfo)
		{	if (it.c != c)
				continue;

			if (neg == false)
			{	// New mode
				ChannelMode mode;
				mode.info = &it;

				if (it.hasArg)
				{	mode.arg = args.last();
					args.removeLast();
				}

				m_Modes << mode;
			}
			else
			{	// Remove existing mode
				for (int j = 0; j < getModes().size(); ++j)
				{	ChannelMode mode = getModes()[j];

					if (mode.info != &it)
						continue;

					m_Modes.removeAt (j);

					if (!mode.arg.isEmpty())
						argidx++;
				}
			}

			break;
		}
	}
}

QString IRCChannel::getModeString() const
{	QString modestring;
	QStringList args;

	for (const ChannelMode& mode : m_Modes)
	{	modestring += mode.info->c;

		if (mode.info->hasArg)
			args << mode.arg;
	}

	args.push_front (modestring);
	return args.join (" ");
}

ChannelMode* IRCChannel::getMode (EChanMode modenum)
{	for (ChannelMode& mode : m_Modes)
	{	if (mode.info->mode != modenum)
			continue;

		return &mode;
	}

	return null;
}
