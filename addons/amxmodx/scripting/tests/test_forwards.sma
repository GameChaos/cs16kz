#include <amxmodx>
#include <amxmisc>
#include <cskz_api>

public plugin_init()
{
	cskz_api_init();

	register_concmd("cskz_api_post_bans", "cmd_post_bans");
	register_concmd("cskz_api_post_jumpstats", "cmd_post_jumpstats");
	register_concmd("cskz_api_post_records", "cmd_post_records", _, "<s:nickname> <s:steamid> <s:route> <s:weapon> <f:time> <i:checkpoints> <i:gochecks> <i:airaccel> <i:date>");
	register_concmd("cskz_api_post_records_replay", "cmd_post_records_replay");
}
public plugin_end()
{
	cskz_api_cleanup();
}
public cmd_post_bans(id)
{

}
public cmd_post_jumpstats(id)
{

}
public cmd_post_records(id, level, cid)
{
	if(!cmd_access(id, level, cid, 10))
	{
		return 1;
	}

	new aRecord[recordData], szArgs[12];
	read_argv(1, aRecord[rd_nickname], MAX_NAME_LENGTH - 1);
	read_argv(2, aRecord[rd_steamid], MAX_AUTHID_LENGTH - 1);
	read_argv(3, aRecord[rd_route], charsmax(aRecord[rd_route]));
	read_argv(8, aRecord[rd_weapon], charsmax(aRecord[rd_weapon]));

	read_argv(4, szArgs, charsmax(szArgs)); aRecord[rd_time] = str_to_float(szArgs);
	read_argv(5, szArgs, charsmax(szArgs)); aRecord[rd_checkpoints] = str_to_num(szArgs);
	read_argv(6, szArgs, charsmax(szArgs)); aRecord[rd_gochecks] = str_to_num(szArgs);
	read_argv(7, szArgs, charsmax(szArgs)); aRecord[rd_airaccel] = str_to_num(szArgs);
	read_argv(9, szArgs, charsmax(szArgs)); aRecord[rd_date] = str_to_num(szArgs);

	cskz_api_post_records(aRecord);
	return 1;
}
public cmd_post_records_replay(id)
{

}