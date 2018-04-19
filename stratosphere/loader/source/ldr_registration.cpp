#include <switch.h>
#include <algorithm>
#include <cstdio>
#include "ldr_registration.hpp"

static Registration::List g_registration_list = {0};
static u64 g_num_registered = 1;

Registration::Process *Registration::get_free_process() {
    unsigned int i;
    for (i = 0; i < REGISTRATION_LIST_MAX; i++) {
        if (!g_registration_list.processes[i].in_use) {
            return &g_registration_list.processes[i];
        }
    }
    return NULL;
}

Registration::Process *Registration::get_process(u64 index) {
    unsigned int i;
    for (i = 0; !g_registration_list.processes[i].in_use || g_registration_list.processes[i].index != index; i++) {
        if (i >= REGISTRATION_LIST_MAX) {
            return NULL;
        }
    }
    return &g_registration_list.processes[i];
}

bool Registration::register_tid_sid(const TidSid *tid_sid, u64 *out_index) { 
    Registration::Process *free_process = get_free_process();
    if (free_process == NULL) {
        return false;
    }
    
    /* Reset the process. */
    *free_process = {0};
    free_process->tid_sid = *tid_sid;
    free_process->in_use = true;
    free_process->index = g_num_registered++;
    *out_index = free_process->index;
    return true;
}

bool Registration::unregister_index(u64 index) {
    Registration::Process *target_process = get_process(index);
    if (target_process == NULL) {
        return false;
    }
    
    /* Reset the process. */
    *target_process = {0};
    return true;
}

void Registration::set_process_id_and_tid_min(u64 index, u64 process_id, u64 tid_min) {
    Registration::Process *target_process = get_process(index);
    if (target_process == NULL) {
        return;
    }
    
    target_process->process_id = process_id;
    target_process->title_id_min = tid_min;
}

void Registration::add_nso_info(u64 index, u64 base_address, u64 size, const unsigned char *build_id) {
    Registration::Process *target_process = get_process(index);
    if (target_process == NULL) {
        return;
    }
    
    for (unsigned int i = 0; i < NSO_INFO_MAX; i++) {
        if (!target_process->nso_infos[i].in_use) {
            target_process->nso_infos[i].base_address = base_address;
            target_process->nso_infos[i].size = size;
            std::copy(build_id, build_id + sizeof(target_process->nso_infos[i].build_id), target_process->nso_infos[i].build_id);
            target_process->nso_infos[i].in_use = true;
            return;
        }
    }
}