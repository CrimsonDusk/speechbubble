#ifndef COMMANDS_H
#define COMMANDS_H

#include "main.h"

typedef void (*CommandFunction) (QStringList);

struct CommandInfo
{	QString				name;
	CommandFunction	func;
};

extern const CommandInfo	g_Commands[];
extern const int				g_NumCommands;

class CommandError : public std::exception
{	public:
		CommandError (QString error) : m_error (error) {}

		virtual inline const char* what() const throw()
		{	return m_error.toStdString().c_str();
		}

	private:
		QString m_error;
};

#endif // COMMANDS_H