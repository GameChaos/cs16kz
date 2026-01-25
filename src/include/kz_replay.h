#ifndef KZ_REPLAY_H
#define KZ_REPLAY_H

#ifndef USERCMD_H
#include "usercmd.h"
#endif

// Bitmask for delta compression
enum : uint64_t 
{
    BIT_CMD_LERP_MSEC       = (1ULL << 0),
    BIT_CMD_MSEC            = (1ULL << 1),
    BIT_CMD_VIEWANGLES      = (1ULL << 2),
    BIT_CMD_FORWARDMOVE     = (1ULL << 3),
    BIT_CMD_SIDEMOVE        = (1ULL << 4),
    BIT_CMD_UPMOVE          = (1ULL << 5),
    BIT_CMD_LIGHTLEVEL      = (1ULL << 6),
    BIT_CMD_BUTTONS         = (1ULL << 7),
    BIT_CMD_IMPULSE         = (1ULL << 8),
    BIT_CMD_WEAPONSELECT    = (1ULL << 9),
    BIT_CMD_IMPACT_INDEX    = (1ULL << 10),
    BIT_CMD_IMPACT_POSITION = (1ULL << 11),

    BIT_VARS_ORIGIN         = (1ULL << 12),
    BIT_VARS_VELOCITY       = (1ULL << 13),
    BIT_VARS_V_ANGLE        = (1ULL << 14),
    BIT_VARS_FIXANGLE       = (1ULL << 15),
    BIT_VARS_MOVETYPE       = (1ULL << 16),
    BIT_VARS_FLAGS          = (1ULL << 17),
    BIT_VARS_BUTTON         = (1ULL << 18),
    BIT_VARS_OLDBUTTON      = (1ULL << 19),
    BIT_VARS_FUSER2         = (1ULL << 20),

    BIT_EVENT               = (1ULL << 62), // events (ex: checkpoint, gocheck..)
    BIT_KEYFRAME            = (1ULL << 63), // no delta
};

enum : uint8_t
{
    KRP_SIGNAL_FRAME,
    KRP_SIGNAL_START,
    KRP_SIGNAL_TELEPORT,
    KRP_SIGNAL_PAUSE,
    KRP_SIGNAL_UNPAUSE,
    KRP_SIGNAL_REJECT,
    KRP_SIGNAL_FINISH,
};

#pragma pack(push, 1)
typedef struct
{
    vec3_t origin;
    vec3_t velocity;
    vec3_t v_angle;
    int32_t fixangle;
    int32_t movetype;
    int32_t flags;
    int32_t button;
    int32_t oldbuttons;
    float fuser2;
} krp_entvars;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct  {
    char steamid[35];
    char nickname[32];
    union {
        bool delete_file;
        float time;
        uint64_t ts;
    };
} krp_signal;
#pragma pack(pop)

typedef usercmd_t krp_usercmd;
#pragma pack(push, 1)
typedef struct  {
    krp_usercmd cmd;
    krp_entvars vars;
} krp_frame;
#pragma pack(pop)


#pragma pack(push, 1)
typedef struct {
    uint8_t type;
    uint8_t player_index;

    // C++ && queue and its bullshit errors when i put a union { } in this struct
    // Just cast this to krp_frame or krp_signal based on ->type
    uint8_t data[sizeof(krp_frame) > sizeof(krp_signal) ? sizeof(krp_frame) : sizeof(krp_signal)];
} krp_packet;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    uint64_t    magic;
    uint64_t    version;

    struct { char name[32]; char steamid[35]; }   player;
    struct { char name[64]; uint32_t checksum; }  map;

    uint64_t    timestamp;
    uint32_t    server_ip;
    uint16_t    server_port;
} krp_header;
#pragma pack(pop)

typedef struct
{
    char        filepath[255];
    char        local_uid[32];
    uint64_t    rec_id;
} ws_upload_replay;

#pragma pack(push, 1)
typedef struct
{
    char        local_uid[32];
    uint64_t    rec_id;
    int32_t     chunk_checksum;
    uint64_t    chunk_index;
} ws_upload_chunk_header;
#pragma pack(pop)

extern void kz_rp_run_started(int id);
extern void kz_rp_run_teleport(int id);
extern void kz_rp_run_paused(int id);
extern void kz_rp_run_unpaused(int id);
extern void kz_rp_run_rejected(int id, bool delete_file);
extern void kz_rp_run_finished(int id, float time);

extern void kz_rp_init(void);
extern void kz_rp_uninit(void);
extern void kz_rp_update_header(void);
extern void kz_rp_set_cmd(int id, const usercmd_t* cmd);
extern void kz_rp_set_vars(int id, const entvars_t* vars);
extern void kz_rp_upload_async(ws_upload_replay upr);
extern void kz_rp_write_frame(int id);
#endif
