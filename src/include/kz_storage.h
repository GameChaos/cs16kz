#ifndef KZ_STORAGE_H
#define KZ_STORAGE_H

#include <string>

typedef void(*logfunc)(const char*, ...);

extern void kz_storage_init(logfunc pfn);
extern void kz_storage_uninit(void);
extern int64_t kz_storage_get_next_id(void);
extern void kz_storage_save(const std::string& text, int64_t msg_id);
extern void kz_storage_delete(int64_t msg_id);

#endif
