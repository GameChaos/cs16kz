#include "amxxmodule.h"

#include "pdata.h"
#include "kz_base.h"
#include "kz_util.h"
#include "kz_mpbhop.h"

#include <string>
#include <unordered_map>
#include <cstring>
#include <cstdio>

#define SF_BUTTON_DONTMOVE     1
#define SF_BUTTON_TOGGLE       32
#define SF_BUTTON_SPARK_IF_OFF 64
#define SF_BUTTON_TOUCH_ONLY   256

enum
{
    MPBHOP_CLASS_FUNC_DOOR = 0,
    MPBHOP_CLASS_FUNC_WALL_TOGGLE,
    MPBHOP_CLASS_FUNC_BUTTON,
    MPBHOP_CLASS_TRIGGER_MULTIPLE,
    MPBHOP_CLASS_COUNT,
};

static int   g_block_touch[33];
static bool  g_on_frozen_block[33];
static float g_first_touch[33];
static int   g_onground;
static int   g_teleported;

static Vector g_origin[33];
static Vector g_angles[33];
static float  g_gravity[33];

static int g_present_class;
static int g_blocks[128];
static int g_blocks_by_plugin[128];
static std::unordered_map<std::string, int> g_blocks_class;

static int  set_class_doors();
static int  set_class_buttons();
static int  set_class_wall_toggle();
static int  set_class_multiple();
/***************************************************************************************************************/
/***************************************************************************************************************/
void kz_mpbhop_init()
{
    g_present_class = 0;
    memset(g_blocks, 0, sizeof(g_blocks));
    memset(g_blocks_by_plugin, 0, sizeof(g_blocks_by_plugin));

    g_blocks_class.clear();
    g_blocks_class["func_door"]         = MPBHOP_CLASS_FUNC_DOOR;
    g_blocks_class["func_wall_toggle"]  = MPBHOP_CLASS_FUNC_WALL_TOGGLE;
    g_blocks_class["func_button"]       = MPBHOP_CLASS_FUNC_BUTTON;
    g_blocks_class["trigger_multiple"]  = MPBHOP_CLASS_TRIGGER_MULTIPLE;

    int count = 0;
    count += set_class_doors();
    count += set_class_buttons();
    count += set_class_wall_toggle();
    count += set_class_multiple();

    // TODO: The detection does not catch all possible cases (on some maps, few blocks get skipped :@)
    MF_PrintSrvConsole("[%s] Detected %d bhop blocks.\n", MODULE_LOGTAG, count);
}
void kz_mpbhop_uninit()
{
}
/***************************************************************************************************************/
/***************************************************************************************************************/
void kz_mpbhop_PlayerPreThink(edict_t* pEntity)
{
    const int id = indexOfEdict(pEntity);
    if (!MF_IsPlayerAlive(id))
    {
        return;
    }

    if (flag_test(g_teleported, id))
    {
        flag_clear(g_teleported, id);
        pEntity->v.velocity = Vector(0.0f, 0.0f, 0.0f);
        return;
    }

    const int flags = pEntity->v.flags;

    if (flags & FL_ONGROUND)
    {
        edict_t*  ground    = pEntity->v.groundentity;
        const int ground_id = ground ? (int)indexOfEdict(ground) : 0;

        if (!ground_id || !bitset_test(g_blocks, ground_id))
        {
            g_on_frozen_block[id] = false;

            if (ground_id)
            {
                const Vector& gvel = ground->v.velocity;
                if (gvel.x || gvel.y || gvel.z) // standing on something moving: don't save a spot
                {
                    flag_clear(g_onground, id);
                    return;
                }
            }

            if (flags & FL_DUCKING)
            {
                Vector origin = pEntity->v.origin;
                origin.z += 18.0f;

                TraceResult tr;
                TRACE_HULL(origin, origin, ignore_monsters, human_hull, pEntity, &tr);
                const bool obstructed = tr.fAllSolid || tr.fStartSolid || !tr.fInOpen;

                if (!obstructed) // room to stand un-ducked here: remember it
                {
                    g_origin[id] = origin;
                    flag_set(g_onground, id);
                }
                else
                {
                    flag_clear(g_onground, id);
                    return;
                }
            }
            else
            {
                g_origin[id] = pEntity->v.origin;
                flag_set(g_onground, id);
            }
        }
        else
        {
            g_on_frozen_block[id] = true;
            flag_clear(g_onground, id);
        }
    }
    else
    {
        g_on_frozen_block[id] = false;
        if (flag_test(g_onground, id))
        {
            g_angles[id]  = pEntity->v.v_angle;
            g_gravity[id] = pEntity->v.gravity;
            flag_clear(g_onground, id);
        }
    }
}
bool kz_mpbhop_DispatchTouch(edict_t* pTouched, edict_t* pToucher)
{
    const int touched = indexOfEdict(pTouched);
    if (!bitset_test(g_blocks, touched))
    {
        return false;
    }

    const int toucher = indexOfEdict(pToucher);
    if (is_player(toucher))
    {
        if (!MF_IsPlayerAlive(toucher))
        {
            return true;
        }
        g_on_frozen_block[toucher] = true;
    }
    else
    {
        return true;
    }
    if (touched != (int)indexOfEdict(pToucher->v.groundentity))
    {
        return true;
    }

    if (g_block_touch[toucher] != touched)
    {
        g_block_touch[toucher] = touched;
        g_first_touch[toucher] = gpGlobals->time;
        return true;
    }

    const float time = gpGlobals->time;
    if (time - g_first_touch[toucher] > 0.25f)
    {
        if (time - g_first_touch[toucher] > 0.92f)
        {
            g_first_touch[toucher] = time;
            return true;
        }

        flag_set(g_teleported, toucher);
        kz_teleport_player(pToucher, g_origin[toucher], &g_angles[toucher]);
        pToucher->v.gravity = g_gravity[toucher];
    }
    return true;
}
/***************************************************************************************************************/
/***************************************************************************************************************/
void kz_mpbhop_ClientPutInServer(edict_t* pEntity)
{
    const int id = indexOfEdict(pEntity);
    if (!is_player(id))
    {
        return;
    }
    flag_clear(g_teleported, id);
    g_on_frozen_block[id] = false;
    g_block_touch[id]     = 0;
    g_first_touch[id]     = 0.0f;
}
void kz_mpbhop_ClientDisconnected(edict_t* pEntity)
{
    const int id = indexOfEdict(pEntity);
    if (!is_player(id))
    {
        return;
    }
    flag_clear(g_teleported, id);
    g_on_frozen_block[id] = false;
}
/***************************************************************************************************************/
/***************************************************************************************************************/
void kz_mpbhop_set_teleport(int id, const Vector& origin, const Vector& v_angle)
{
    if (is_player(id))
    {
        g_origin[id] = origin;
        g_angles[id] = v_angle;
    }
}
bool kz_mpbhop_on_frozen_block(int id)
{
    return (is_player(id) && g_on_frozen_block[id]);
}
/***************************************************************************************************************/
/***************************************************************************************************************/
static int set_class_doors()
{
    char noise[32];
    int count = 0;
    int index = 0;

    edict_t* pEntity = INDEXENT(0);
    while (!FNullEnt(pEntity = FIND_ENTITY_BY_CLASSNAME(pEntity, "func_door")))
    {
        if (pEntity->v.dmg) // damaging door: mute it, skip
        {
            pEntity->v.noise1 = ALLOC_STRING("common/null.wav");
            pEntity->v.noise2 = ALLOC_STRING("common/null.wav");
            pEntity->v.noise3 = ALLOC_STRING("common/null.wav");
            continue;
        }
        if (pEntity->v.movedir.z > 0.0f) // moves upward: skip
        {
            continue;
        }
        if (pEntity->v.size.x < 24.0f && pEntity->v.size.y > 50.0f) // too thin / real door
        {
            continue;
        }
        if (pEntity->v.size.y < 24.0f && pEntity->v.size.x > 50.0f) // too thin / real door
        {
            continue;
        }
        if (pEntity->v.noise1)
        {
            noise[0] = '\0';
            snprintf(noise, sizeof(noise), "%s", STRING(pEntity->v.noise1));
            if (noise[0] && strcmp(noise, "common/null.wav") != 0)
            {
                continue;
            }
        }
        if (pEntity->v.noise2)
        {
            noise[0] = '\0';
            snprintf(noise, sizeof(noise), "%s", STRING(pEntity->v.noise2));
            if (noise[0] && strcmp(noise, "common/null.wav") != 0)
            {
                continue;
            }
        }
        if (pEntity->v.speed < 80.0f) // too slow
        {
            continue;
        }

        count++;
        g_present_class |= (1 << MPBHOP_CLASS_FUNC_DOOR);

        index = indexOfEdict(pEntity);
        bitset_set(g_blocks_by_plugin, index);
        bitset_set(g_blocks, index);
    }
    return count;
}
static int set_class_buttons()
{
    static const char* start_stop[] = {
        "counter_start", "clockstartbutton", "firsttimerelay", "gogogo", "multi_start",
        "counter_start_button", "startcounter", "counter_off", "clockstop", "clockstopbutton",
        "multi_stop", "stop_counter", "stopcounter",
    };

    std::unordered_map<std::string, bool> skip_buttons;
    for (const char* name : start_stop)
    {
        skip_buttons[name] = true;
    }

    const int touch_mask = SF_BUTTON_DONTMOVE | SF_BUTTON_TOGGLE | SF_BUTTON_TOUCH_ONLY;
    int count = 0;
    int index = 0;

    edict_t* pEntity = INDEXENT(0);
    while (!FNullEnt(pEntity = FIND_ENTITY_BY_CLASSNAME(pEntity, "func_button")))
    {
        if ((pEntity->v.spawnflags & touch_mask) == SF_BUTTON_TOUCH_ONLY)
        {
            const char* target = (pEntity->v.target ? STRING(pEntity->v.target) : "");
            if (!target[0] || skip_buttons.find(target) == skip_buttons.end())
            {
                const char* targetname = (pEntity->v.targetname ? STRING(pEntity->v.targetname) : "");
                if (!targetname[0] || skip_buttons.find(targetname) == skip_buttons.end())
                {
                    count++;
                    g_present_class |= (1 << MPBHOP_CLASS_FUNC_BUTTON);

                    index = indexOfEdict(pEntity);
                    bitset_set(g_blocks_by_plugin, index);
                    bitset_set(g_blocks, index);
                }
            }
        }
        if (pEntity->v.spawnflags & SF_BUTTON_SPARK_IF_OFF)
        {
            pEntity->v.spawnflags = (pEntity->v.spawnflags & ~SF_BUTTON_SPARK_IF_OFF);
        }
    }
    return count;
}
static int set_class_wall_toggle()
{
    int count = 0;
    int index = 0;

    edict_t* pEntity = INDEXENT(0);
    while (!FNullEnt(pEntity = FIND_ENTITY_BY_CLASSNAME(pEntity, "func_wall_toggle")))
    {
        count++;
        g_present_class |= (1 << MPBHOP_CLASS_FUNC_WALL_TOGGLE);

        index = indexOfEdict(pEntity);
        bitset_set(g_blocks_by_plugin, index);
        bitset_set(g_blocks, index);
    }
    return count;
}
static int set_class_multiple()
{
    int index = 0;

    edict_t* pEntity = INDEXENT(0);
    while (!FNullEnt(pEntity = FIND_ENTITY_BY_CLASSNAME(pEntity, "trigger_multiple")))
    {
        const char* target = (pEntity->v.target ? STRING(pEntity->v.target) : "");
        if (!target[0])
        {
            continue;
        }

        edict_t* pTargetEntity = FIND_ENTITY_BY_TARGETNAME(NULL, target);
        if (!FNullEnt(pTargetEntity) && bitset_test(g_blocks, indexOfEdict(pTargetEntity)))
        {
            g_present_class |= (1 << MPBHOP_CLASS_TRIGGER_MULTIPLE);

            index = indexOfEdict(pEntity);
            bitset_set(g_blocks_by_plugin, index);
            bitset_set(g_blocks, index);
        }
    }
    return 0;
}
