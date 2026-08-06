#include "kz_path_validate.h"

#include <cstring>

static int g_failures = 0;

static void expect(bool cond, const char* msg)
{
    if (!cond)
    {
        ++g_failures;
    }
}

int run_path_validate_tests(void)
{
    g_failures = 0;

    expect(kz_ws_valid_replay_segment("kz_canyon"), "valid map name");
    expect(kz_ws_valid_replay_segment("0_00012345_steam_abc"), "valid local uid");
    expect(kz_ws_valid_replay_segment("map-name_v2.test"), "valid punctuation");

    expect(!kz_ws_valid_replay_segment(nullptr), "null rejected");
    expect(!kz_ws_valid_replay_segment(""), "empty rejected");
    expect(!kz_ws_valid_replay_segment("."), "dot rejected");
    expect(!kz_ws_valid_replay_segment(".."), "dotdot rejected");
    expect(!kz_ws_valid_replay_segment("../etc"), "traversal rejected");
    expect(!kz_ws_valid_replay_segment("map/name"), "slash rejected");
    expect(!kz_ws_valid_replay_segment("map\\name"), "backslash rejected");
    expect(!kz_ws_valid_replay_segment("map name"), "space rejected");

    expect(kz_ws_valid_replays_relative_path("kz_map/0_00012345_steam_abc.krpz"), "map replay path");
    expect(kz_ws_valid_replays_relative_path("0_00012345_steam_abc.krpz"), "flat replay path");
    expect(!kz_ws_valid_replays_relative_path(nullptr), "null relative path");
    expect(!kz_ws_valid_replays_relative_path(""), "empty relative path");
    expect(!kz_ws_valid_replays_relative_path("../secret.krpz"), "parent traversal");
    expect(!kz_ws_valid_replays_relative_path("kz_map/../../secret.krpz"), "nested traversal");
    expect(!kz_ws_valid_replays_relative_path("/etc/passwd.krpz"), "absolute path");

    return g_failures;
}
