#include "amxxmodule.h"
#include "resdk/mod_rehlds_api.h"

#include "pdata.h"
#include "kz_util.h"
#include "kz_ws.h"
#include "kz_storage.h"
#include "kz_natives.h"

WSMessageFunc g_callback_table[ectoi(WSMessageType::_MAX)];

void kz_ws_run_tasks(int max_tasks_per_frame)
{
    // TODO: check storage if we need to resent something
    int tasks_done = 0;

    while(g_log_queue.front() && tasks_done < max_tasks_per_frame)
    {
        std::string* message = g_log_queue.front();
        MF_Log("%s", message->c_str());

        g_log_queue.pop();
        tasks_done++;
    }
    while(g_incoming_queue.front() && tasks_done < max_tasks_per_frame)
    {
        JSON_Value* root_val    = *g_incoming_queue.front();
        JSON_Object* root_obj   = json_value_get_object(root_val);
        int index               = json_object_get_number(root_obj, "msg_type");

        if(index <= 0 || index >= ectoi(WSMessageType::_MAX))
        {
            g_callback_table[ectoi(WSMessageType::invalid)](root_obj);
        }
        else
        {
            g_callback_table[index](root_obj);
        }

        json_value_free(root_val);
        g_incoming_queue.pop();
        tasks_done++;
    }
    if(g_websocket_state.load() == WSState::Connected && g_websocket.getReadyState() == ix::ReadyState::Open)
    {
        while(g_outgoing_queue.front() && tasks_done < max_tasks_per_frame)
        {
            std::string* message = g_outgoing_queue.front(); assert(message);
            kz_ws_send_msg(*message, 0);

            g_outgoing_queue.pop();
            tasks_done++;
        }
    }
}
void kz_ws_register(WSMessageType type, WSMessageFunc pfn)
{
    g_callback_table[ectoi(type)] = pfn;
}
void kz_ws_event_client_connect(edict_t* pEntity)
{
    int id = indexOfEdict(pEntity);

    char szIP[16];
    const char* authid = GETPLAYERAUTHID(pEntity);
    UTIL_split_net_address(MF_GetPlayerIP(id), szIP, sizeof(szIP), nullptr, 0);

    JSON_Value* data_val = json_value_init_object();
    JSON_Object* data_obj = json_value_get_object(data_val);

    std::string message;
    int64_t msg_id = kz_storage_get_next_id();

    json_object_set_string(data_obj, "nickname", MF_GetPlayerName(id));
    json_object_set_string(data_obj, "ipaddr", szIP);
    json_object_set_string(data_obj, "steamid", authid);

    kz_ws_build_msg(WSMessageType::client_info, data_val, message, msg_id);
    kz_ws_queue_msg(message, msg_id);
}
void kz_ws_ack_invalid(JSON_Object* obj)
{
    MF_Log("[kz_ws_ack_invalid] Invalid msg_id: %d", json_object_dotget_number(obj, "data.msg_id"));
}
void kz_ws_ack_hello(JSON_Object* obj)
{
    int heartbeat_interval = json_object_dotget_number(obj, "data.heartbeat_interval");
    
    g_websocket.setPingInterval(heartbeat_interval);
    MF_Log("[kz_ws_ack_hello] Heartbeat interval: %d", heartbeat_interval);
}
void kz_ws_ack_map_info(JSON_Object* obj)
{
    const char* mapname = json_object_dotget_string(obj, "data.mapname");
    int type            = json_object_dotget_number(obj, "data.type");
    int length          = json_object_dotget_number(obj, "data.length");
    int difficulty      = json_object_dotget_number(obj, "data.difficulty");

    int64_t msg_id = json_object_dotget_number(obj, "msg_id");
    auto it = g_plugin_callbacks.find(msg_id);

    if(it != g_plugin_callbacks.end())
    {
        MF_ExecuteForward(it->second.fwd, mapname, type, length, difficulty);
        MF_UnregisterSPForward(it->second.fwd);
    }
    else
    {
        MF_Log("[kz_ws_ack_map_info] Failed to find %lld in g_plugin_callbacks", msg_id);
    }
}
void kz_ws_ack_client_info(JSON_Object* obj)
{
    bool is_banned = json_object_dotget_boolean(obj, "data.banned");
    
    if(!is_banned)
    {
        return;
    }

    const char* authid = json_object_dotget_string(obj, "data.steamid");
    if(!authid || !authid[0])
    {
        return;
    }

    edict_t* pEntity = UTIL_find_player_by_authid(authid);
    if(FNullEnt(pEntity))
    {
        return;
    }

    char buff[192];
    const char* banned_by = json_object_dotget_string(obj, "data.by");

    snprintf(buff, sizeof(buff), "kick #%d \"You've been cross-community banned by %s\"\n", GETPLAYERUSERID(pEntity), banned_by);
    SERVER_COMMAND(buff);

    snprintf(buff, sizeof(buff), "banid 2 %s\n", authid);
    SERVER_COMMAND(buff);
}
void kz_ws_ack_add_record(JSON_Object* obj)
{
    int64_t msg_id = json_object_get_number(obj, "msg_id");
    auto it = g_plugin_callbacks.find(msg_id);
    
    if (it != g_plugin_callbacks.end())
    {
        std::vector<int>& plugin_data = it->second.data;
        size_t size = plugin_data.size();
        int global_rec_id = json_object_dotget_number(obj, "data.rec_id");

        MF_ExecuteForward(fwd_on_record_added, global_rec_id, MF_PrepareCellArray((int*)(plugin_data.data()), size), size);
        g_plugin_callbacks.erase(it);
    }
    else
    {
        MF_Log("[WS] Failed to find %lld in g_plugin_callbacks", msg_id);
    }
}
void kz_ws_ack_del_record(JSON_Object* obj)
{
    MF_Log("[kz_ws_ack_del_record]");
}
void kz_ws_ack_add_replay(JSON_Object* obj)
{
    MF_Log("[kz_ws_ack_add_replay]");
}
void kz_ws_ack_get_replay(JSON_Object* obj)
{
    MF_Log("[kz_ws_ack_get_replay]");
}
