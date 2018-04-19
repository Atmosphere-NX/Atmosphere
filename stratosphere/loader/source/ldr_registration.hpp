#pragma once
#include <switch.h>

#define REGISTRATION_LIST_MAX (0x40)

#define NSO_INFO_MAX (0x20)

class Registration {
    public:
        struct NsoInfo {
            bool in_use;
            u64 base_address;
            u64 size;
            unsigned char build_id[0x20];
        };
        
        struct TidSid {
            u64 title_id;
            FsStorageId storage_id;
        };
        
        struct Process {
            bool in_use;
            u64 index;
            u64 process_id;
            u64 title_id_min;
            Registration::TidSid tid_sid;
            Registration::NsoInfo nso_infos[NSO_INFO_MAX];
            u64 _0x730;
        };
        
        struct List {
            Registration::Process processes[REGISTRATION_LIST_MAX];
            u64 num_processes;
        };
        
        static Registration::Process *get_free_process();
        static Registration::Process *get_process(u64 index);
        static bool register_tid_sid(const TidSid *tid_sid, u64 *out_index);
        static bool unregister_index(u64 index);
        static void set_process_id_and_tid_min(u64 index, u64 process_id, u64 tid_min);
        static void add_nso_info(u64 index, u64 base_address, u64 size, const unsigned char *build_id);
};
