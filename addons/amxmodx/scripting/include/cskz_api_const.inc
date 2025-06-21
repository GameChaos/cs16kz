#if defined _cskz_api_const_included
    #endinput
#endif
#define _cskz_api_const_included

enum _:banData
{
}
enum _:jumpstatsData
{
    jsd_type,
    Float:jsd_dist,
    jsd_block,
    jsd_hasblock,
    Float:jsd_edge,
    jsd_hasedge,
    Float:jsd_landedge,
    Float:jsd_veer,
    jsd_fwdrelease,
    Float:jsd_sync,
    Float:jsd_maxspeed,
    Float:jsd_prespeed,
    jsd_overlap,
    jsd_deadair,
    Float:jsd_jofangle,
    Float:jsd_airpath,
    jsd_strafes,
    jsd_airtime,
    jsd_direction,
    jsd_fog,
    jsd_hasfog,
    Float:jsd_height,
    jsd_hasstamina,
    Float:jsd_stamina,
    Float:jsd_loss,
    Float:jsd_potency
}
enum _:recordData
{
    rd_nickname[MAX_NAME_LENGTH],
    rd_steamid[MAX_AUTHID_LENGTH],
    rd_route[12],
    Float:rd_time,
    rd_checkpoints,
    rd_gochecks,
    rd_airaccel,
    rd_weapon[6],
    rd_date
}