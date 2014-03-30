#include "lineedit.h"
#include <QPaintEvent>

SBLineEdit::SBLineEdit (QWidget* parent) :
	QLineEdit (parent) {}

SBLineEdit::SBLineEdit (const QString& text, QWidget* parent) :
	QLineEdit (text, parent) {}

QVariant SBLineEdit::inputMethodQuery (Qt::InputMethodQuery type) const
{
	if (type == Qt::ImSurroundingText)
	{
		QString displayText = text();
		displayText.replace (BOLD_STR, QString::fromUtf16 (reinterpret_cast<const ushort*> (u"Ⓑ")));
		return QVariant (displayText);
	}

	return QLineEdit::inputMethodQuery (type);
}

void SBLineEdit::keyPressEvent (QKeyEvent* ev)
{
	if (ev->modifiers() & Qt::ControlModifier && ev->key() == Qt::Key_B)
	{
		insert (BOLD_STR);
		ev->accept();
		return;
	}

	QLineEdit::keyPressEvent (ev);
}
