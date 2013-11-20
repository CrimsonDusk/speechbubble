#ifndef COBALT_IRC_CHANNEL_H
#define COBALT_IRC_CHANNEL_H

#include <QTime>
#include "main.h"

class Context;
class IRCConnection;
class IRCUser;

enum EChanMode
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

// =========================================================================
// -------------------------------------------------------------------------
struct ChannelModeInfo
{	const char c;
	const EChanMode mode;
	const QString name;
	const bool hasArg;
};

// =========================================================================
// -------------------------------------------------------------------------
struct ChannelMode
{	const ChannelModeInfo* info;
	QString arg;
};

class IRCChannel
{	public:
		// =========================================================================
		// -------------------------------------------------------------------------
		enum EStatus
		{	FNormal = (0),
			FVoiced = (1 << 0),
			FHalfOp = (1 << 1),
			FOp     = (1 << 2),
			FAdmin  = (1 << 3),
			FOwner  = (1 << 4),
		};

		Q_DECLARE_FLAGS (FStatusFlags, EStatus)

		// =========================================================================
		// -------------------------------------------------------------------------
		class Entry
		{	NEW_PROPERTY (public, IRCUser*, UserInfo)
			NEW_PROPERTY (public, IRCChannel::FStatusFlags, Status)

			public:
				Entry (IRCUser* user, EStatus stat) :
					m_UserInfo (user),
					m_Status (stat) {}

				bool operator== (const Entry& other) const;
		};

	// =========================================================================
	// -------------------------------------------------------------------------
	NEW_PROPERTY (public,  QString,					Name)
	NEW_PROPERTY (public,  QString,					Topic)
	NEW_PROPERTY (public,  QTime,						JoinTime)
	NEW_PROPERTY (public,  Context*,					Context)
	NEW_PROPERTY (private, IRCConnection*,			Connection)
	NEW_PROPERTY (private, QList<Entry>,			Userlist)
	NEW_PROPERTY (private, QList<ChannelMode>,	Modes)
	NEW_PROPERTY (private, QStringList,				Banlist)
	NEW_PROPERTY (private, QStringList,				Whitelist)
	NEW_PROPERTY (private, QStringList,				Invitelist)

	public:
		IRCChannel (IRCConnection* conn, QString name);
		~IRCChannel();

		Entry*					addUser (IRCUser* info);
		void						applyModeString (QString text);
		void						removeUser (IRCUser* info);
		Entry*					findUserByName (QString name);
		Entry*					findUser (IRCUser* info);
		QString					getModeString() const;
		FStatusFlags			getStatusOf (IRCUser* info);
		EStatus					getEffectiveStatusOf (IRCUser* info);
		ChannelMode*			getMode (EChanMode modenum);

		static EStatus			effectiveStatus (FStatusFlags mode);
		static FStatusFlags	getStatusFlag (char c);
		static QString			getStatusName (FStatusFlags mode);
};

Q_DECLARE_OPERATORS_FOR_FLAGS (IRCChannel::FStatusFlags)

#endif // COBALT_IRC_CHANNEL_H
