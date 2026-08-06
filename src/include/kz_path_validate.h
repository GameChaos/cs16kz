#ifndef KZ_PATH_VALIDATE_H
#define KZ_PATH_VALIDATE_H

/** Validates a single path segment (map name, local_uid) for replay storage. */
bool kz_ws_valid_replay_segment(const char* value);

#endif
