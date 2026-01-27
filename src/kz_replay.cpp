#include "amxxmodule.h"

#include "pdata.h"
#include "kz_ws.h"
#include "kz_util.h"
#include "kz_cvars.h"
#include "kz_replay.h"
#include "kz_storage.h"

#include <condition_variable>
#include <filesystem>

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <arpa/inet.h>
#endif 

krp_header g_header;
krp_packet g_current_frame[33];
std::atomic<bool> g_krp_running;

kz::queue<std::string> g_replay_writer_log(64);
kz::queue<std::string> g_replay_upload_log(64);

kz::queue<krp_packet> g_replay_writer_queue(4096); // (32 players * 100 fps each = 3096 + some additonal room)
kz::queue<ws_upload_replay> g_replay_upload_queue(64);

std::mutex g_replay_writer_mtx;
std::mutex g_replay_upload_mtx;
std::condition_variable g_replay_writer_cv;
std::condition_variable g_replay_upload_cv;

static std::thread g_replay_writer_thread;
static std::thread g_replay_upload_thread;
static void kz_rp_writer_thread(void);
static void kz_rp_upload_thread(void);

extern std::filesystem::path g_data_dir;

#define CHUNK_SIZE (64*1024) // 64KB
#define FRAME_OFFSET(field) (offsetof(krp_packet, data) + offsetof(krp_frame, field))

static const struct { 
    uint64_t bit;
    size_t offset;
    size_t size;
} g_delta_fields[] = {
    { BIT_CMD_LERP_MSEC,        FRAME_OFFSET(cmd.lerp_msec),         sizeof(krp_frame::cmd.lerp_msec) },
    { BIT_CMD_MSEC,             FRAME_OFFSET(cmd.msec),              sizeof(krp_frame::cmd.msec) },
    { BIT_CMD_VIEWANGLES,       FRAME_OFFSET(cmd.viewangles),        sizeof(krp_frame::cmd.viewangles) },
    { BIT_CMD_FORWARDMOVE,      FRAME_OFFSET(cmd.forwardmove),       sizeof(krp_frame::cmd.forwardmove) },
    { BIT_CMD_SIDEMOVE,         FRAME_OFFSET(cmd.sidemove),          sizeof(krp_frame::cmd.sidemove) },
    { BIT_CMD_UPMOVE,           FRAME_OFFSET(cmd.upmove),            sizeof(krp_frame::cmd.upmove) },
    { BIT_CMD_LIGHTLEVEL,       FRAME_OFFSET(cmd.lightlevel),        sizeof(krp_frame::cmd.lightlevel) },
    { BIT_CMD_BUTTONS,          FRAME_OFFSET(cmd.buttons),           sizeof(krp_frame::cmd.buttons) },
    { BIT_CMD_IMPULSE,          FRAME_OFFSET(cmd.impulse),           sizeof(krp_frame::cmd.impulse) },
    { BIT_CMD_WEAPONSELECT,     FRAME_OFFSET(cmd.weaponselect),      sizeof(krp_frame::cmd.weaponselect) },
    { BIT_CMD_IMPACT_INDEX,     FRAME_OFFSET(cmd.impact_index),      sizeof(krp_frame::cmd.impact_index) },
    { BIT_CMD_IMPACT_POSITION,  FRAME_OFFSET(cmd.impact_position),   sizeof(krp_frame::cmd.impact_position) },

    { BIT_VARS_ORIGIN,          FRAME_OFFSET(vars.origin),           sizeof(krp_frame::vars.origin) },
    { BIT_VARS_VELOCITY,        FRAME_OFFSET(vars.velocity),         sizeof(krp_frame::vars.velocity) },
    { BIT_VARS_V_ANGLE,         FRAME_OFFSET(vars.v_angle),          sizeof(krp_frame::vars.v_angle) },
    { BIT_VARS_FIXANGLE,        FRAME_OFFSET(vars.fixangle),         sizeof(krp_frame::vars.fixangle) },
    { BIT_VARS_MOVETYPE,        FRAME_OFFSET(vars.movetype),         sizeof(krp_frame::vars.movetype) },
    { BIT_VARS_FLAGS,           FRAME_OFFSET(vars.flags),            sizeof(krp_frame::vars.flags) },
    { BIT_VARS_BUTTON,          FRAME_OFFSET(vars.button),           sizeof(krp_frame::vars.button) },
    { BIT_VARS_OLDBUTTON,       FRAME_OFFSET(vars.oldbuttons),       sizeof(krp_frame::vars.oldbuttons) },
    { BIT_VARS_FUSER2,          FRAME_OFFSET(vars.fuser2),           sizeof(krp_frame::vars.fuser2) }
};

