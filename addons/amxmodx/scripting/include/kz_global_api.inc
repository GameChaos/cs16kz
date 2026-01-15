#if defined _kz_global_api_included
  #endinput
#endif
#define _kz_global_api_included

#define KZ_GLOBAL_API_VERSION_MAJOR 0
#define KZ_GLOBAL_API_VERSION_MINOR 1

/**
 * Map types
 */
enum KZMap_Type
{
	KZM_BHOP,
	KZM_CLIMB,
	KZM_SLIDE,
	KZM_MIX,
	KZM_SPECIAL,
};

/**
 * Map length
 */
enum KZMap_Length
{
	KZM_VERY_SHORT = 0,
	KZM_SHORT,
	KZM_MIDDLE,
	KZM_LONG,
	KZM_VERY_LONG,
};

/**
 * Map difficulty
 */
enum KZMap_Difficulty
{
	KZM_BEGINNER = 0,
	KZM_EASY,
	KZM_EASY_MEDIUM,
	KZM_MEDIUM,
	KZM_MEDIUM_HARD,
	KZM_HARD,
	KZM_HARD_EXTREME,
	KZM_EXTREME,
	KZM_EXTREME_DEATH,
	KZM_DEATH,
};

/**
 * Weapons enum for the KZ API.
 */
enum KZWeapons
{
	KZW_AWP = 0,
	KZW_M249,
	KZW_AK47,
	KZW_M4A1,
	KZW_SG552,
	KZW_FAMAS,
	KZW_P90,
	KZW_USP,
	KZW_SCOUT,	
};

/**
 * Called when the current map's information is retrieved from the global database.
 *
 * @param type          The map category (from KZMap_Type enum)
 * @param length        The map length (from KZMap_Length enum)
 * @param difficulty    The map difficulty (from KZMap_Difficulty enum)
 *
 * @noreturn
 */
forward kz_api_on_map_loaded(KZMap_Type:type, KZMap_Length:length, KZMap_Difficulty:difficulty);

/**
 * Called when a record has been successfully added to the global database.
 *
 * @param global_rec_id The unique record ID assigned by the database
 * @param data          Additional data array passed from the initial native call
 * @param dataSize      The size of the additional data array
 *
 * @noreturn
 */
forward kz_api_on_record_added(global_rec_id, data[], dataSize);

/**
 * Called when a replay upload process has completed.
 *
 * @param global_rec_id The unique record ID associated with the replay
 *
 * @noreturn
 */
forward kz_api_on_replay_uploaded(global_rec_id);

/**
 * Called when a replay download process has completed.
 *
 * @param global_rec_id The unique record ID requested
 * @param filepath      The local path where the file was saved
 *
 * @noreturn
 */
forward kz_api_on_replay_downloaded(global_rec_id, filepath[]);

/**
 * Adds a new record to the global KZ database.
 *
 * @param id            Player index
 * @param seconds       The completion time in seconds
 * @param checkpoints   Number of checkpoints made
 * @param gochecks      Number of teleports (gochecks) used
 * @param weapon        The weapon used during the run (from KZWeapons enum)
 * @param data          Additional data array to be passed to the forward handler
 * @param dataSize      The size of the additional data array
 *
 * @return              1 on success, 0 otherwise
 */
native kz_api_add_record(id, Float:seconds, checkpoints, gochecks, KZWeapons:weapon, any:data[], dataSize);

/**
 * Deletes a record from the global database.
 *
 * @param global_rec_id The unique record ID to delete
 *
 * @return              1 on success, 0 otherwise
 */
native kz_api_del_record(global_rec_id);

/**
 * Associates a replay file with an existing global record.
 *
 * @param global_rec_id The unique record ID returned by the API
 * @param filepath      The local path to the replay file to be uploaded
 *
 * @return              1 on success, 0 otherwise
 */
native kz_api_add_replay(global_rec_id, filepath[]);

/**
 * Retrieves a replay file for a specific global record.
 *
 * @param global_rec_id The unique record ID to fetch
 * @param filepath      The local path where the replay file should be saved
 *
 * @return              1 on success, 0 otherwise
 */
native kz_api_get_replay(global_rec_id, filepath[]);

/**
 * Internal callback for version verification. Do not modify.
 */
public __kz_global_api_version_check(const majorVersion, const minorVersion)
{
	if (majorVersion != KZ_GLOBAL_API_VERSION_MAJOR)
	{
		new temp[512];
		formatex(temp, sizeof temp - 1, "[KZ Global]: Api major version mismatch; expected %d, real %d", KZ_GLOBAL_API_VERSION_MAJOR, majorVersion);
		set_fail_state(temp);
		return;
	}

	if (minorVersion < KZ_GLOBAL_API_VERSION_MINOR)
	{
		new temp[512];
		formatex(temp, sizeof temp - 1, "[KZ Global]: Api minor version mismatch; expected at least %d, real %d", KZ_GLOBAL_API_VERSION_MINOR, minorVersion);
		set_fail_state(temp);
		return;
	}
}