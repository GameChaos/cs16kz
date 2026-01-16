#include <amxmodx>
#include <kz_global_api>

enum _:something_that_resembles_a_c_struct
{
	number,
	Float:decimal,
	first[2],
	second[64]
};

public plugin_init()
{
	register_clcmd("test_add_record", "test_add_record");

	kz_api_get_map_details("de_dust2", "map_details_handler");
	kz_api_get_map_details("kz_longjump2", "map_details_handler");
	kz_api_get_map_details("bkz_goldbhop", "map_details_handler");
}
public kz_api_on_record_added(global_rec_id, data[], dataSize)
{
	new var[something_that_resembles_a_c_struct];
	if(dataSize == sizeof(var))
	{
		server_print("---- something_that_resembles_a_c_struct");
	}
	if(dataSize)
	{
		server_print("[AMXX] New global record: (global_rec_id: %d, data: %s, dataSize: %d)", global_rec_id, data, dataSize);
		if(dataSize == sizeof(var))
		{
			/// kz_api_add_record(id, ..., var, sizeof(var));
			/// Previous server_print will output some garbage + first[]
			/// We output second[]
			server_print("data[second] = %s", data[second]);
		}
	}
	else
	{
		server_print("[AMXX] New global record: (global_rec_id: %d)", global_rec_id);
	}
}
public test_add_record(id)
{
	kz_api_add_record(id, random_float(0.1, 99999.0), random_num(0, 99999), random_num(0, 99999), KZWeapons:random_num(_:KZW_AWP, _:KZW_SCOUT), "", 0);
	kz_api_add_record(id, random_float(0.1, 99999.0), random_num(0, 99999), random_num(0, 99999), KZWeapons:random_num(_:KZW_AWP, _:KZW_SCOUT), "rr", 1);

	new var[something_that_resembles_a_c_struct];
	var[number] = 5555;
	var[decimal] = _:1451.1541;
	
	copy(var[first], 1, "b");
	copy(var[second], 63, " dasdc23c4ewv5wa54vaw456ab6");

	kz_api_add_record(id, random_float(0.1, 99999.0), random_num(0, 99999), random_num(0, 99999), KZWeapons:random_num(_:KZW_AWP, _:KZW_SCOUT), var[second], strlen(var[second]));
	kz_api_add_record(id, random_float(0.1, 99999.0), random_num(0, 99999), random_num(0, 99999), KZWeapons:random_num(_:KZW_AWP, _:KZW_SCOUT), var, sizeof(var));
}
public map_details_handler(mapname[], wr[], map_props[3])
{
	new szType[32], szLength[32], szDifficulty[32];

	kz_api_get_map_type(map_props[0], szType, charsmax(szType));
	kz_api_get_map_length(map_props[1], szLength, charsmax(szLength));
	kz_api_get_map_difficulty(map_props[2], szDifficulty, charsmax(szDifficulty));

	server_print("[AMXX] Received details for map (%s): [type: %d][length: %d][diff: %d] - [%s][%s][%s]", mapname, map_props[0], map_props[1], map_props[2], szType, szLength, szDifficulty);
}
