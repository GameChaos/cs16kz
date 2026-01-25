#ifndef KZ_NATIVES_H
#define KZ_NATIVES_H

typedef struct { int fwd; std::vector<int> data; } plugin_callback_data;

extern int fwd_on_record_added;
extern int fwd_on_replay_downloaded;

extern std::map<int64_t, plugin_callback_data> g_plugin_callbacks;

extern void kz_api_add_forwards(void);
extern void kz_api_add_natives(void);
#endif 
