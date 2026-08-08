#ifndef KZ_MPBHOP_H
#define KZ_MPBHOP_H

extern void kz_mpbhop_init();
extern void kz_mpbhop_uninit();

extern void kz_mpbhop_PlayerPreThink(edict_t* pEntity);
extern bool kz_mpbhop_DispatchTouch(edict_t* pTouched, edict_t* pToucher); // caller should MRES_SUPERCEDE on true

extern void kz_mpbhop_ClientPutInServer(edict_t* pEntity);
extern void kz_mpbhop_ClientDisconnected(edict_t* pEntity);

// Sets the teleport origin/position if a player is standing on a bhop block for too long
extern void kz_mpbhop_set_teleport(int id, const Vector& origin, const Vector& v_angle);

// Returns true/false if the player is currently touching a bhop block
extern bool kz_mpbhop_on_frozen_block(int id);
#endif
