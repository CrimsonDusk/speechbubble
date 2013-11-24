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

	PROPERTY (private, bool, CtrlPressed)

	public:
		explicit MainWindow (QWidget* parent = 0, Qt::WindowFlags flags = 0);
		virtual ~MainWindow();

		void addContext (Context* a);
		void removeContext (Context* a);
		void updateOutputWidget();

	public slots:
		void actionConnectTo();
		void actionDisconnect();
		void actionQuit();
		void contextSelected (QTreeWidgetItem* item);
		void inputEnterPressed();
		void updateUserlist();

	protected:
		void closeEvent (QCloseEvent* ev);
		void keyPressEvent (QKeyEvent* ev);
		void keyReleaseEvent (QKeyEvent* ev);

	private:
		Ui_MainWindow* m_ui;

		void updateWindowTitle();
};

extern MainWindow* win;

#endif // COIRC_MAINWINDOW_H