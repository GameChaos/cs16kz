#ifndef KZ_STORAGE_H
#define KZ_STORAGE_H

enum class StorageTable : int
{
    outgoing_queue,
    replay_up_queue,
};

extern void kz_storage_init(void);
extern void kz_storage_uninit(void);
extern int64_t kz_storage_get_next_id(StorageTable table);
extern void kz_storage_save(const std::string& text, int64_t msg_id, StorageTable table);
extern void kz_storage_delete(int64_t msg_id, StorageTable table);

#endif
