#ifndef COMMANDS_H
#define COMMANDS_H

#include "main.h"

struct CommandInfo;
using CommandFunction = void (*) (QStringList, const CommandInfo*);

struct CommandInfo
{
	QString			name;
	CommandFunction	func;
};

class CommandError : public std::exception
{
	public:
		CommandError (QString error) :
			mError (error) {}
		
		virtual inline const char* what() const throw()
		{
			return mError.toStdString().c_str();
		}
	
	private:
		QString mError;
};

const CommandInfo* getCommandByName (const QString& name);

#endif // COMMANDS_H

