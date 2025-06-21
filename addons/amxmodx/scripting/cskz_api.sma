#include <amxmodx>
#include <easy_http>
#include <cskz_api_const>

enum
{
	ENDPOINT_POST_BANS,
	ENDPOINT_POST_JUMPSTATS,
	ENDPOINT_POST_RECORDS,
	ENDPOINT_POST_RECORDS_REPLAY
}
new const g_szEndpoints[][] =
{
	"https://0.0.0.0/api/v1/bans",
	"https://0.0.0.0/api/v1/jumpstats",
	"https://0.0.0.0/api/v1/records",
	"https://0.0.0.0/api/v1/records/{}/replay"
}

new g_szAPIKey[512];
new g_szHostname[64];
new g_szIP[MAX_IP_LENGTH];
new g_iPort;

new g_szCurrentMap[32];

public plugin_init()
{

}
public plugin_cfg()
{
	get_pcvar_string(register_cvar("cskz_api_key", ""), g_szAPIKey, charsmax(g_szAPIKey));
	format(g_szAPIKey, charsmax(g_szAPIKey), "Bearer: %s", g_szAPIKey);

	get_cvar_string("hostname", g_szHostname, charsmax(g_szHostname));
	get_cvar_string("ip", g_szIP, charsmax(g_szIP));
	g_iPort = get_cvar_num("port");

	server_cmd("exec addons/amxmodx/configs/cskz.cfg");
	get_mapname(g_szCurrentMap, charsmax(g_szCurrentMap));
}
public plugin_natives()
{
	register_library("cskz_api");
}
public ezhttp_post_bans_oncomplete(EzHttpRequest:request_id)
{
}
public ezhttp_post_jumpstats_oncomplete(EzHttpRequest:request_id)
{

}
public ezhttp_post_records_oncomplete(EzHttpRequest:request_id)
{
	if(ezhttp_get_error_code(request_id) != EZH_OK)
	{
		new szError[128];
		ezhttp_get_error_message(request_id, szError, charsmax(szError));
		log_amx("[ezhttp_post_records_oncomplete] Error: %s", szError);
	}
	if(ezhttp_get_http_code(request_id) != 200)
	{
	}
}
public ezhttp_post_records_replay_oncomplete(EzHttpRequest:request_id)
{
}
public cskz_api_post_bans(aBan[banData])
{
}
public cskz_api_post_jumpstats(aStats[jumpstatsData])
{

}
public cskz_api_post_records(aRecord[recordData])
{
	new EzJSON:body = ezjson_init_object();
	ezjson_object_set_string(body, "player.name", aRecord[rd_nickname], true);
	ezjson_object_set_string(body, "player.steam_id", aRecord[rd_steamid], true);
	ezjson_object_set_string(body, "run.map.name", g_szCurrentMap, true);

	if(aRecord[rd_route][0])
	{
		ezjson_object_set_string(body, "run.map.route", aRecord[rd_route], true);
	}
	else
	{
		ezjson_object_set_null(body, "run.map.route", .dot_not = true);
	}
	ezjson_object_set_real(body, "run.time", aRecord[rd_time], true);
	ezjson_object_set_number(body, "run.checkpoints", aRecord[rd_checkpoints], true);
	ezjson_object_set_number(body, "run.gochecks", aRecord[rd_gochecks], true);
	ezjson_object_set_number(body, "run.airaccel", aRecord[rd_airaccel], true);
	ezjson_object_set_string(body, "run.weapon", aRecord[rd_weapon], true);
	ezjson_object_set_number(body, "run.date", aRecord[rd_date], true);

	ezjson_object_set_string(body, "source.hostname", g_szHostname, true);
	ezjson_object_set_string(body, "source.ipaddr", g_szIP, true);
	ezjson_object_set_number(body, "source.port", g_iPort, true);

	new EzHttpOptions:ezOptions = ezhttp_create_options();
	ezhttp_option_set_header(ezOptions, "Content-type", "application/json");
	ezhttp_option_set_header(ezOptions, "Authorization", g_szAPIKey);
	ezhttp_option_set_body_from_json(ezOptions, body);
	ezhttp_post(g_szEndpoints[ENDPOINT_POST_RECORDS], "ezhttp_post_records_oncomplete", ezOptions);
	ezjson_free(body);
}
public cskz_api_post_records_replay(record_id, replay[])
{
}