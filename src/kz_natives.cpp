#include "amxxmodule.h"

#include "pdata.h"
#include "kz_ws.h"
#include "kz_storage.h"
#include "kz_natives.h"

/* native kz_api_add_record(id, Float:seconds, checkpoints, gochecks, KZWeapons:weapon, data[], dataSize); */
static cell AMX_NATIVE_CALL kz_api_add_record(AMX* amx, cell* params)
{
    int num_params = (params[0] / sizeof(cell));
    if (num_params < 5)
    {
        MF_LogError(amx, AMX_ERR_NATIVE, "Expected atleast 5 params, got %d", num_params);
        return 0;
    }

    int id = params[1];
    if (id < 0 || id > gpGlobals->maxClients)
    {
        MF_LogError(amx, AMX_ERR_NATIVE, "Invalid player index %d", id);
        return 0;
    }
    if (!MF_IsPlayerIngame(id))
    {
        MF_LogError(amx, AMX_ERR_NATIVE, "Player is not in-game");
        return 0;
    }
    if (MF_IsPlayerBot(id))
    {
        MF_LogError(amx, AMX_ERR_NATIVE, "Bots can't make records !");
        return 0;
    }
    if (!MF_IsPlayerAlive(id))
    {
        MF_LogError(amx, AMX_ERR_NATIVE, "Player is not alive");
        return 0;
    }

    edict_t* pEntity        = edictByIndex(id);
    const char* nickname    = MF_GetPlayerName(id);
    const char* steamid     = GETPLAYERAUTHID(pEntity);

    REAL seconds            = amx_ctof(params[2]);
    int checkpoints         = params[3];
    int gochecks            = params[4];
    int weapon              = params[5];

    JSON_Value* data_val    = json_value_init_object();
    JSON_Object* data_obj   = json_value_get_object(data_val);

    json_object_dotset_string(data_obj, "player.nickname", nickname);
    json_object_dotset_string(data_obj, "player.steamid", steamid);
    
    json_object_dotset_string(data_obj, "run.map.name", STRING(gpGlobals->mapname));
    json_object_dotset_number(data_obj, "run.time",     seconds);
    json_object_dotset_number(data_obj, "run.cps",      checkpoints);
    json_object_dotset_number(data_obj, "run.gcs",      gochecks);
    json_object_dotset_number(data_obj, "run.weapon",   weapon);

    std::string message;
    int64_t msg_id = kz_storage_get_next_id();

    g_plugin_callbacks[msg_id] = std::vector<int>();

    if (num_params >= 7)
    {
        int* ptr = MF_GetAmxAddr(amx, params[6]);
        int size = params[7];
        if (ptr && size)
        {
            g_plugin_callbacks[msg_id].assign(ptr, ptr + size);
        }
    }
    kz_ws_build_msg(WSMessageType::add_record, data_val, message, msg_id);
    kz_ws_queue_msg(message, msg_id);
    return 1;
}

/* native kz_api_del_replay(global_record_id); */
static cell AMX_NATIVE_CALL kz_api_del_replay(AMX* amx, cell* params)
{
    return 0;
}

/* native kz_api_add_replay(global_record_id, filepath[]); */
static cell AMX_NATIVE_CALL kz_api_add_replay(AMX* amx, cell* params)
{
    return 0;
}

/* native kz_api_get_replay(global_record_id, filepath[]); */
static cell AMX_NATIVE_CALL kz_api_get_replay(AMX* amx, cell* params)
{
    return 0;
}


AMX_NATIVE_INFO kz_api_natives[] =
{
    {"kz_api_add_record", kz_api_add_record},
    {"kz_api_add_replay", kz_api_add_replay},
    {"kz_api_get_replay", kz_api_get_replay},
    {NULL, NULL},
};

int fwd_on_map_loaded = -1;
int fwd_on_record_added = -1;
int fwd_on_replay_uploaded = -1;
int fwd_on_replay_downloaded = -1;

std::map<int64_t, std::vector<int>> g_plugin_callbacks;

void kz_api_add_forwards(void)
{
    int fwd = MF_RegisterForward("__kz_global_api_version_check", ET_IGNORE, FP_CELL, FP_CELL, FP_DONE);
    MF_ExecuteForward(fwd, MODULE_VERSION_MAJOR, MODULE_VERSION_MINOR);

    fwd_on_map_loaded        = MF_RegisterForward("kz_api_on_map_loaded", ET_IGNORE, FP_CELL, FP_CELL, FP_CELL, FP_DONE);
    fwd_on_record_added      = MF_RegisterForward("kz_api_on_record_added", ET_IGNORE, FP_CELL, FP_ARRAY, FP_CELL, FP_DONE);
    fwd_on_replay_uploaded   = MF_RegisterForward("kz_api_on_replay_uploaded", ET_IGNORE, FP_CELL, FP_DONE);
    fwd_on_replay_downloaded = MF_RegisterForward("kz_api_on_replay_downloaded", ET_IGNORE, FP_CELL, FP_STRING, FP_DONE);
}
void kz_api_add_natives(void)
{
    MF_AddNatives(kz_api_natives);
}
