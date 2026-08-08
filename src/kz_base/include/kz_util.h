#ifndef KZ_UTIL_H
#define KZ_UTIL_H

inline bool is_player(int index)
{
    return index >= 1 && index <= gpGlobals->maxClients;
}

// Single 32-bit field, one bit per player id 1..32
inline void flag_set(int& field, int id)   { field |=  (1 << (id & 31)); }
inline bool flag_test(int field, int id)   { return (field & (1 << (id & 31))) != 0; }
inline void flag_clear(int& field, int id) { field &= ~(1 << (id & 31)); }

// Int-array bitset, one bit per entity index
inline void bitset_set(int* set, int index)        { set[index >> 5] |=  (1 << (index & 31)); }
inline bool bitset_test(const int* set, int index) { return (set[index >> 5] & (1 << (index & 31))) != 0; }
inline void bitset_clear(int* set, int index)      { set[index >> 5] &= ~(1 << (index & 31)); }

void kz_teleport_player(edict_t* pEntity, const Vector& origin, const Vector* angles);

#endif // KZ_UTIL_H
