#include "kz_path_validate.h"

#include <cstring>

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
