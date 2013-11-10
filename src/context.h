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
		enum Type
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
	PROPERTY (private, Context*,							parent)
	PROPERTY (private, TargetUnion,						target)
	PROPERTY (private, Type,								type)
	PROPERTY (private, int,									id)
	PROPERTY (private, QString,							html)

	public:
		Context (IRCConnection* conn);
		Context (IRCChannel* channel);
		Context (IRCUser* user);
		~Context();

		void				add_subcontext (Context* child);
		void				forget_subcontext (Context* child);
		void				update_tree_item();
		IRCConnection*	GetConnection();
		QString			name() const;
		void				Print (QString text, bool allow_internals);
		void				WriteIRCMessage (QString from, QString msg);

		static const list<Context*>& all_contexts();
		static Context* from_tree_widget_item (QTreeWidgetItem* item);
		static Context* CurrentContext();
		static void set_current_context (Context* context);

	private:
		void common_init();
};

#endif // COBALTIRC_CONTEXT_H
