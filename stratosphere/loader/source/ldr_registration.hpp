#pragma once
#include <switch.h>

#define REGISTRATION_LIST_MAX (0x40)

#define NSO_INFO_MAX (0x20)
#define NRR_INFO_MAX (0x40)

class Registration {
    public:
        struct NsoInfo {
            u64 base_address;
            u64 size;
            unsigned char build_id[0x20];
        };
        
        struct NsoInfoHolder {
            bool in_use;
            NsoInfo info;
        };
        
        struct NrrInfo {
            u64 base_address;
            u64 size;
            u64 code_memory_address;
            u64 loader_address;
        };
        
        struct NrrInfoHolder {
            bool in_use;
            NrrInfo info;
        };
        
        struct TidSid {
            u64 title_id;
            FsStorageId storage_id;
        };
        
        struct Process {
            bool in_use;
            bool is_64_bit_addspace;
            u64 index;
            u64 process_id;
            u64 title_id_min;
            Registration::TidSid tid_sid;
            Registration::NsoInfoHolder nso_infos[NSO_INFO_MAX];
            Registration::NrrInfoHolder nrr_infos[NRR_INFO_MAX];
            void *owner_ro_service;
        };
        
        struct List {
            Registration::Process processes[REGISTRATION_LIST_MAX];
            u64 num_processes;
        };
        
        static Registration::Process *GetFreeProcess();
        static Registration::Process *GetProcess(u64 index);
        static Registration::Process *GetProcessByProcessId(u64 pid);
        static Result GetRegisteredTidSid(u64 index, Registration::TidSid *out);
        static bool RegisterTidSid(const TidSid *tid_sid, u64 *out_index);
        static bool UnregisterIndex(u64 index);
        static void SetProcessIdTidMinAndIs64BitAddressSpace(u64 index, u64 process_id, u64 tid_min, bool is_64_bit_addspace);
        static void AddNsoInfo(u64 index, u64 base_address, u64 size, const unsigned char *build_id);
        static Result AddNrrInfo(u64 index, u64 base_address, u64 size, u64 code_memory_address, u64 loader_address);
        static Result GetNsoInfosForProcessId(NsoInfo *out, u32 max_out, u64 process_id, u32 *num_written);
};
