#ifndef SPEECHBUBBLE_CHANNEL_H
#define SPEECHBUBBLE_CHANNEL_H

#include <QTime>
#include "main.h"

class Context;
class IRCConnection;
class IRCUser;

// =============================================================================
//
enum EStatus
{
	FNormal = (0),
	FVoiced = (1 << 0),
	FHalfOp = (1 << 1),
	FOp     = (1 << 2),
	FAdmin  = (1 << 3),
	FOwner  = (1 << 4),
};

Q_DECLARE_FLAGS (FStatusFlags, EStatus)
Q_DECLARE_OPERATORS_FOR_FLAGS (FStatusFlags)

// =============================================================================
//
class UserlistEntry
{
	PROPERTY (IRCUser* userInfo)
	PROPERTY (FStatusFlags status)
	CLASSDATA (UserlistEntry)

public:
	UserlistEntry (IRCUser* user, FStatusFlags stat) :
		userInfo (user),
		status (stat) {}

	bool operator== (const UserlistEntry& other) const;
};

// =========================================================================
//
class IRCChannel : public QObject
{
	Q_OBJECT
	PROPERTY (QString name)
	PROPERTY (QString topic)
	PROPERTY (QTime joinTime)
	PROPERTY (Context* context)
	PROPERTY (IRCConnection* connection)
	PROPERTY (QList<UserlistEntry> userlist)
	PROPERTY (QList<char> modes)
	PROPERTY (QList<UserlistEntry> newNames)
	PROPERTY (bool isDoneWithNames);
	CLASSDATA (IRCChannel)

public:
	IRCChannel (IRCConnection* conn, const QString& newname);
	~IRCChannel();

	UserlistEntry*			addUser (IRCUser* info);
	void					addNames (const QStringList& names);
	void					applyModeString (QString text);
	UserlistEntry*			findUserByName (QString name);
	UserlistEntry*			findUser (IRCUser* info);
	QString					getModeString() const;
	FStatusFlags			getStatusOf (IRCUser* info);
	EStatus					getEffectiveStatusOf (IRCUser* info);
	void					namesDone();
	void					removeUser (IRCUser* info);

	static EStatus			effectiveStatus (FStatusFlags mode);
	static FStatusFlags		getStatusFlag (char c);
	static QString			getStatusName (FStatusFlags mode);

signals:
	void userlistChanged();
};

#endif // SPEECHBUBBLE_CHANNEL_H
