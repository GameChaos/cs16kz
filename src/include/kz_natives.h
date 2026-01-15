#ifndef KZ_NATIVES_H
#define KZ_NATIVES_H

extern int fwd_on_map_loaded;
extern int fwd_on_record_added;
extern int fwd_on_replay_uploaded;
extern int fwd_on_replay_downloaded;

extern std::map<int64_t, std::vector<int>> g_plugin_callbacks;

extern void kz_api_add_forwards(void);
extern void kz_api_add_natives(void);
#endif 
