#ifndef KZ_CMD_H
#define KZ_CMD_H

typedef void (*kz_cmd_fn)(edict_t* pEntity);

extern void kz_register_client_command(const char* command, kz_cmd_fn callback, bool chat_only);
extern bool kz_cmd_dispatch(edict_t* pEntity);

#endif // KZ_CMD_H
