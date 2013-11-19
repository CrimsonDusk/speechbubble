#include "commands.h"
#include "context.h"
#include "connection.h"

#define DEFINE_COMMAND(N) void COIRC_Command_##N (QStringList args, const CommandInfo* cmdinfo)
#define DECLARE_COMMAND(N) { #N, &COIRC_Command_##N },
#define ALIAS_COMMAND(A,B) DEFINE_COMMAND (A) { COIRC_Command_##B (args, cmdinfo); }
#define CHECK_PARMS(MIN,MAX,USAGE) CheckParms (MIN, MAX, USAGE, args, cmdinfo);

// ============================================================================
// ----------------------------------------------------------------------------
static void Error (QString message)
{	throw CommandError (message);
}

// ============================================================================
// ----------------------------------------------------------------------------
static void CheckParms (int minparms, int maxparms, QString parmusage, 
								QStringList& args, const CommandInfo* cmdinfo)
{	if (args.size() < minparms || args.size() > maxparms)
		Error (fmt ("Too %1 arguments\nUsage: /%2 %3",
			(args.size() < minparms ? "few" : "many"),
			cmdinfo->name, parmusage));

	while (args.size() < maxparms)
		args << "";	

	assert (args.size() == maxparms);
}

// ============================================================================
// ----------------------------------------------------------------------------
static inline void WriteRaw (QString text)
{	CurrentConnection()->write (text);
}

// ============================================================================
// ----------------------------------------------------------------------------
static inline IRCConnection* CurrentConnection()
{	return Context::CurrentContext()->GetConnection();
}

// ============================================================================
// ----------------------------------------------------------------------------
DEFINE_COMMAND (nick)
{	CHECK_PARMS (1, 1, "<newnick>")
	WriteRaw (fmt ("NICK %1\n", args[0]));
}

// ============================================================================
// ----------------------------------------------------------------------------
DEFINE_COMMAND (join)
{	CHECK_PARMS (1, 2, "<channel> [password]")
	WriteRaw (fmt ("JOIN %1\n", args[0]));
}

// ============================================================================
// ----------------------------------------------------------------------------
DEFINE_COMMAND (part)
{	CHECK_PARMS (1, 2, "<channel> [partmessage]")
	WriteRaw (fmt ("PART %1 :%2", args[0], args[1]));
}

// ============================================================================
// ----------------------------------------------------------------------------
// Aliases
ALIAS_COMMAND (j, join)

// ============================================================================
// ----------------------------------------------------------------------------
const CommandInfo g_Commands[] =
{	DECLARE_COMMAND (join)
	DECLARE_COMMAND (j)
	DECLARE_COMMAND (nick)
	DECLARE_COMMAND (part)
};

const int g_NumCommands = (sizeof g_Commands / sizeof *g_Commands);