void kz_rp_run_started(int id) 
{
    krp_packet item = {0};
    item.player_index = id;
    item.type = KRP_SIGNAL_START;
    
    krp_signal* sig = reinterpret_cast<krp_signal*>(item.data);
    sig->ts = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    snprintf(sig->steamid, sizeof(sig->steamid), "%s", GETPLAYERAUTHID(edictByIndex(id)));
    snprintf(sig->nickname, sizeof(sig->nickname), "%s", MF_GetPlayerName(id));

    remove_substring(sig->steamid, "STEAM_");
    remove_substring(sig->steamid, ":");
    remove_substring(sig->steamid, ":");

    g_current_frame[id].player_index = id;
    if(!g_replay_writer_queue.try_push(item))
    {
        kz_log(nullptr, "[KRP] The queue is full");
    }
}
void kz_rp_run_teleport(int id)
{
    // TODO:...
}
void kz_rp_run_paused(int id)
{
    krp_packet item = {0};
    item.player_index = id;
    item.type = KRP_SIGNAL_PAUSE;

    if (!g_replay_writer_queue.try_push(item))
    {
        kz_log(nullptr, "[KRP] The queue is full");
    }
}
void kz_rp_run_unpaused(int id)
{
    krp_packet item = {0};
    item.player_index = id;
    item.type = KRP_SIGNAL_UNPAUSE;

    krp_signal* sig = reinterpret_cast<krp_signal*>(item.data);
    snprintf(sig->steamid, sizeof(sig->steamid), "%s", GETPLAYERAUTHID(edictByIndex(id)));
    snprintf(sig->nickname, sizeof(sig->nickname), "%s", MF_GetPlayerName(id));

    remove_substring(sig->steamid, "STEAM_");
    remove_substring(sig->steamid, ":");
    remove_substring(sig->steamid, ":");

    g_current_frame[id].player_index = id;
    if (!g_replay_writer_queue.try_push(item))
    {
        kz_log(nullptr, "[KRP] The queue is full");
    }
}
void kz_rp_run_rejected(int id, bool delete_file)
{
    krp_packet item = {0};
    item.player_index = id;
    item.type = KRP_SIGNAL_REJECT;

    krp_signal* sig = reinterpret_cast<krp_signal*>(item.data);
    sig->delete_file = true;

    if (!g_replay_writer_queue.try_push(item))
    {
        kz_log(nullptr, "[KRP] The queue is full");
    }
}

