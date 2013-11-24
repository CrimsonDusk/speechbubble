#include "commands.h"
#include "context.h"
#include "connection.h"
#include "misc.h"

#define DEFINE_COMMAND(N) void COIRC_Command_##N (QStringList args, const CommandInfo* cmdinfo)
#define DECLARE_COMMAND(N) { #N, &COIRC_Command_##N },
#define ALIAS_COMMAND(A,B) DEFINE_COMMAND (A) { COIRC_Command_##B (args, cmdinfo); }
#define CHECK_PARMS(MIN,MAX,USAGE) checkParms (MIN, MAX, USAGE, args, cmdinfo);

// ============================================================================
// ----------------------------------------------------------------------------
static void error (QString message)
{	throw CommandError (message);
}

// ============================================================================
// ----------------------------------------------------------------------------
static void padArgsTo (QStringList& args, int a)
{	while (args.size() < a)
	args << "";
}

// ============================================================================
// ----------------------------------------------------------------------------
static void checkParms (int minparms, int maxparms, QString parmusage, 
								QStringList& args, const CommandInfo* cmdinfo)
{	if (args.size() < minparms || (maxparms != -1 && args.size() > maxparms))
		error (fmt ("Too %1 arguments\nUsage: /%2 %3",
			(args.size() < minparms ? "few" : "many"),
			cmdinfo->name, parmusage));

	if (maxparms != -1)
		padArgsTo (args, maxparms);

	assert (maxparms == -1 || args.size() == maxparms);
}

// ============================================================================
// ----------------------------------------------------------------------------
static inline IRCConnection* getCurrentConnection()
{	return Context::getCurrentContext()->getConnection();
}

// ============================================================================
// ----------------------------------------------------------------------------
static inline void writeRaw (QString text)
{	getCurrentConnection()->write (text);
}

// ============================================================================
// ----------------------------------------------------------------------------
DEFINE_COMMAND (nick)
{	CHECK_PARMS (1, 1, "<newnick>")
	writeRaw (fmt ("NICK %1\n", args[0]));
}

// ============================================================================
// ----------------------------------------------------------------------------
DEFINE_COMMAND (join)
{	CHECK_PARMS (1, 2, "<channel> [password]")
	writeRaw (fmt ("JOIN %1\n", args[0]));
}

// ============================================================================
// ----------------------------------------------------------------------------
DEFINE_COMMAND (part)
{	CHECK_PARMS (1, -1, "<channel> [partmessage]")
	padArgsTo (args, 2);
	writeRaw (fmt ("PART %1 :%2\n", args[0], lrange (args, 1)));
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