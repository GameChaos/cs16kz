#ifndef KZ_REPLAY_UID_H
#define KZ_REPLAY_UID_H

#include <cstdint>
#include <filesystem>
#include <string>

/** Replay filename: <class:1>_<time_ms>_<steamid>_<timestamp>.krpz */
bool kz_replay_parse_uid(const std::string& filename, uint8_t& run_class, uint32_t& time_ms);

/** PRO (0) beats NUB (1); within a class, lower time_ms wins. */
bool kz_replay_uid_is_better(uint8_t cls_a, uint32_t time_a, uint8_t cls_b, uint32_t time_b);

/** Best .krpz in map_dir by class then time; empty if none valid. */
std::filesystem::path kz_replay_find_best_in_dir(const std::filesystem::path& map_dir);

#endif
