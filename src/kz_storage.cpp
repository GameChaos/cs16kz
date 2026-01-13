#include <iostream>
#include <vector>

#if defined (__linux__)
    #include <sys/types.h>
    #include <sys/stat.h>
#endif


#include <SQLiteCpp/SQLiteCpp.h>
#include "kz_storage.h"


static const char* KZ_DATABASE_PATH = "cstrike/addons/amxmodx/data/sqlite3";
static const char* KZ_DATABASE_FILE = "cstrike/addons/amxmodx/data/sqlite3/kz_global_api.sq3";

static thread_local SQLite::Database* kz_storage_database = nullptr;
static thread_local bool kz_storage_initialiazed = false;

typedef void(*logfunc)(const char*, ...);
static thread_local logfunc kz_storage_log = nullptr;

inline bool DirExists(const char *dir)
{
#if defined WIN32 || defined _WIN32
	DWORD attr = GetFileAttributes(dir);

	if (attr == INVALID_FILE_ATTRIBUTES)
		return false;

	if (attr & FILE_ATTRIBUTE_DIRECTORY)
		return true;

#else
	struct stat s;

	if (stat(dir, &s) != 0)
		return false;

	if (S_ISDIR(s.st_mode))
		return true;
#endif

	return false;
}


void kz_storage_init(logfunc pfn)
{
    if(!kz_storage_initialiazed)
    {
        kz_storage_log = pfn;

        if (!DirExists(KZ_DATABASE_PATH))
	{
            #if defined(__linux__)
            mkdir(KZ_DATABASE_PATH, 0775);
            #else
            mkdir(KZ_DATABASE_PATH);
            #endif
	}
        try
        {
            kz_storage_database = new SQLite::Database(KZ_DATABASE_FILE, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
            kz_storage_database->exec("PRAGMA journal_mode=WAL;");
            kz_storage_database->exec("CREATE TABLE IF NOT EXISTS outgoing_queue(id INTEGER PRIMARY KEY AUTOINCREMENT, msg TEXT NOT NULL)");
            kz_storage_database->setBusyTimeout(5000);

            kz_storage_initialiazed = true;
        }
        catch (const std::exception& e)
        {
            if(kz_storage_log)
            {
                kz_storage_log("[Storage] init: %s", e.what());
            }
        }
    }
}
void kz_storage_uninit(void)
{
    if(kz_storage_database)
    {
        delete kz_storage_database;
        kz_storage_database = nullptr;
        kz_storage_initialiazed = false;

        if(kz_storage_log)
        {
            kz_storage_log("[Storage] uninit: Connection closed.");
        }
    }
}
int64_t kz_storage_get_next_id(void)
{
    try
    {
        int64_t next_id = 1;
        SQLite::Statement query(*kz_storage_database, "SELECT seq FROM sqlite_sequence WHERE name='outgoing_queue'");
        if(query.executeStep())
        {
            next_id = query.getColumn(0).getInt64() + 1;
        }
        return next_id;
    }
    catch (const std::exception& e)
    {
        if(kz_storage_log)
        {
            kz_storage_log("[Storage] get_next_id: %s", e.what());
        }
    }
    return 1;
}
void kz_storage_save(const std::string& text, int64_t msg_id)
{
    try
    {
        SQLite::Statement query(*kz_storage_database, "INSERT INTO outgoing_queue (id, msg) VALUES (?, ?)");
        query.bind(1, static_cast<long long>(msg_id));
        query.bind(2, text);
        query.exec();
    }
    catch (const std::exception& e)
    {
        if(kz_storage_log)
        {
            kz_storage_log("[Storage] save: %s", e.what());
        }
    }
}
void kz_storage_delete(int64_t msg_id)
{
    try
    {
        SQLite::Statement query(*kz_storage_database, "DELETE FROM outgoing_queue WHERE id = ?");
        query.bind(1, static_cast<long long>(msg_id));
        query.exec();
    }
    catch (const std::exception& e)
    {
        if(kz_storage_log)
        {
            kz_storage_log("[Storage] delete: %s", e.what());
        }
    }
}
