#ifndef COIRC_CONNECTION_H
#define COIRC_CONNECTION_H

#include "main.h"
#include <QObject>
#include <QAbstractSocket>

class IRCUser;
class IRCChannel;
class Context;
class QTcpSocket;
class QTimer;

// =====================================================================
//
// State of connection
//
enum EConnectionState
{
	CNS_Disconnected,
	EConnecting,
	ERegistering,
	EConnected,
};

// =====================================================================
//
// IRC server reply codes.. not a comprehensive list
//
enum EReplyCode
{
	Reply_Welcome				= 1,
	Reply_YourHost				= 2,
	Reply_Created				= 3,
	Reply_MyInfo				= 4,
	Reply_Supported				= 5,
	Reply_UniqueId				= 42,
	Reply_Away					= 301,
	Reply_UserHost				= 302,
	Reply_IsOn					= 303,
	Reply_UnAway				= 305,
	Reply_NowAway				= 306,
	Reply_WhoisUser				= 311,
	Reply_WhoisServer			= 312,
	Reply_WhoisOperator			= 313,
	Reply_WhowasUser			= 314,
	Reply_EndOfWho				= 315,
	Reply_WhoisIdle				= 317,
	Reply_EndOfWhois			= 318,
	Reply_WhoisChannels			= 319,
	Reply_EndOfWhowas			= 369,
	Reply_ListStart				= 321,
	Reply_List					= 322,
	Reply_ListEnd				= 323,
	Reply_ChannelModeIs			= 324,
	Reply_UniqOpIs				= 325,
	Reply_WhoisAccount			= 330,
	Reply_NoTopic				= 331,
	Reply_Topic					= 332,
	Reply_TopicSetAt			= 333,
	Reply_Inviting				= 341,
	Reply_Summoning				= 342,
	Reply_InviteList			= 346,
	Reply_EndOfInviteList		= 347,
	Reply_ExceptList			= 348,
	Reply_EndOfExceptList		= 349,
	Reply_Version				= 351,
	Reply_WhoReply				= 352,
	Reply_NameReply				= 353,
	Reply_Links					= 364,
	Reply_EndOfLinks			= 365,
	Reply_EndOfNames			= 366,
	Reply_BanList				= 367,
	Reply_EndOfBanList			= 368,
	Reply_Info					= 371,
	Reply_Motd					= 372,
	Reply_EndOfInfo				= 374,
	Reply_MotdStart				= 375,
	Reply_EndOfMotd				= 376,
	Reply_YoureOper				= 381,
	Reply_Rehashing				= 382,
	Reply_YoureService			= 383,
	Reply_Time					= 391,
	Reply_ErroneusNickname		= 432,
	Reply_NicknameInUse			= 433,
	Reply_NeedMoreParams		= 461,
};

struct PrefixInfo
{
	char	modesym;
	char	prefix;
};

class IRCConnection : public QObject
{
public:
	Q_OBJECT
	DELETE_COPY (IRCConnection)
	PROPERTY (QString nickname)
	PROPERTY (QString username)
	PROPERTY (QString realname)
	PROPERTY (Context* context)
	PROPERTY (QString hostname)
	PROPERTY (quint16 port)
	PROPERTY (EConnectionState state)
	PROPERTY (QString lineWork)
	PROPERTY (QList<IRCChannel*> channels)
	PROPERTY (IRCUser* ourselves)

	PROPERTY (QTcpSocket* socket)
	PROPERTY (QTimer* timer)
	PROPERTY (QList<IRCUser*> users)

	CLASSDATA (IRCConnection)

public:
	explicit IRCConnection (QString host, quint16 port, QObject* parent = 0);
	virtual ~IRCConnection();

	void			addChannel (IRCChannel* a);
	void			connectToServer();
	void			disconnectFromServer (QString quitmessage = "");
	IRCChannel*		findChannel (QString name, bool createIfNeeded = false);
	IRCUser*		findUser (QString nickname, bool createIfNeeded = false);
	void			forgetUser (IRCUser* user);
	void			removeChannel (IRCChannel* a);
	void			write (QString text);

	static const QList<IRCConnection*>& getAllConnections();

public slots:
	void readyRead();
	void writeLogin();

	void processMessage (QString msg);
	void print (QString msg);
	void warning (QString msg);
	void parseNumeric (QString msg, QStringList tokens, int num);
	void processJoin (QString msg, QStringList tokens);
	void processPart (QString msg, QStringList tokens);
	void processQuit (QString msg, QStringList tokens);
	void processPrivmsg (QString msg, QStringList tokens);
	void processTopicChange (QString msg, QStringList tokens);

private slots:
	void tick();
	void processConnectionError (QAbstractSocket::SocketError err);
	void processMode (QString msg, QStringList tokens);
};

#endif // COIRC_CONNECTION_H
