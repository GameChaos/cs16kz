#include "amxxmodule.h"

#include "pdata.h"
#include "kz_util.h"

void UTIL_split_net_address(const char* addr, char* ip, size_t ip_maxlen, char* port, size_t port_maxlen)
{
    char* ptr;
    if(!port || !port_maxlen)
    {
        snprintf(ip, ip_maxlen, "%s", addr);
        if((ptr = strstr(ip, ":")) != nullptr)
        {
            *ptr = '\0';
        }
        return;
    }
    
    ptr = strstr(addr, ":");
    if(ptr)
    {
        snprintf(ip, ip_maxlen, "%.*s", static_cast<int>(ptr - addr), addr);
        snprintf(port, port_maxlen, "%s", ptr + 1);
    }
    else
    {
        snprintf(ip, ip_maxlen, "%s", addr);
        port[0] = '\0';
    }
}
edict_t* UTIL_find_player_by_authid(const char* authid)
{
    for(int i = 1; i <= gpGlobals->maxClients; i++)
    {
        edict_t* pEntity = edictByIndex(i);

        if(!FNullEnt(pEntity) && !MF_IsPlayerBot(i))
        {
            const char* authid2 = GETPLAYERAUTHID(pEntity);
            if(authid2 && strcmp(authid, authid2) == 0)
            {
                return pEntity;
            }
        }
    }
    return NULL;
}
cvar_t* UTIL_register_cvar(const char* name, const char* value, int flags)
{
    cvar_t reg_helper;
    reg_helper.name = name;
    reg_helper.string = value;
    reg_helper.flags = flags;

    CVAR_REGISTER(&reg_helper);
    return CVAR_GET_POINTER(name);
}