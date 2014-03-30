#ifndef SPEECHBUBBLE_CONTEXT_H
#define SPEECHBUBBLE_CONTEXT_H

#include "main.h"
#include <QObject>
#include <QTreeWidget>

class Context;
class IRCConnection;
class IRCChannel;
class IRCUser;
class QTextDocument;

enum ContextType
{
	CTX_Channel,
	CTX_Query,
	CTX_Server,
};

// =============================================================================
// -----------------------------------------------------------------------------
class Context final : public QObject
{
	DELETE_COPY (Context)

public:
	union TargetUnion
	{
		IRCConnection* conn;
		IRCChannel* chan;
		IRCUser* user;
	};

	PROPERTY (public,  QTreeWidgetItem*,	treeItem,		setTreeItem,	STOCK_WRITE)
	PROPERTY (public,  QTextDocument*,		document,		setDocument,	STOCK_WRITE)
	PROPERTY (private, QList<Context*>,		subContexts,	setSubContexts,	STOCK_WRITE)
	PROPERTY (private, Context*,			parent,			setParent,		STOCK_WRITE)
	PROPERTY (private, TargetUnion,			target,			setTarget,		STOCK_WRITE)
	PROPERTY (private, ContextType,			type,			setType,		STOCK_WRITE)
	PROPERTY (private, int,					id,				setID,			STOCK_WRITE)
	PROPERTY (private, QString,				html,			setHTML,		STOCK_WRITE)

public:
	Context (IRCConnection* conn);
	Context (IRCChannel* channel);
	Context (IRCUser* user);
	~Context();

	void							addSubContext (Context* child);
	void							forgetSubContext (Context* child);
	IRCConnection*					connection();
	QString							name() const;
	void							print (QString text);
	void							updateTreeItem();
	void							writeIRCMessage (QString from, QString msg);

	static Context*					fromTreeWidgetItem (QTreeWidgetItem* item);
	static const QList<Context*>&	allContexts();
	static Context*					currentContext();
	static void						setCurrentContext (Context* context);

	static inline void printToCurrent (QString msg)
	{
		Context::currentContext()->print (msg);
	}

private:
	void commonInit();
	void printTimestamp();
	void rawPrint (QString msg, bool replaceEscapeCodes);
};

// ============================================================================
//
static inline IRCConnection* getCurrentConnection()
{
	return Context::currentContext()->connection();
}

#endif // SPEECHBUBBLE_CONTEXT_H
