#include "amxxmodule.h"
#include "resdk/mod_rehlds_api.h"

#include "pdata.h"
#include "kz_util.h"
#include "kz_cvars.h"

#include "kz_ws.h"
#include "kz_storage.h"
#include "kz_natives.h"

edict_t* g_pEdicts   = nullptr;
cvar_t* kz_api_url   = nullptr;
cvar_t* kz_api_token = nullptr;

cvar_t* kz_api_log_send = nullptr;
cvar_t* kz_api_log_recv = nullptr;

void RH_Cvar_DirectSet(IRehldsHook_Cvar_DirectSet* chain, cvar_t* var, const char* value);
void KZ_Cvar_DirectSet(const char* const varname, const char* const value);

/***************************************************************************************************************/
/***************************************************************************************************************/
int FN_AMXX_CHECKGAME(const char* game)
{
    return (FStrEq(game, "cstrike") ? AMXX_GAME_OK : AMXX_GAME_BAD);
}
void FN_AMXX_ATTACH()
{
    if (RehldsApi_Init())
    {
        RehldsHookchains->Cvar_DirectSet()->registerHook(RH_Cvar_DirectSet, HC_PRIORITY_UNINTERRUPTABLE);
    }

    g_pEdicts = (*g_engfuncs.pfnPEntityOfEntIndex)(0);
    
    kz_ws_register(WSMessageType::invalid,     kz_ws_ack_invalid);
    kz_ws_register(WSMessageType::hello,       kz_ws_ack_hello);
    kz_ws_register(WSMessageType::map_info,    kz_ws_ack_map_info);
    kz_ws_register(WSMessageType::client_info, kz_ws_ack_client_info);
    kz_ws_register(WSMessageType::add_record,  kz_ws_ack_add_record);
    kz_ws_register(WSMessageType::del_record,  kz_ws_ack_del_record);
    kz_ws_register(WSMessageType::add_replay,  kz_ws_ack_add_replay);
    kz_ws_register(WSMessageType::get_replay,  kz_ws_ack_get_replay);

    kz_api_url      = UTIL_register_cvar("kz_api_url",  "", FCVAR_EXTDLL | FCVAR_PROTECTED | FCVAR_SPONLY);
    kz_api_token    = UTIL_register_cvar("kz_api_token","", FCVAR_EXTDLL | FCVAR_PROTECTED | FCVAR_SPONLY);

    kz_api_log_send = UTIL_register_cvar("kz_api_log_send", "0", FCVAR_EXTDLL | FCVAR_SPONLY);
    kz_api_log_recv = UTIL_register_cvar("kz_api_log_recv", "0", FCVAR_EXTDLL | FCVAR_SPONLY);

    kz_storage_init(MF_Log);
    kz_ws_init(std::this_thread::get_id());

    kz_api_add_natives();
}
void FN_AMXX_PLUGINSLOADED()
{
    kz_api_add_forwards();
}
void FN_META_DETACH()
{
    kz_ws_uninit();
    kz_storage_uninit();
}
/***************************************************************************************************************/
/***************************************************************************************************************/
void FN_StartFrame()
{
    kz_run_cvar_checker();
    kz_ws_run_tasks(5);

    RETURN_META(MRES_IGNORED);
}
void FN_ServerDeactivate_Post(void)
{
    g_pEdicts = nullptr;
    RETURN_META(MRES_IGNORED);
}
void FN_DispatchKeyValue(edict_t* pentKeyvalue, KeyValueData* pkvd)
{
    if (FClassnameIs(pentKeyvalue, "worldspawn"))
    {
        g_pEdicts = pentKeyvalue;
    }
    RETURN_META(MRES_IGNORED);
}
int FN_DispatchSpawn(edict_t* pent)
{
    if (FClassnameIs(pent, "worldspawn"))
    {
        g_pEdicts = (*g_engfuncs.pfnPEntityOfEntIndex)(0);
    }
    RETURN_META_VALUE(MRES_IGNORED, FALSE);
}
void FN_CvarValue2(const edict_t* pEdict, int requestId, const char* cvar, const char* value)
{
    kz_qqc_handler(pEdict, requestId, cvar, value);
    RETURN_META(MRES_IGNORED);
}

/* This never gets called and i dont know why. Tried without rehlds/regamedll installed */
/* Maybe it works on amxmodx <= 1.8.2 (not tested) */
void FN_Cvar_DirectSet_Post(struct cvar_s *var, char *value) 
{
    if(!var || !value || FStrEq(var->string, value))
    {
        RETURN_META(MRES_IGNORED);
    }
    if(!g_rehlds_available)
    {
        KZ_Cvar_DirectSet(var->name, value);
    }
    RETURN_META(MRES_IGNORED);
}
/***************************************************************************************************************/
/***************************************************************************************************************/
void FN_PlayerPreThink(edict_t* pEntity)
{
    // TODO: dont trust qqc2 result and actually check stuff here...
}
BOOL FN_ClientConnect_Post(edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128])
{
    int id = ENTINDEX(pEntity);

    if (!MF_IsPlayerBot(id))
    {
        for(size_t i = 0; i < g_player_cvars_size; ++i)
        {
            // maybe no? idk
            CLIENT_COMMAND(pEntity, "%s %s\n", g_player_cvars[i].name, g_player_cvars[i].expected_value);
        }
        kz_ws_event_client_connect(pEntity);
    }
    RETURN_META_VALUE(MRES_IGNORED, TRUE);
}
/***************************************************************************************************************/
/***************************************************************************************************************/
void RH_Cvar_DirectSet(IRehldsHook_Cvar_DirectSet* chain, cvar_t* var, const char* value)
{
    if (!var || !value || FStrEq(var->string, value))
    {
        chain->callNext(var, value);
        return;
    }
    chain->callNext(var, value);
    KZ_Cvar_DirectSet(var->name, value);
}
/***************************************************************************************************************/
/***************************************************************************************************************/
void KZ_Cvar_DirectSet(const char* const varname, const char* const value)
{
    for (size_t i = 0; i < g_server_cvars_size; ++i)
    {
        if (FStrEq(varname, g_server_cvars[i].name) && !FStrEq(value, g_server_cvars[i].expected_value))
        {
            MF_Log("Illegal cvar value: %s %s", varname, value);
            CVAR_SET_STRING(varname, g_server_cvars[i].expected_value);
            return;
        }
    }
    if (!FStrEq(varname, kz_api_url->name) && !FStrEq(varname, kz_api_token->name))
    {
        return;
    }
    if (!kz_api_url->string || !kz_api_token->string || !kz_api_url->string[0] || !kz_api_token->string[0])
    {
        return;
    }
    if (g_websocket_state.load() > WSState::Uninitialized)
    {
        MF_Log("[WS] API settings change detected. Reconnecting...");
        kz_ws_stop();
    }
    kz_ws_start(kz_api_url->string, kz_api_token->string);
    return;
}
