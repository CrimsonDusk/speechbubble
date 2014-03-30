#pragma once
#include <QLineEdit>
#include "main.h"

class QPaintEvent;

class SBLineEdit : public QLineEdit
{
public:
	explicit SBLineEdit (QWidget* parent = null);
	explicit SBLineEdit (const QString& text, QWidget* parent = null);
	SBLineEdit (const QLineEdit&) = delete;

	QVariant	inputMethodQuery (Qt::InputMethodQuery type) const override;

protected:
	void		keyPressEvent (QKeyEvent* ev) override;
};
