#ifndef KZ_PATH_VALIDATE_H
#define KZ_PATH_VALIDATE_H

/** Validates a single path segment (map name, local_uid) for replay storage. */
bool kz_ws_valid_replay_segment(const char* value);

/** Validates a Steam auth id (STEAM_X:Y:Z) for safe use in server commands. */
bool kz_ws_valid_steam_authid(const char* value);

/** Validates a relative path under kz_global/replays (each component, no traversal). */
bool kz_ws_valid_replays_relative_path(const char* relative);

#endif
