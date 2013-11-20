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

	NEW_PROPERTY (public,  QTreeWidgetItem*,	TreeItem)
	NEW_PROPERTY (public,  QTextDocument*,		Document)
	NEW_PROPERTY (private, QList<Context*>,	Subcontexts)
	NEW_PROPERTY (private, Context*,				Parent)
	NEW_PROPERTY (private, TargetUnion,			Target)
	NEW_PROPERTY (private, ContextType,			Type)
	NEW_PROPERTY (private, int,					ID)
	NEW_PROPERTY (private, QString,				HTML)

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
		void				print (QString text, bool allow_internals);
		void				writeIRCMessage (QString from, QString msg);

		static const QList<Context*>& getAllContexts();
		static Context* getFromTreeWidgetItem (QTreeWidgetItem* item);
		static Context* getCurrentContext();
		static void setCurrentContext (Context* context);

	private:
		void commonInit();
};

#endif // COBALTIRC_CONTEXT_H
