#include "channel.h"
#include "user.h"
#include "connection.h"
#include "context.h"
#include <qstringlist.h>

IRCChannel::IRCChannel (IRCConnection* conn, QString name) :
	m_name (name),
	m_joinTime (QTime::currentTime()),
	m_connection (conn)
{	set_context (new Context (this));
}

IRCChannel::~IRCChannel()
{	delete context();
}

IRCChannel::Entry* IRCChannel::AddUser (IRCUser* info)
{	Entry e (info, FNormal);
	info->add_known_channel (this);
	m_userlist << e;
	return &m_userlist.last();
}

void IRCChannel::RemoveUser (IRCUser* info)
{	info->del_known_channel (this);

	for (Entry& e : m_userlist)
	{	if (e.userinfo() == info)
		{	m_userlist.removeOne (e);
			return;
		}
	}
}

IRCChannel::Entry* IRCChannel::FindUser (QString name)
{	for (Entry& e : m_userlist)
	{	if (e.userinfo()->nick() == name)
			return &e;
	}

	return null;
}

IRCChannel::Entry* IRCChannel::FindUser (IRCUser* info)
{	for (Entry& e : m_userlist)
	{	if (e.userinfo() == info)
			return &e;
	}

	return null;
}

bool IRCChannel::Entry::operator== (const IRCChannel::Entry& other) const
{	return (userinfo() == other.userinfo()) && (status() == other.status());
}

long IRCChannel::StatusOf (IRCUser* info)
{	Entry* e = FindUser (info);

	if (!e)
		return FNormal;

	return e->status();
}

IRCChannel::Status IRCChannel::EffectiveStatusOf (IRCUser* info)
{	Entry* e = FindUser (info);

	if (!e)
		return FNormal;

	long mode = e->status();
	return EffectiveStatus (mode);
}

IRCChannel::Status IRCChannel::EffectiveStatus (long mode)
{	if (mode & FOwner)  return FOwner;
	if (mode & FAdmin)  return FAdmin;
	if (mode & FOp)     return FOp;
	if (mode & FHalfOp) return FHalfOp;
	if (mode & FVoiced) return FVoiced;

	return FNormal;
}

QString IRCChannel::StatusName (long int mode)
{	switch (EffectiveStatus (mode))
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

int IRCChannel::num_users() const
{	return m_userlist.size();
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

IRCChannel::StatusFlags IRCChannel::get_status_flag (char c)
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

void IRCChannel::apply_mode_string (QString text)
{	bool neg = false;
	QStringList args = text.split (" ", QString::SkipEmptyParts);
	uint argidx = 0;
	str modestring = args[0];
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
			str arg = args.last();
			args.removeLast();

			Entry* e = FindUser (arg);

			if (!e)
				continue;

			long status = e->status();
			long flag = get_status_flag (c);

			if (!neg)
				status |= flag;
			else
				status &= ~flag;

			e->set_status (status);
			continue;
		}

		for (const ChannelModeInfo& it : g_ChannelModeInfo)
		{	if (it.c != c)
				continue;

			if (neg == false)
			{	// New mode
				IRCChannelMode mode;
				mode.info = &it;

				if (it.hasArg)
				{	mode.arg = args.last();
					args.removeLast();
				}

				m_modes << mode;
			}
			else
			{	// Remove existing mode
				for (int j = 0; j < m_modes.size(); ++j)
				{	IRCChannelMode mode = m_modes[j];

					if (mode.info != &it)
						continue;

					m_modes.removeAt (j);

					if (!mode.arg.isEmpty())
						argidx++;
				}
			}

			break;
		}
	}
}

str IRCChannel::mode_string() const
{	str modestring;
	QStringList args;

	for (const IRCChannelMode& mode : m_modes)
	{	modestring += mode.info->c;

		if (mode.info->hasArg)
			args << mode.arg;
	}

	args.push_front (modestring);
	return args.join (" ");
}

IRCChannelMode* IRCChannel::get_mode (ChanMode modenum)
{	for (IRCChannelMode& mode : m_modes)
	{	if (mode.info->mode != modenum)
			continue;

		return &mode;
	}

	return null;
}
