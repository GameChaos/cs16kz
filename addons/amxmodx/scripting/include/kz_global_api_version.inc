#if defined _kz_global_api_version_included
  #endinput
#endif
#define _kz_global_api_version_included

#define KZ_GLOBAL_API_VERSION_MAJOR 0
#define KZ_GLOBAL_API_VERSION_MINOR 1

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