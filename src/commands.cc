#include "commands.h"
#include "context.h"
#include "connection.h"
#include "misc.h"

#define COMMAND_FUNCTION_NAME(N) CommandDefinition_##N
#define DEFINE_COMMAND(N) static void COMMAND_FUNCTION_NAME(N) (QStringList args, const CommandInfo* cmdinfo)
#define DECLARE_COMMAND(N) { #N, &COMMAND_FUNCTION_NAME(N) },
#define ALIAS_COMMAND(A,B) DEFINE_COMMAND (A) { COMMAND_FUNCTION_NAME(B) (args, cmdinfo); }
#define CHECK_PARMS(MIN,MAX,USAGE) checkParms (MIN, MAX, USAGE, args, cmdinfo);

// ============================================================================
//
static void error (QString message)
{
	throw CommandError (message);
}

// ============================================================================
//
static void padArgsTo (QStringList& args, int a)
{
	while (args.size() < a)
		args << "";
}

// ============================================================================
//
static void checkParms (int minparms, int maxparms, QString parmusage,
	QStringList& args, const CommandInfo* cmdinfo)
{
	if (args.size() < minparms || (maxparms != -1 && args.size() > maxparms))
		error (format("Too %1 arguments\nUsage: /%2 %3",
			(args.size() < minparms ? "few" : "many"),
			cmdinfo->name, parmusage));
	
	if (maxparms != -1)
		padArgsTo (args, maxparms);
	
	assert (maxparms == -1 || args.size() == maxparms);
}

// ============================================================================
//
static inline void writeRaw (QString text)
{
	getCurrentConnection()->write (text);
}

// ============================================================================
//
DEFINE_COMMAND (nick)
{
	CHECK_PARMS (1, 1, "<newnick>")
	writeRaw (format("NICK %1\n", args[0]));
}

// ============================================================================
//
DEFINE_COMMAND (join)
{
	CHECK_PARMS (1, 2, "<channel> [password]")
	writeRaw (format("JOIN %1 %2\n", args[0], args[1]));
}

// ============================================================================
//
DEFINE_COMMAND (part)
{
	CHECK_PARMS (1, -1, "<channel> [partmessage]")
	padArgsTo (args, 2);
	writeRaw (format("PART %1 :%2\n", args[0], subset (args, 1)));
}

// ============================================================================
//
DEFINE_COMMAND (quote)
{
	(void) cmdinfo;

	if (args.isEmpty() == false)
	{
		QString msg = args.join (" ");
		Context::currentContext()->print ("QUOTE: " + msg);
		writeRaw (msg + '\n');
	}
}

// ============================================================================
//
// Command aliases
//
ALIAS_COMMAND (j,	join)
ALIAS_COMMAND (raw,	quote)

// ============================================================================
//
// Command declarations
//
const CommandInfo g_Commands[] =
{
	DECLARE_COMMAND (join)
	DECLARE_COMMAND (j)
	DECLARE_COMMAND (nick)
	DECLARE_COMMAND (part)
	DECLARE_COMMAND (quote)
	DECLARE_COMMAND (raw)
};

// ============================================================================
//
const CommandInfo* getCommandByName (const QString& name)
{
	for (const CommandInfo& info : g_Commands)
		if (info.name == name.toLower())
			return &info;

	return null;
}
