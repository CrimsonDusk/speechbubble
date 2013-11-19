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
	connect (m_ui->action##NAME, SIGNAL (triggered()), this, SLOT (Action##NAME()));

MainWindow* win = null;
QTextDocument blank_document;

CONFIG (String, quicklaunch_nick, "")
CONFIG (String, quicklaunch_server, "")
CONFIG (Int,    quicklaunch_port, 6667)
CONFIG (String, output_font, "")
CONFIG (Int,    output_font_size, 0)

// =============================================================================
// -----------------------------------------------------------------------------
MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags) :
	QMainWindow (parent, flags),
	m_ui (new Ui_MainWindow)
{
	m_ui->setupUi (this);
	win = this;
	UpdateWindowTitle();

	CALIBRATE_ACTION (ConnectTo)
	CALIBRATE_ACTION (Disconnect)
	CALIBRATE_ACTION (Quit)

	connect (m_ui->m_channels, SIGNAL (currentItemChanged (QTreeWidgetItem*, QTreeWidgetItem*)),
		this, SLOT (ContextSelected (QTreeWidgetItem*)));
	connect (m_ui->m_input, SIGNAL (returnPressed()), this, SLOT (InputEnterPressed()));

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
void MainWindow::UpdateWindowTitle()
{	QString title = fmt (APPNAME " %1", version_string());
	setWindowTitle (title);
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::ActionConnectTo()
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
	conn->set_nick (ui.m_nick->text());
	conn->set_user (conn->nick());
	conn->set_name (conn->nick());
	conn->start();
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::ActionDisconnect()
{	Context* ctx = Context::CurrentContext();
	ctx->GetConnection()->stop();
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::ActionQuit()
{	exit (0);
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::AddContext (Context* a)
{	m_ui->m_channels->addTopLevelItem (a->treeitem());
	a->treeitem()->setExpanded (true);
	a->UpdateTreeItem();
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::RemoveContext (Context* a)
{	delete a->treeitem();
	a->set_treeitem (null);
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::closeEvent (QCloseEvent* ev)
{	int numactive = 0;

	for (Context* c : Context::all_contexts())
		if (c->Type() == Context::EServerContext && c->GetConnection()->state() == IRCConnection::EConnected)
			numactive++;

	if (numactive > 0)
	{	if (QMessageBox::question (this, "Really quit?",
			fmt (tr ("There are %1 active connections, do you really want to quit?"), numactive),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
		{	ev->ignore();
			return;
		}
	}

	for (Context* c : Context::all_contexts())
		c->GetConnection()->stop();

	Config::SaveTo (configname);
	ev->accept();
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::UpdateOutputWidget()
{	Context* context = Context::CurrentContext();
	m_ui->m_output->setEnabled (context != null);

	if (context)
		m_ui->m_output->setDocument (context->document());
	else
		m_ui->m_output->setDocument (&blank_document);
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::ContextSelected (QTreeWidgetItem* item)
{	Context::SetCurrentContext (Context::FromTreeWidgetItem (item));
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::keyPressEvent (QKeyEvent* ev)
{	SetCtrlPressed (ev->modifiers() & Qt::ControlModifier);
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::keyReleaseEvent (QKeyEvent* ev)
{	SetCtrlPressed (ev->modifiers() & Qt::ControlModifier);
}

// =============================================================================
// -----------------------------------------------------------------------------
void MainWindow::InputEnterPressed() // [slot]
{	QString input = m_ui->m_input->text();
	Context* context = Context::CurrentContext();
	IRCConnection* conn = context->GetConnection();

	if (input.isEmpty() && !CtrlPressed())
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
				{	Context::CurrentContext()->Print (fmt (tr ("\\b\\c4Error: %1\n"), err.what()), true);
				}

				return;
			}
		}

		// No command matched, send as raw
		Context::CurrentContext()->Print (fmt ("-> raw: %1\n", input), true);
		conn->write (input + "\n");
		return;
	}

	switch (context->Type())
	{	case Context::EServerContext:
		{	conn->write (input + "\n");
		} break;

		case Context::EChannelContext:
		{	conn->write (fmt ("PRIVMSG %1 :%2\n", context->Target().chan->name(), input));
		} break;

		case Context::EQueryContext:
		{	conn->write (fmt ("PRIVMSG %1 :%2\n", context->Target().user->nick(), input));
		} break;
	}
}