void kz_rp_run_finished(int id, float time)
{
    krp_packet item = {0};
    item.player_index = id;
    item.type = KRP_SIGNAL_FINISH;

    krp_signal* sig = reinterpret_cast<krp_signal*>(item.data);
    sig->time = time;
    snprintf(sig->steamid, sizeof(sig->steamid), "%s", GETPLAYERAUTHID(edictByIndex(id)));
    snprintf(sig->nickname, sizeof(sig->nickname), "%s", MF_GetPlayerName(id));

    remove_substring(sig->steamid, "STEAM_");
    remove_substring(sig->steamid, ":");
    remove_substring(sig->steamid, ":");

    if (!g_replay_writer_queue.try_push(item))
    {
        kz_log(nullptr, "[KRP] The queue is full");
    }
}
/***************************************************************************************************************/
/***************************************************************************************************************/
void kz_rp_init(void)
{
    kz_log_addq(&g_replay_writer_log);
    kz_log_addq(&g_replay_upload_log);
    g_krp_running.store(true);

    g_replay_writer_thread = std::thread(kz_rp_writer_thread);
    g_replay_upload_thread = std::thread(kz_rp_upload_thread);
}
void kz_rp_update_header(void)
{
    char szIP[16];
    char szPort[16];
    const char* addr = CVAR_GET_STRING("net_address");
    split_net_address(addr, szIP, sizeof(szIP), szPort, sizeof(szPort));

    g_header.magic          = 0x4B52502146494C45;
    g_header.version        = 0;
    g_header.server_ip      = inet_addr(szIP);
    g_header.server_port    = static_cast<uint16_t>(atoi(szPort));

    g_header.map.checksum = get_map_crc32(STRING(gpGlobals->mapname));
    snprintf(g_header.map.name, sizeof(g_header.map.name), "%s", STRING(gpGlobals->mapname));

    std::filesystem::path dir = g_data_dir / "replays" / g_header.map.name;
    if (!std::filesystem::exists(dir))
    {
        std::error_code ec;
        if (std::filesystem::create_directories(dir, ec))
        {
            kz_log(nullptr, "Directory created: %s", dir.c_str());
        }
        else
        {
            kz_log(nullptr, "Failed to create directory (%s): %s", dir.c_str(), ec.message().c_str());
            return;
        }
    }
}
void kz_rp_uninit()
{
    g_krp_running.store(false);

    g_replay_writer_thread.join();
    g_replay_upload_thread.join();
}
void kz_rp_set_cmd(int id, const usercmd_t* cmd)
{
    krp_frame* frame = reinterpret_cast<krp_frame*>(g_current_frame[id].data);
    memcpy(&frame->cmd, cmd, sizeof(frame->cmd));
}
void kz_rp_set_vars(int id, const entvars_t* vars)
{
    krp_frame* frame = reinterpret_cast<krp_frame*>(g_current_frame[id].data);
    frame->vars.origin        = vars->origin;
    frame->vars.velocity      = vars->velocity;
    frame->vars.v_angle       = vars->v_angle;
    frame->vars.fixangle      = vars->fixangle;
    frame->vars.movetype      = vars->movetype;
    frame->vars.flags         = vars->flags;
    frame->vars.button        = vars->button;
    frame->vars.oldbuttons    = vars->oldbuttons;
}
void kz_rp_write_frame(int id)
{
    g_current_frame[id].player_index = id;
    g_current_frame[id].type = KRP_SIGNAL_FRAME;
    if (!g_replay_writer_queue.try_push(g_current_frame[id]))
    {
        assert(!"This is not supposed to happend.");
    }
    else
    {
        g_replay_writer_cv.notify_one();
    }
}
void kz_rp_upload_async(ws_upload_replay upr)
{
    if (!g_replay_upload_queue.try_push(upr))
    {
        kz_log(nullptr, "[KRP] The queue is full");
    }
    else
    {
        g_replay_upload_cv.notify_one();
    }
}
/***************************************************************************************************************/
/***************************************************************************************************************/
static uint64_t kz_rp_timestamp_from_header(FILE* fp)
{
    if (!fp)
    {
        return 0;
    }

    long current_pos = ftell(fp);
    size_t ts_offset = offsetof(krp_header, timestamp);

    uint64_t ts = 0;
    if (fseek(fp, ts_offset, SEEK_SET) == 0)
    {
        fread(&ts, sizeof(uint64_t), 1, fp);
    }
    fseek(fp, current_pos, SEEK_SET);
    return ts;
}
static void kz_rp_write_event(FILE* fp, krp_packet* curr)
{
    uint64_t mask = BIT_EVENT;
    fwrite(&mask, sizeof(mask), 1, fp);
    fflush(fp);
}
static void kz_rp_write_keyframe(FILE* fp, krp_packet* curr)
{
    krp_frame* frame = reinterpret_cast<krp_frame*>(curr->data);

    uint64_t mask = BIT_KEYFRAME;
    fwrite(&mask, sizeof(mask), 1, fp);
    fwrite(&frame->cmd, sizeof(frame->cmd), 1, fp);
    fwrite(&frame->vars, sizeof(frame->vars), 1, fp);
    fflush(fp);
}
static void kz_rp_write_delta(FILE* fp, krp_packet* curr, krp_packet* last)
{
    uint64_t mask = 0ULL;
    size_t offset = sizeof(mask);
    uint8_t buffer[sizeof(*curr) + offset];

    uint8_t* pCurr = (uint8_t*)curr;
    uint8_t* pLast = (uint8_t*)last;

    for (const auto& field : g_delta_fields)
    {
        if (memcmp(pCurr + field.offset, pLast + field.offset, field.size) != 0)
        {
            mask |= field.bit;
            memcpy(buffer + offset, pCurr + field.offset, field.size);
            offset += field.size;
        }
    }

    memcpy(buffer, &mask, sizeof(mask));
    fwrite(buffer, offset, 1, fp);
}
static void kz_rp_writer_thread(void)
{
    static FILE* s_fd[33];
    static krp_packet s_last[33];
    static size_t s_counter[33];

    static char s_filepath[33][255];

    kz_storage_init();
    while (g_krp_running.load() || !g_replay_writer_queue.empty())
    {
        while (krp_packet* s_curr = g_replay_writer_queue.front())
        {
            int id = s_curr->player_index;
            switch (s_curr->type)
            {
                case KRP_SIGNAL_START:
                {

                    if(s_fd[id])
                    {
                        fclose(s_fd[id]);
                        s_fd[id] = nullptr;

                        kz_log(&g_replay_writer_log, "[KRP] run_start: closing active file descriptor for player (%d)", id);
                    }

                    krp_signal* sig     = reinterpret_cast<krp_signal*>(s_curr->data);
                    krp_header header   = g_header;
                    const char* mapname = g_header.map.name;

                    header.timestamp = sig->ts;

                    std::filesystem::path path = g_data_dir / "replays" / mapname / sig->steamid;
                    snprintf(s_filepath[id], sizeof(s_filepath[0]), "%s.tmp", path.c_str());
                    snprintf(header.player.steamid, sizeof(header.player.steamid), "%s", sig->steamid);

                    s_fd[id] = fopen(s_filepath[id], "wb+");
                    if (s_fd[id])
                    {
                        fwrite(&header, sizeof(header), 1, s_fd[id]);
                    }
                    else
                    {
                        kz_log(&g_replay_writer_log, "[KRP] ERROR: Could not create %s (%s)", s_filepath[id], strerror(errno));
                    }
                    memset(&s_last[id], 0, sizeof(s_last[0]));
                    s_counter[id] = 0;
                    break;
                }
                case KRP_SIGNAL_TELEPORT:
                {
                    // TODO:...
                    break;
                }
                case KRP_SIGNAL_FRAME:
                {
                    if (s_fd[id])
                    {
                        // TODO: add zstd for ultra compression ??
                        // TODO: write in chunks, less io
                        if (!s_counter[id])
                        {
                            // We write a full frame (no delta) when the run just started or got unpaused (savepos)
                            kz_rp_write_keyframe(s_fd[id], s_curr);
                        }
                        else
                        {
                            kz_rp_write_delta(s_fd[id], s_curr, &s_last[id]);
                        }
                        memcpy(&s_last[id], s_curr, sizeof(s_last[0]));
                        s_counter[id]++;
                    }
                    break;
                }
                case KRP_SIGNAL_PAUSE:
                {
                    if (s_fd[id])
                    {
                        fclose(s_fd[id]);
                        s_fd[id] = nullptr;
                    }
                    else
                    {
                        kz_log(&g_replay_writer_log, "[KRP] run_pause: no file descriptor for player (%d)", id);
                    }
                    break;
                }
                case KRP_SIGNAL_UNPAUSE:
                {
                    krp_signal* sig     = reinterpret_cast<krp_signal*>(s_curr->data);
                    const char* mapname = g_header.map.name;

                    std::filesystem::path path = g_data_dir / "replays" / mapname / sig->steamid;
                    snprintf(s_filepath[id], sizeof(s_filepath[0]), "%s.tmp", path.c_str());

                    if (s_fd[id])
                    {
                        fclose(s_fd[id]);
                        s_fd[id] = nullptr;

                        kz_log(&g_replay_writer_log, "[KRP] run_unpause: closing active file descriptor for player (%d)", id);
                    }
                    if (!s_fd[id])
                    {
                        s_fd[id] = fopen(s_filepath[id], "ab+");
                        s_counter[id] = 0;
                    }
                    if (!s_fd[id])
                    {
                        kz_log(&g_replay_writer_log, "[KRP] ERROR: Could not open %s (%s)", s_filepath[id], strerror(errno));
                    }
                    break;
                }
                case KRP_SIGNAL_REJECT:
                {
                    if (s_fd[id])
                    {
                        fclose(s_fd[id]);
                        s_fd[id] = nullptr;

                        krp_signal* sig = reinterpret_cast<krp_signal*>(s_curr->data);
                        if(sig->delete_file)
                        {
                            unlink(s_filepath[id]);
                        }
                    }
                    else
                    {
                        kz_log(&g_replay_writer_log, "[KRP] run_reject: no file descriptor for player (%d)", id);
                    }
                    break;
                }
                case KRP_SIGNAL_FINISH:
                {
                    if (s_fd[id])
                    {
                        char new_path[255];
                        krp_signal* sig     = reinterpret_cast<krp_signal*>(s_curr->data);
                        const char* mapname = g_header.map.name;

                        char uid_str[32];
                        char ts_str[16];
                        to_base36(kz_rp_timestamp_from_header(s_fd[id]), ts_str, sizeof(ts_str));
                        snprintf(uid_str, sizeof(uid_str), "%s_%s", sig->steamid, ts_str);

                        std::filesystem::path npath = g_data_dir / "replays" / mapname / uid_str;
                        snprintf(new_path, sizeof(new_path), "%s.krp_c", npath.c_str());

                        fclose(s_fd[id]);
                        s_fd[id] = nullptr;

                        if (rename(s_filepath[id], new_path) == 0)
                        {
                            kz_storage_save(uid_str, kz_storage_get_next_id(StorageTable::replay_up_queue), StorageTable::replay_up_queue);
                            kz_log(&g_replay_writer_log, "[KRP] Saved replay: %s.krp_c", uid_str);
                        }
                        else
                        {
                            kz_log(&g_replay_writer_log, "[KRP] ERROR: Rename failed (%s)", strerror(errno));
                            break;
                        }

                        JSON_Value* data_val = json_value_init_object();
                        JSON_Object* data_obj = json_value_get_object(data_val);

                        char steamid[35];
                        snprintf(steamid, sizeof(steamid), "STEAM_%c:%c:%s", sig->steamid[0], sig->steamid[1], sig->steamid + 2);

                        json_object_dotset_string(data_obj, "player.nickname", sig->nickname);
                        json_object_dotset_string(data_obj, "player.steamid", steamid);
                        json_object_dotset_string(data_obj, "map.name", mapname);
                        json_object_dotset_number(data_obj, "map.checksum", g_header.map.checksum);
                        json_object_dotset_number(data_obj, "run.time", sig->time);
                        json_object_dotset_number(data_obj, "run.checkpoints", 0);
                        json_object_dotset_number(data_obj, "run.gochecks", 0);
                        json_object_dotset_string(data_obj, "local_uid", uid_str);

                        std::string message;
                        uint64_t msg_id = kz_storage_get_next_id(StorageTable::outgoing_queue);
                        kz_ws_build_msg(WSMessageType::add_record, data_val, message, msg_id);
                        kz_storage_save(message, msg_id, StorageTable::outgoing_queue);
                        kz_ws_send_msg(message, msg_id);
                    }
                    else
                    {
                        kz_log(&g_replay_writer_log, "[KRP] run_finish: no file descriptor for player (%d)", id);
                    }
                    break;
                }
            }
            g_replay_writer_queue.pop();
        }
        std::unique_lock<std::mutex> lock(g_replay_writer_mtx);
        g_replay_writer_cv.wait(lock, []{ return (!g_replay_writer_queue.empty() || !g_krp_running.load()); });
    }
    for (int i = 0; i < 33; i++)
    {
        if (s_fd[i])
        {
            fclose(s_fd[i]);
            s_fd[i] = nullptr;
        }
    }
    kz_storage_uninit();
}
static void kz_rp_upload_thread(void)
{
    while (g_krp_running.load() || !g_replay_upload_queue.empty())
    {
        while (ws_upload_replay* item = g_replay_upload_queue.front())
        {
            FILE* fp = fopen(item->filepath, "rb");
            if (!fp)
            {
                kz_log(&g_replay_upload_log, "[Upload] fopen failure:", strerror(errno));
                g_replay_upload_queue.pop();
                continue;
            }

            const size_t max_data_per_chunk = (CHUNK_SIZE - sizeof(ws_upload_chunk_header));
            fseek(fp, 0, SEEK_END);
            uint32_t total_chunks = (ftell(fp) + (max_data_per_chunk - 1)) / max_data_per_chunk;
            rewind(fp);

            if (kz_api_log_upload->value > 0.0f)
            {
                kz_log(&g_replay_upload_log, "[Upload] Starting: (rec_id: %llu) - (%u chunks)", item->rec_id, total_chunks);
            }

            ws_upload_chunk_header* header = nullptr;

            char buffer[CHUNK_SIZE];
            char* data_ptr = buffer + sizeof(ws_upload_chunk_header);
            size_t bytes = 0;

            auto last_log_time = std::chrono::steady_clock::now();
            for (uint32_t i = 0; i < total_chunks; ++i)
            {
                bytes = fread(data_ptr, 1, max_data_per_chunk, fp);
                if (bytes > 0)
                {
                    header = reinterpret_cast<ws_upload_chunk_header*>(buffer);
                    memset(header, 0, sizeof(*header));

                    header->rec_id         = item->rec_id;
                    header->chunk_index    = i;
                    header->chunk_checksum = UTIL_CRC32(data_ptr, bytes);
                    memcpy(header->local_uid, item->local_uid, sizeof(header->local_uid));

                    g_websocket.sendBinary(std::string(buffer, sizeof(*header) + bytes));
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                if (kz_api_log_upload->value > 0.0f)
                {
                    auto now     = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_log_time).count();
                    if (elapsed > 1 || i == (total_chunks - 1))
                    {
                        float p = (static_cast<float>(i + 1) / static_cast<float>(total_chunks)) * 100.0f;
                        kz_log(&g_replay_upload_log, "[Upload] (rec_id: %llu): %u/%u chunks (%0.1f%%) complete.", item->rec_id, (i + 1), total_chunks, p);
                        last_log_time = now;
                    }
                }
            }
            g_replay_upload_queue.pop();
            fclose(fp);
        }
        {
            std::unique_lock<std::mutex> lock(g_replay_upload_mtx);
            g_replay_upload_cv.wait(lock, []{ return (!g_replay_upload_queue.empty() || !g_krp_running.load()); });
        }
    }
}
