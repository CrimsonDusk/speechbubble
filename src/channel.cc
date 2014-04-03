#include "channel.h"
#include "user.h"
#include "connection.h"
#include "context.h"

// ============================================================================
//
IRCChannel::IRCChannel (IRCConnection* conn, const QString& newname) :
	name (newname),
	joinTime (QTime::currentTime()),
	connection (conn),
	isDoneWithNames (true)
{
	context = new Context (this);
	connection->addChannel (this);
	connection->write (format ("WHO %1\n", name));
}

// ============================================================================
//
IRCChannel::~IRCChannel()
{
	delete context;

	for (const UserlistEntry& e : userlist)
	{
		IRCUser* user = e.userInfo;
		user->dropKnownChannel (this);
	}
}

// ============================================================================
//
UserlistEntry* IRCChannel::addUser (IRCUser* info)
{
	UserlistEntry e (info, FNormal);
	info->addKnownChannel (this);
	userlist << e;
	emit userlistChanged();
	return &userlist.last();
}

// ============================================================================
//
void IRCChannel::removeUser (IRCUser* info)
{
	info->dropKnownChannel (this);

	for (const UserlistEntry& e : userlist)
	{
		if (e.userInfo == info)
		{
			userlist.removeOne (e);
			emit userlistChanged();
			return;
		}
	}
}

// ============================================================================
//
UserlistEntry* IRCChannel::findUserByName (QString name)
{
	for (UserlistEntry & e : userlist)
	{
		if (e.userInfo->nickname == name)
			return &e;
	}

	return null;
}

// ============================================================================
//
UserlistEntry* IRCChannel::findUser (IRCUser* info)
{
	for (UserlistEntry & e : userlist)
	{
		if (e.userInfo == info)
			return &e;
	}

	return null;
}

// ============================================================================
//
bool UserlistEntry::operator== (const UserlistEntry& other) const
{
	return (userInfo == other.userInfo) &&
		   (status == other.status);
}

// ============================================================================
//
FStatusFlags IRCChannel::getStatusOf (IRCUser* info)
{
	UserlistEntry* e = findUser (info);

	if (!e)
		return FNormal;

	return e->status;
}

// ============================================================================
//
EStatus IRCChannel::getEffectiveStatusOf (IRCUser* info)
{
	UserlistEntry* e = findUser (info);

	if (!e)
		return FNormal;

	FStatusFlags mode = e->status;
	return effectiveStatus (mode);
}

// ============================================================================
//
EStatus IRCChannel::effectiveStatus (FStatusFlags mode)
{
	if (mode & FOwner)  return FOwner;
	if (mode & FAdmin)  return FAdmin;
	if (mode & FOp)     return FOp;
	if (mode & FHalfOp) return FHalfOp;
	if (mode & FVoiced) return FVoiced;

	return FNormal;
}

// ============================================================================
//
// Since the effective status returns a flag instead of a normal enumerator,
// this cannot be a lookup table.
//
QString IRCChannel::getStatusName (FStatusFlags mode)
{
	switch (effectiveStatus (mode))
	{
		case FOwner:	return tr ("Owner");
		case FAdmin:	return tr ("Admin");
		case FOp:		return tr ("Operator");
		case FHalfOp:	return tr ("Half-operator");
		case FVoiced:	return tr ("Voiced");
		case FNormal:	return tr ("User");
	}

	return "User";
}

// ============================================================================
//
FStatusFlags IRCChannel::getStatusFlag (char c)
{
	switch (c)
	{
		case 'q':	return FOwner;
		case 'a':	return FAdmin;
		case 'o':	return FOp;
		case 'h':	return FHalfOp;
		case 'v':	return FVoiced;
	}

	return 0;
}

// ============================================================================
//
void IRCChannel::applyModeString (QString text)
{
	if (text.isEmpty())
		return;

	bool	neg = false;
	QString	modestring = text.split (" ", QString::SkipEmptyParts) [0];
	bool	needNames = false;

	for (char c : modestring.toUtf8())
	{
		if (c == '+')
		{
			// +abc
			neg = false;
			continue;
		}

		if (c == '-')
		{
			// -abc
			neg = true;
			continue;
		}

		// Handle status modes. Who on earth thought it was a good
		// idea to have them as channel modes?
		// Since we cannot really parse the mode string fully, we
		// must ask the server for a new NAMES list.
		if (c == 'o' || c == 'v' || c == 'h' || c == 'a' || c == 'q')
		{
			needNames = true;
			continue;
		}

		if (neg == false)
			modes << c;
		else
			modes.removeOne (c);
	}

	if (needNames)
		connection->write (format("NAMES :%1\n", name));
}

// ============================================================================
//
QString IRCChannel::getModeString() const
{
	QString modestring;
	QStringList args;

	for (char mode : modes)
		modestring += mode;

	args.push_front (modestring);
	return args.join (" ");
}

// ============================================================================
//
void IRCChannel::addNames (const QStringList& names)
{
	static const struct
	{
		char	symbol;
		EStatus	flag;
	} statusflags[] =
	{
		{ '~',	FOwner	},
		{ '&',	FAdmin	},
		{ '@',	FOp		},
		{ '%',	FHalfOp	},
		{ '+',	FVoiced	},
	};

	for (QString nick : names)
	{
		FStatusFlags	flags = 0;
		IRCUser*		user = connection->findUser (nick, true);
		bool			repeat;

		do
		{
			repeat = false;

			for (const auto & flaginfo : statusflags)
			{
				if (nick.startsWith (flaginfo.symbol))
				{
					nick.remove (0, 1);
					flags |= flaginfo.flag;
					repeat = true;
					break;
				}
			}
		}
		while (repeat == true);

		newNames << UserlistEntry (user, flags);
	}
}

// ============================================================================
//
void IRCChannel::namesDone()
{
	QList<IRCUser*> oldusers;

	// This gets a bit tricky, I don't want any users to be deleted just because
	// this is their only known channel and they would be auto-pruned in the
	// process. So we flag them so that they won't be pruned and perform the
	// pruning manually once we're done.
	for (UserlistEntry & e : userlist)
	{
		e.userInfo->flags |= IRCUser::FDoNotDelete;
		oldusers << e.userInfo;
		e.userInfo->dropKnownChannel (this);
	}

	userlist = newNames;

	// Now check which users in oldusers are still in the userlist.
	for (auto it = oldusers.begin(); it != oldusers.end(); ++it)
	{
		IRCUser* user = *it;

		for (UserlistEntry& e : userlist)
		{
			if (e.userInfo == user)
			{
				it = oldusers.erase (it) - 1;
				break;
			}
		}
	}

	// Now remove the do not delete flag and perform pruning.
	for (IRCUser * user : oldusers)
	{
		user->flags &= ~IRCUser::FDoNotDelete;
		user->checkForPruning();
	}

	newNames.clear();
	emit userlistChanged();
}
