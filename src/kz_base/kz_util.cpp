#include "amxxmodule.h"
#include "kz_util.h"

// Lowercase names: VEC_DUCK_HULL_MIN/MAX/VIEW are macros in HLSDK dlls/util.h (and its MAX is z=18,
// whereas mpbhop.inl uses z=32), so we can't reuse those names.

static const Vector VEC_NULL(0.0f, 0.0f, 0.0f);
static const Vector s_duck_mins(-16.0f, -16.0f, -18.0f);
static const Vector s_duck_maxs( 16.0f,  16.0f,  32.0f);
static const Vector s_duck_view(0.0f, 0.0f, 12.0f);

void kz_teleport_player(edict_t* pEntity, const Vector& origin, const Vector* angles)
{
    // TODO: fwd_resetbug
    // Register a Pawn forward & call it (resetbug - reset stats for jump stats plugins)

    int flags = pEntity->v.flags;
    if (flags & FL_BASEVELOCITY)
    {
        flags &= ~FL_BASEVELOCITY;
        pEntity->v.basevelocity = VEC_NULL;
    }

    pEntity->v.velocity = VEC_NULL;
    pEntity->v.flags    = flags | FL_DUCKING;

    SET_SIZE(pEntity, s_duck_mins, s_duck_maxs);
    SET_ORIGIN(pEntity, origin);

    if (angles)
    {
        pEntity->v.v_angle  = *angles;
        pEntity->v.angles   = *angles;
        pEntity->v.fixangle = 1;
    }

    pEntity->v.view_ofs     = s_duck_view;
    pEntity->v.punchangle   = VEC_NULL;
    pEntity->v.fuser2       = 0.0f;
}
