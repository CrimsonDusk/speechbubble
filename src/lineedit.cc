#include "lineedit.h"
#include <QPaintEvent>

SBLineEdit::SBLineEdit (QWidget* parent) :
	Super (parent) {}

SBLineEdit::SBLineEdit (const QString& text, QWidget* parent) :
	Super (text, parent) {}

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
	if (ev->modifiers() & Qt::ControlModifier)
	{
		switch (ev->key())
		{
			case Qt::Key_B:
				insert (BOLD_STR);
				break;

			case Qt::Key_U:
				insert (UNDERLINE_STR);
				break;

			case Qt::Key_R:
				insert (REVERSE_STR);
				break;

			case Qt::Key_K:
				insert (COLOR_STR);
				break;

			case Qt::Key_O:
				insert (NORMAL_STR);
				break;

			default:
				goto defaultbehavior;
		}

		ev->accept();
		return;
	}

defaultbehavior:
	Super::keyPressEvent (ev);
}

void SBLineEdit::paintEvent (QPaintEvent* ev)
{
	Super::paintEvent (ev);
}
