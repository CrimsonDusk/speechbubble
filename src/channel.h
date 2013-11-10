#ifndef COBALT_IRC_CHANNEL_H
#define COBALT_IRC_CHANNEL_H

#include <QTime>
#include "main.h"

class Context;
class IRCConnection;
class IRCUser;
enum ChanMode
{	EChanModeAllInvite,
	EChanModeBan,
	EChanModeBlockCaps,
	EChanModeBlockColors,
	EChanModeBlockCTCP,
	EChanModeDelayedJoin,
	EChanModeBanExempt,
	EChanModeFloodKick,
	EChanModeNickFlood,
	EChanModeWordCensor,
	EChanModeNetworkCensor,
	EChanModeInviteOnly,
	EChanModeKickNoRejoin,
	EChanModeLocked,
	EChanModeNoKnock,
	EChanModeUserLimit,
	EChanModeRedirect,
	EChanModeModerated,
	EChanModeRegisterSpeak,
	EChanModeNoExtMessages,
	EChanModeNoNickChanges,
	EChanModeChanOp,
	EChanModeOpersOnly,
	EChanModePrivate,
	EChanModePermanent,
	EChanModeChanOwner,
	EChanModeNoKick,
	EChanModeRegisterJoin,
	EChanModeSecret,
	EChanModeStripColors,
	EChanModeTopicLock,
	EChanModeBlockNotice,
	EChanModeAuditorium,
	EChanModeUserVoice,
	EChanModeIRCOpInChannel,
	EChanModeSecure,
	EChanModeChanHalfOp,
	EChanModeHistory,
	EChanModeJoinFlood,
	EChanModeInviteExcept,
	EChanModeDelayedMessage,
	EChanModeAutoOp,
	EChanModeNamedMode,
	EChanModeIRCOpOJoin,
	EChanModeGenericRestrict,
};

struct ChannelModeInfo
{	const char c;
	const ChanMode mode;
	const str name;
	const bool hasArg;
};

// =========================================================================
// -------------------------------------------------------------------------
struct IRCChannelMode
{	const ChannelModeInfo* info;
	QString arg;
};

class IRCChannel
{	public:
		// =========================================================================
		// -------------------------------------------------------------------------
		enum Status
		{	FNormal = (0),
			FVoiced = (1 << 0),
			FHalfOp = (1 << 1),
			FOp     = (1 << 2),
			FAdmin  = (1 << 3),
			FOwner  = (1 << 4),
		};

		Q_DECLARE_FLAGS (StatusFlags, Status)

		// =========================================================================
		// -------------------------------------------------------------------------
		class Entry
		{	PROPERTY (public, IRCUser*, userinfo)
			PROPERTY (public, long, status)

			public:
				Entry (IRCUser* user, Status stat) :
					m_userinfo (user),
					m_status (stat) {}

				bool operator== (const Entry& other) const;
		};

	// =========================================================================
	// -------------------------------------------------------------------------
	PROPERTY (public,  QString,               name)
	PROPERTY (public,  QString,               topic)
	PROPERTY (public,  QTime,                 joinTime)
	PROPERTY (public,  Context*,           context)
	PROPERTY (private, IRCConnection*,        connection)
	PROPERTY (private, QList<Entry>,          userlist)
	PROPERTY (private, QList<IRCChannelMode>, modes)
	PROPERTY (private, QStringList,           banlist)
	PROPERTY (private, QStringList,           whitelist)
	PROPERTY (private, QStringList,           invitelist)

	public:
		typedef ChannelModeInfo modeinfo;

		IRCChannel (IRCConnection* conn, QString name);
		~IRCChannel();

		Entry*             addUser (IRCUser* info);
		void               apply_mode_string (QString text);
		void               delUser (IRCUser* info);
		Entry*             findUser (QString name);
		Entry*             findUser (IRCUser* info);
		QString            mode_string() const;
		int                num_users() const;
		long               statusof (IRCUser* info);
		Status             effective_status_of (IRCUser* info);
		IRCChannelMode*    get_mode (ChanMode modenum);

		static Status      effective_status (long mode);
		static StatusFlags get_status_flag (char c);
		static QString     status_name (long mode);
};

Q_DECLARE_OPERATORS_FOR_FLAGS (IRCChannel::StatusFlags)

#endif // COBALT_IRC_CHANNEL_H
