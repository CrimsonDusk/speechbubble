#ifndef COIRC_MAINWINDOW_H
#define COIRC_MAINWINDOW_H

#include <QMainWindow>
#include "main.h"

class QTreeWidgetItem;
class QCloseEvent;
class Context;
class Ui_MainWindow;

class MainWindow : public QMainWindow
{	Q_OBJECT

	NEW_PROPERTY (private, bool, CtrlPressed)

	public:
		explicit MainWindow (QWidget* parent = 0, Qt::WindowFlags flags = 0);
		virtual ~MainWindow();

		void AddContext (Context* a);
		void RemoveContext (Context* a);
		void UpdateOutputWidget();

	public slots:
		void ActionConnectTo();
		void ActionDisconnect();
		void ActionQuit();
		void ContextSelected (QTreeWidgetItem* item);
		void InputEnterPressed();

	protected:
		void closeEvent (QCloseEvent* ev);
		void keyPressEvent (QKeyEvent* ev);
		void keyReleaseEvent (QKeyEvent* ev);

	private:
		Ui_MainWindow* m_ui;

		void UpdateWindowTitle();
};

extern MainWindow* win;

#endif // COIRC_MAINWINDOW_H