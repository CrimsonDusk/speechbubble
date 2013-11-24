#ifndef COBALTIRC_CONTEXT_H
#define COBALTIRC_CONTEXT_H

#include "main.h"
#include <QObject>
#include <QTreeWidget>

class Context;
class IRCConnection;
class IRCChannel;
class IRCUser;
class QTextDocument;

#define DEFINE_CONTEXT(NAME) \
	public: \
		virtual Type type() const { return E##NAME##Context; } \
		virtual QString name() const override;

// =============================================================================
// -----------------------------------------------------------------------------
class Context final : public QObject
{	DELETE_COPY (Context)

	public:
		enum ContextType
		{	EChannelContext,
			EQueryContext,
			EServerContext,
		};

		union TargetUnion
		{	IRCConnection* conn;
			IRCChannel* chan;
			IRCUser* user;
		};

	PROPERTY (public,  QTreeWidgetItem*,	TreeItem)
	PROPERTY (public,  QTextDocument*,		Document)
	PROPERTY (private, QList<Context*>,		Subcontexts)
	PROPERTY (private, Context*,				Parent)
	PROPERTY (private, TargetUnion,			Target)
	PROPERTY (private, ContextType,			Type)
	PROPERTY (private, int,						ID)
	PROPERTY (private, QString,				HTML)

	public:
		Context (IRCConnection* conn);
		Context (IRCChannel* channel);
		Context (IRCUser* user);
		~Context();

		void				addSubContext (Context* child);
		void				forgetSubContext (Context* child);
		void				updateTreeItem();
		IRCConnection*	getConnection();
		QString			getName() const;
		void				print (QString text, bool replaceEscapeCodes);
		void				writeIRCMessage (QString msg);

		static const QList<Context*>& getAllContexts();
		static Context* getFromTreeWidgetItem (QTreeWidgetItem* item);
		static Context* getCurrentContext();
		static void setCurrentContext (Context* context);

		static inline void printToCurrent (QString msg)
		{	Context::getCurrentContext()->print (msg, true);
		}

	private:
		void commonInit();
};

#endif // COBALTIRC_CONTEXT_H
