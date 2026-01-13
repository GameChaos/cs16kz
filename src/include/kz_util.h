#ifndef KZ_UTIL_H
#define KZ_UTIL_H

extern void     UTIL_split_net_address(const char* addr, char* ip, size_t ip_maxlen, char* port, size_t port_maxlen);
extern edict_t* UTIL_find_player_by_authid(const char* authid);
extern cvar_t*  UTIL_register_cvar(const char* name, const char* value, int flags);

#endif
