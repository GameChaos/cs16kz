#include "krp_codec.h"

#include <cstring>

namespace krp
{

error validate_header(const uint8_t* buf, size_t len)
{
    if (!buf || len < sizeof(krp_header))
    {
        return error::truncated;
    }

    krp_header header;
    memcpy(&header, buf, sizeof(header));

    if (header.magic != KRP_MAGIC)
    {
        return error::bad_magic;
    }
    if (header.version != KRP_CURRENT_VERSION)
    {
        return error::bad_version;
    }
    return error::ok;
}

}
