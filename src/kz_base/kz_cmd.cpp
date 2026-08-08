#include "amxxmodule.h"

#include "pdata.h"
#include "kz_cmd.h"

#include <cstring>
#include <vector>

struct kz_command_t
{
    const char* name;
    kz_cmd_fn   callback;
    bool        chat_only;
};

static std::vector<kz_command_t> g_commands;

// The engine only routes registered commands to ClientCommand; a no-op is enough to register them.
static void kz_cmd_dummy() {}

void kz_register_client_command(const char* command, kz_cmd_fn callback, bool chat_only)
{
    for (const kz_command_t& c : g_commands)
    {
        if (strcmp(c.name, command) == 0)
        {
            return; // already registered
        }
    }

    g_commands.push_back({ command, callback, chat_only });

    if (!chat_only)
    {
        REG_SVR_COMMAND(const_cast<char*>(command), kz_cmd_dummy);
    }
}

static bool dispatch_name(edict_t* pEntity, const char* name, bool from_chat)
{
    for (const kz_command_t& c : g_commands)
    {
        if (strcmp(c.name, name) == 0)
        {
            if (from_chat || !c.chat_only)
            {
                c.callback(pEntity);
                return true;
            }
            return false; // chat_only command used directly: ignore
        }
    }
    return false;
}

bool kz_cmd_dispatch(edict_t* pEntity)
{
    const char* cmd = CMD_ARGV(0);

    if (strcmp(cmd, "say") == 0 || strcmp(cmd, "say_team") == 0)
    {
        const char* msg = CMD_ARGV(1);
        if (msg[0] == '"')
        {
            msg++;
        }
        if (msg[0] != '/')
        {
            return false;
        }
        return dispatch_name(pEntity, msg + 1, true);
    }
    if (cmd[0] == '/')
    {
        cmd++;
    }
    return dispatch_name(pEntity, cmd, false);
}