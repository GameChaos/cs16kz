#include "kz_path_validate.h"

#include <cstring>
#include <filesystem>

bool kz_ws_valid_replay_segment(const char* value)
{
    if (!value || !value[0])
    {
        return false;
    }
    if (strcmp(value, ".") == 0 || strcmp(value, "..") == 0)
    {
        return false;
    }
    for (const char* p = value; *p; ++p)
    {
        const char c = *p;
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.'))
        {
            return false;
        }
    }
    return true;
}

bool kz_ws_valid_steam_authid(const char* value)
{
    if (!value || strncmp(value, "STEAM_", 6) != 0)
    {
        return false;
    }

    int colons = 0;
    for (const char* p = value; *p; ++p)
    {
        const char c = *p;
        if (c == ':')
        {
            ++colons;
            continue;
        }
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'))
        {
            return false;
        }
    }

    return colons == 2;
}

bool kz_ws_valid_replays_relative_path(const char* relative)
{
    if (!relative || !relative[0])
    {
        return false;
    }

    const std::filesystem::path rel(relative);
    if (rel.is_absolute() || rel.has_root_name() || rel.has_root_directory())
    {
        return false;
    }

    for (const auto& part : rel)
    {
        if (part.empty() || part == "." || part == "..")
        {
            return false;
        }
        if (!kz_ws_valid_replay_segment(part.string().c_str()))
        {
            return false;
        }
    }

    return true;
}
