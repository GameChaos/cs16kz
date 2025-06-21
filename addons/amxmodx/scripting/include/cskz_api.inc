#if defined _cskz_api_included
    #endinput
#endif
#define _cskz_api_included

#pragma reqlib cskz_api
#if !defined AMXMODX_NOAUTOLOAD
    #pragma loadlib cskz_api
#endif

#include <cskz_api_const>

enum
{
    APIForward_post_bans,
    APIForward_post_jumpstats,
    APIForward_post_records,
    APIForward_post_records_replay,
}

new __api_forwards[4];
new __ignored;

public cskz_api_init()
{
    new plugin_id;

    if((plugin_id = find_plugin_byfile("cskz_api.amxx")) != -1)
    {
        __api_forwards[APIForward_post_bans]            = CreateOneForward(plugin_id, "cskz_api_post_bans", FP_ARRAY);
        __api_forwards[APIForward_post_jumpstats]       = CreateOneForward(plugin_id, "cskz_api_post_jumpstats", FP_ARRAY);
        __api_forwards[APIForward_post_records]         = CreateOneForward(plugin_id, "cskz_api_post_records", FP_ARRAY);
        __api_forwards[APIForward_post_records_replay]  = CreateOneForward(plugin_id, "cskz_api_post_records_replay", FP_CELL, FP_STRING);
    }
}
public cskz_api_cleanup()
{
    for(new i = 0; i < sizeof(__api_forwards); i++)
    {
        DestroyForward(__api_forwards[i]);
    }
}
stock cskz_api_post_bans(aBan[banData])
{
    ExecuteForward(__api_forwards[APIForward_post_bans], __ignored, PrepareArray(aBan, sizeof(aBan)));
}
stock cskz_api_post_jumpstats(aStats[jumpstatsData])
{
    ExecuteForward(__api_forwards[APIForward_post_jumpstats], __ignored, PrepareArray(aStats, sizeof(aStats)));
}
stock cskz_api_post_records(aRecord[recordData])
{
    ExecuteForward(__api_forwards[APIForward_post_records], __ignored, PrepareArray(aRecord, sizeof(aRecord)))
}
stock cskz_api_post_records_replay(record_id, replay[])
{
    ExecuteForward(__api_forwards[APIForward_post_records_replay], __ignored, record_id, replay);
}