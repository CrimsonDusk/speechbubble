#include <QDialog>
#include <QCloseEvent>
#include <QMessageBox>
#include "mainwindow.h"
#include "connection.h"
#include "context.h"
#include "ui_main.h"
#include "ui_connectTo.h"
#include "config.h"
#include "commands.h"
#include "user.h"

#define CALIBRATE_ACTION(NAME) \
	connect (m_ui->action##NAME, SIGNAL (triggered()), this, SLOT (action##NAME()));

MainWindow* win = null;
QTextDocument g_defaultDocument;

CONFIG (String,		quicklaunch_nick,		"")
CONFIG (String,		quicklaunch_server,		"")
CONFIG (Int,		quicklaunch_port,		6667)
CONFIG (Font,		output_font,			QFont())

// =============================================================================
//
MainWindow::MainWindow (QWidget* parent, Qt::WindowFlags flags) :
	QMainWindow (parent, flags),
	m_ui (new Ui_MainWindow)
{
	m_ui->setupUi (this);
	win = this;
	updateWindowTitle();

	CALIBRATE_ACTION (ConnectTo)
	CALIBRATE_ACTION (Disconnect)
	CALIBRATE_ACTION (Quit)
	CALIBRATE_ACTION (BanList)
	CALIBRATE_ACTION (ExceptList)
	CALIBRATE_ACTION (InviteList)

	connect (m_ui->m_channels, SIGNAL (currentItemChanged (QTreeWidgetItem*, QTreeWidgetItem*)),
			 this, SLOT (contextSelected (QTreeWidgetItem*)));
	connect (m_ui->m_input, SIGNAL (returnPressed()), this, SLOT (inputEnterPressed()));
}

// =============================================================================
//
MainWindow::~MainWindow()
{
	delete m_ui;
}

// =============================================================================
//
void MainWindow::updateWindowTitle()
{
	QString title = format(APPNAME " %1", getVersionString());
	setWindowTitle (title);
}

// =============================================================================
//
void MainWindow::actionConnectTo()
{
	QDialog* dlg = new QDialog (this);
	Ui_ConnectTo ui;
	ui.setupUi (dlg);
	ui.m_nick->setText (cfg::quicklaunch_nick);
	ui.m_host->setText (cfg::quicklaunch_server);
	ui.m_port->setValue (cfg::quicklaunch_port);

	if (dlg->exec() == QDialog::Rejected)
		return;

	cfg::quicklaunch_nick = ui.m_nick->text();
	cfg::quicklaunch_server = ui.m_host->text();
	cfg::quicklaunch_port = ui.m_port->value();

	IRCConnection* conn = new IRCConnection (ui.m_host->text(), ui.m_port->value());
	conn->setNickname (ui.m_nick->text());
	conn->setUsername (conn->nickname());
	conn->setRealname (conn->nickname());
	conn->connectToServer();
}

// =============================================================================
//
void MainWindow::actionDisconnect()
{
	Context* ctx = Context::currentContext();
	ctx->connection()->disconnectFromServer();
}

// =============================================================================
//
void MainWindow::actionQuit()
{
	exit (EXIT_SUCCESS);
}

// =============================================================================
//
void MainWindow::addContext (Context* a)
{
	m_ui->m_channels->addTopLevelItem (a->treeItem());
	a->treeItem()->setExpanded (true);
	a->updateTreeItem();
}

// =============================================================================
//
void MainWindow::removeContext (Context* a)
{
	delete a->treeItem();
	a->setTreeItem (null);
}

// =============================================================================
//
void MainWindow::closeEvent (QCloseEvent* ev)
{
	int numactive = 0;

	for (IRCConnection * conn : IRCConnection::getAllConnections())
		if (conn->state() != CNS_Disconnected)
			numactive++;

	if (numactive > 0)
	{
		QString msg;

		if (numactive == 1)
			msg = tr ("There is an active connection, do you really want to quit?");
		else
			msg = format(tr ("There are %1 active connections, do you really want to quit?"), numactive);

		if (QMessageBox::question (this, "Really quit?",
			msg, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
		{
			ev->ignore();
			return;
		}
	}

	for (Context* c : Context::allContexts())
		c->connection()->disconnectFromServer();

	Config::saveToFile (configname);
	ev->accept();
}

// =============================================================================
//
void MainWindow::updateOutputWidget()
{
	Context* context = Context::currentContext();
	m_ui->m_output->setEnabled (context != null);
	m_ui->m_output->setDocument (context ? context->document() : &g_defaultDocument);
	m_ui->m_output->setFont (cfg::output_font);
}

// =============================================================================
//
void MainWindow::contextSelected (QTreeWidgetItem* item)
{
	Context* oldContext = Context::currentContext();
	Context* newContext = Context::fromTreeWidgetItem (item);

	if (oldContext == newContext)
		return;

	Context::setCurrentContext (newContext);
	updateUserlist();
}

// =============================================================================
//
void MainWindow::keyPressEvent (QKeyEvent* ev)
{
	setCtrlPressed (ev->modifiers() & Qt::ControlModifier);
}

// =============================================================================
//
void MainWindow::keyReleaseEvent (QKeyEvent* ev)
{
	setCtrlPressed (ev->modifiers() & Qt::ControlModifier);
}

// =============================================================================
//
void MainWindow::inputEnterPressed() // [slot]
{
	QString input = m_ui->m_input->text();
	Context* context = Context::currentContext();
	IRCConnection* conn = context->connection();

	if (input.isEmpty() && !isCtrlPressed())
		return;

	// Remove the message from the input field
	m_ui->m_input->setText ("");

	if (input.startsWith ("/"))
	{
		input.remove (0, 1); // remove the '/'
		QStringList args = input.split (" ", QString::SkipEmptyParts);
		QString cmd = args[0];
		args.removeFirst();

		const CommandInfo* info = getCommandByName (cmd);

		if (info != null)
		{
			try
			{
				(info->func) (args, info);
			}
			catch (CommandError& err)
			{
				Context::printToCurrent (format(tr ("\\b\\c4Error: %1"), err.what()));
			}

			return;
		}

		// No command matched, send as raw
		// Context::printToCurrent (format("-> raw: %1\n", input));
		// conn->write (input + "\n");
		Context::printToCurrent (format(tr ("\\b\\c4Unknown command \"%1\""), cmd));
		return;
	}

	switch (context->type())
	{
		case CTX_Server:
		{
			conn->write (input + "\n");
			context->print (input);
			break;
		}

		case CTX_Channel:
		{
			conn->write (format("PRIVMSG %1 :%2\n", context->target().chan->name(), input));
			context->writeIRCMessage (conn->ourselves()->nickname(), input);
			break;
		}

		case CTX_Query:
		{
			conn->write (format("PRIVMSG %1 :%2\n", context->target().user->nickname(), input));
			context->writeIRCMessage (conn->ourselves()->nickname(), input);
			break;
		}
	}
}

// =============================================================================
//
void MainWindow::updateUserlist()
{
	IRCChannel* senderChan = qobject_cast<IRCChannel*> (sender());
	Context* ctx = Context::currentContext();
	IRCChannel* currentChan = (ctx->type() == CTX_Channel) ? ctx->target().chan : null;

	// If this is triggered by a channel through a connection, only update if the
	// channel is the one we have selected right now.
	if (senderChan != null && currentChan != senderChan)
		return;

	m_ui->m_userlist->clear();

	// If we're not in a channel, don't re-populate it.
	if (currentChan == null)
		return;

	auto sortFunction =
		[currentChan] (const UserlistEntry& a, const UserlistEntry& b) -> bool
		{
			EStatus statusA = a.userInfo()->getStatusInChannel (currentChan);
			EStatus statusB = b.userInfo()->getStatusInChannel (currentChan);

			if (statusA > statusB)
				return true;

			if (statusA < statusB)
				return false;

			QString nickA = a.userInfo()->nickname();
			QString nickB = b.userInfo()->nickname();
			return nickA.localeAwareCompare (nickB) > 0;
		};

	QList<UserlistEntry> users = currentChan->userlist();
	std::sort (users.begin(), users.end(), sortFunction);

	for (const UserlistEntry& e : users)
		m_ui->m_userlist->addItem (e.userInfo()->nickname());
}

enum EntryListType
{
	ENTRYLIST_BanList,
	ENTRYLIST_ExceptList,
	ENTRYLIST_InviteList,
};

static void viewEntryList (EntryListType type)
{
	Context* ctx = Context::currentContext();

	if (ctx->type() != CTX_Channel)
		return;

	IRCChannel* chan = ctx->target().chan;
}

// =============================================================================
//
void MainWindow::actionBanList()
{
	printf ("Ban list\n");
}

// =============================================================================
//
void MainWindow::actionExceptList()
{
	printf ("Exception list\n");
}

// =============================================================================
//
void MainWindow::actionInviteList()
{
	printf ("Invite list\n");
}
