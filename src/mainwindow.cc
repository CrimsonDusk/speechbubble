#include <QDialog>
#include <QCloseEvent>
#include <QMessageBox>
#include "mainwindow.h"
#include "connection.h"
#include "context.h"
#include "ui_main.h"
#include "ui_connectTo.h"
#include "moc_mainwindow.cpp"
#include "config.h"
#include "commands.h"
#include "user.h"

#define CALIBRATE_ACTION(NAME) \
	connect (m_ui->action##NAME, SIGNAL (triggered()), this, SLOT (action##NAME()));

MainWindow* win = null;
QTextDocument g_defaultDocument;

CONFIG (String,	quicklaunch_nick,		"")
CONFIG (String,	quicklaunch_server,	"")
CONFIG (Int,		quicklaunch_port,		6667)
CONFIG (Font,		output_font,			QFont())

// =============================================================================
// -----------------------------------------------------------------------------
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

	connect (m_ui->m_channels, SIGNAL (currentItemChanged (QTreeWidgetItem*, QTreeWidgetItem*)),
		this, SLOT (contextSelected (QTreeWidgetItem*)));
	connect (m_ui->m_input, SIGNAL (returnPressed()), this, SLOT (inputEnterPressed()));

	m_ui->m_output->setFontFamily ("Monospace");
	m_ui->m_output->setFontPointSize (11);
}

// =============================================================================
// -----------------------------------------------------------------------------
MainWindow::~MainWindow()
{	delete m_ui;
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::updateWindowTitle()
{	QString title = fmt (APPNAME " %1", getVersionString());
	setWindowTitle (title);
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::actionConnectTo()
{	QDialog* dlg = new QDialog (this);
	Ui_ConnectTo ui;
	ui.setupUi (dlg);
	ui.m_nick->setText (cfg::quicklaunch_nick);
	ui.m_host->setText (cfg::quicklaunch_server);
	ui.m_port->setValue (cfg::quicklaunch_port);

	if (!dlg->exec())
		return;

	cfg::quicklaunch_nick = ui.m_nick->text();
	cfg::quicklaunch_server = ui.m_host->text();
	cfg::quicklaunch_port = ui.m_port->value();

	IRCConnection* conn = new IRCConnection (ui.m_host->text(), ui.m_port->value());
	conn->setNick (ui.m_nick->text());
	conn->setUser (conn->getNick());
	conn->setName (conn->getNick());
	conn->connectToServer();
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::actionDisconnect()
{	Context* ctx = Context::getCurrentContext();
	ctx->getConnection()->disconnectFromServer();
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::actionQuit()
{	exit (0);
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::addContext (Context* a)
{	log ("Add context #%1 as top-level item\n", a->getID());
	m_ui->m_channels->addTopLevelItem (a->getTreeItem());
	a->getTreeItem()->setExpanded (true);
	a->updateTreeItem();
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::removeContext (Context* a)
{	delete a->getTreeItem();
	a->setTreeItem (null);
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::closeEvent (QCloseEvent* ev)
{	int numactive = 0;

	for (IRCConnection* conn : IRCConnection::getAllConnections())
		if (conn->getState() != IRCConnection::EDisconnected)
			numactive++;

	if (numactive > 0)
	{	QString msg;

		if (numactive == 1)
			msg = tr ("There is an active connection, do you really want to quit?");
		else
			msg = fmt (tr ("There are %1 active connections, do you really want to quit?"), numactive);

		if (QMessageBox::question (this, "Really quit?",
			msg, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
		{	ev->ignore();
			return;
		}
	}

	for (Context* c : Context::getAllContexts())
		c->getConnection()->disconnectFromServer();

	Config::saveToFile (configname);
	ev->accept();
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::updateOutputWidget()
{	Context* context = Context::getCurrentContext();
	m_ui->m_output->setEnabled (context != null);
	m_ui->m_output->setDocument (context ? context->getDocument() : &g_defaultDocument);
	m_ui->m_output->setFont (cfg::output_font);
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::contextSelected (QTreeWidgetItem* item)
{	Context::setCurrentContext (Context::getFromTreeWidgetItem (item));
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::keyPressEvent (QKeyEvent* ev)
{	setCtrlPressed (ev->modifiers() & Qt::ControlModifier);
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::keyReleaseEvent (QKeyEvent* ev)
{	setCtrlPressed (ev->modifiers() & Qt::ControlModifier);
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::inputEnterPressed() // [slot]
{	QString input = m_ui->m_input->text();
	Context* context = Context::getCurrentContext();
	IRCConnection* conn = context->getConnection();

	if (input.isEmpty() && !getCtrlPressed())
		return;

	// Remove the message from the input field
	m_ui->m_input->setText ("");

	if (input.startsWith ("/"))
	{	input.remove (0, 1); // remove the '/'
		QStringList args = input.split (" ", QString::SkipEmptyParts);
		QString cmd = args[0];
		args.removeFirst();

		for (int i = 0; i < g_NumCommands; ++i)
		{	if (cmd.toLower() == g_Commands[i].name)
			{	try
				{	(*g_Commands[i].func) (args, &g_Commands[i]);
				} catch (CommandError& err)
				{	Context::getCurrentContext()->print (fmt (tr ("\\b\\c4Error: %1\n"), err.what()), true);
				}

				return;
			}
		}

		// No command matched, send as raw
		Context::getCurrentContext()->print (fmt ("-> raw: %1\n", input), true);
		conn->write (input + "\n");
		return;
	}

	switch (context->getType())
	{	case Context::EServerContext:
		{	conn->write (input + "\n");
		} break;

		case Context::EChannelContext:
		{	conn->write (fmt ("PRIVMSG %1 :%2\n", context->getTarget().chan->getName(), input));
		} break;

		case Context::EQueryContext:
		{	conn->write (fmt ("PRIVMSG %1 :%2\n", context->getTarget().user->getNickname(), input));
		} break;
	}
}