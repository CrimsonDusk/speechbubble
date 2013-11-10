#include "commands.h"
#include "context.h"
#include "connection.h"

#define DEFINE_COMMAND(N) void COIRC_Command_##N (QStringList args)
#define DECLARE_COMMAND(N) { #N, &COIRC_Command_##N },

// ============================================================================
// ----------------------------------------------------------------------------
static void error (QString message)
{	throw CommandError (message);
}

// ============================================================================
// ----------------------------------------------------------------------------
static inline IRCConnection* CurrentConnection()
{	return Context::CurrentContext()->GetConnection();
}

// ============================================================================
// ----------------------------------------------------------------------------
DEFINE_COMMAND (nick)
{	if (args.size() != 1)
		error ("Usage: /nick <new_name>");

	CurrentConnection()->write (fmt ("NICK %1", args[0]));
}

// ============================================================================
// ----------------------------------------------------------------------------
const CommandInfo g_Commands[] =
{	DECLARE_COMMAND (nick)
};

const int g_NumCommands = (sizeof g_Commands / sizeof *g_Commands);