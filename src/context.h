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
class IRCContextTreeWidgetItem : public QTreeWidgetItem
{	PROPERTY (public, Context*, context)

	public:
		IRCContextTreeWidgetItem (Context* context) :
			QTreeWidgetItem ((QTreeWidgetItem*) null),
			m_context (context) {}
};

// =============================================================================
// -----------------------------------------------------------------------------
class Context final : public QObject
{	public:
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

	PROPERTY (public,  IRCContextTreeWidgetItem*,	treeitem)
	PROPERTY (public,  QTextDocument*,					document)
	PROPERTY (private, QList<Context*>,					subcontexts)
	NEW_PROPERTY (private, Context*,							Parent)
	NEW_PROPERTY (private, TargetUnion,						Target)
	NEW_PROPERTY (private, ContextType,						Type)
	NEW_PROPERTY (private, int,								ID)
	NEW_PROPERTY (private, QString,							HTML)

	public:
		Context (IRCConnection* conn);
		Context (IRCChannel* channel);
		Context (IRCUser* user);
		~Context();

		void				AddSubContext (Context* child);
		void				forget_subcontext (Context* child);
		void				UpdateTreeItem();
		IRCConnection*	GetConnection();
		QString			GetName() const;
		void				Print (QString text, bool allow_internals);
		void				WriteIRCMessage (QString from, QString msg);

		static const list<Context*>& all_contexts();
		static Context* FromTreeWidgetItem (QTreeWidgetItem* item);
		static Context* CurrentContext();
		static void SetCurrentContext (Context* context);

	private:
		void CommonInit();
};

#endif // COBALTIRC_CONTEXT_H
