#include <string.h>
#include "utils.h"
#include "se.h"
#include "kernel_patches.h"

#define MAKE_BRANCH(a, o) 0x14000000 | ((((o) - (a)) >> 2) & 0x3FFFFFF)

#define MAKE_KERNEL_PATTERN_NAME(vers, name) g_kernel_patch_##vers_##name
#define MAKE_KERNEL_HOOK_NAME(vers, name) g_kernel_hook_##vers_##name

typedef uint32_t instruction_t;

typedef struct {
    size_t pattern_size;
    const uint8_t *pattern;
    size_t pattern_hook_offset;
    size_t payload_num_instructions;
    size_t branch_back_offset;
    const instruction_t *payload;
} kernel_hook_t;

typedef struct {
    uint8_t hash[0x20]; /* TODO: Come up with a better way to identify kernels, that doesn't rely on hashing them. */
    size_t free_code_space_offset;
    unsigned int num_hooks;
    const kernel_hook_t *hooks;
} kernel_info_t;

/* Patch definitions. */
/*  
    mov w10, w23
    lsl x10, x10, #2
    ldr x10, [x27, x10]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #12
    ldr x10, [sp,#0x80]
    ldr x8, [x10,#0x2b0]
    ldr x10, [sp,#0x80]
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(500, proc_id_send)[] = {0xEA, 0x43, 0x40, 0xF9, 0x48, 0x59, 0x41, 0xF9, 0xE9, 0x03, 0x17, 0x2A, 0x29, 0xF5, 0x7E, 0xD3};
static const instruction_t MAKE_KERNEL_HOOK_NAME(500, proc_id_send)[] = {0x2A1703EA, 0xD37EF54A, 0xF86A6B6A, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000060, 0xF94043EA, 0xF9415948, 0xF94043EA};
/*  
    ldr x13, [sp, #0x70]
    mov w10, w21
    lsl x10, x10, #2
    ldr x10, [x13, x10]
    mov x9, #0x0000ffffffffffff
    and x8, x10, x9
    mov x9, #0xffff000000000000
    and x10, x10, x9
    mov x9, #0xfffe000000000000
    cmp x10, x9
    beq #8
    ldr x8, [x24,#0x2b0]
    ldr x10, [sp,#0xd8] 
*/
static const uint8_t MAKE_KERNEL_PATTERN_NAME(500, proc_id_recv)[] = {0x08, 0x5B, 0x41, 0xF9, 0xEA, 0x6F, 0x40, 0xF9, 0xE9, 0x03, 0x15, 0x2A, 0x29, 0xF5, 0x7E, 0xD3};
static const instruction_t MAKE_KERNEL_HOOK_NAME(500, proc_id_recv)[] = {0xF9403BED, 0x2A1503EA, 0xD37EF54A, 0xF86A69AA, 0x92FFFFE9, 0x8A090148, 0xD2FFFFE9, 0x8A09014A, 0xD2FFFFC9, 0xEB09015F, 0x54000040, 0xF9415B08, 0xF9406FEA};

/* Hook Definitions. */
static const kernel_hook_t g_kernel_hooks_100[] = {
    /* TODO */
};
static const kernel_hook_t g_kernel_hooks_200[] = {
    /* TODO */
};
static const kernel_hook_t g_kernel_hooks_300[] = {
    /* TODO */
};
static const kernel_hook_t g_kernel_hooks_302[] = {
    /* TODO */
};
static const kernel_hook_t g_kernel_hooks_400[] = {
    /* TODO */
};
static const kernel_hook_t g_kernel_hooks_500[] = {
    {   /* Send Message Process ID Patch. */
        .pattern_size = 0x10,
        .pattern = MAKE_KERNEL_PATTERN_NAME(500, proc_id_send),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = 13,
        .branch_back_offset = 0x8,
        .payload = MAKE_KERNEL_HOOK_NAME(500, proc_id_send)
    },
    {   /* Receive Message Process ID Patch. */
        .pattern_size = 0x10,
        .pattern = MAKE_KERNEL_PATTERN_NAME(500, proc_id_recv),
        .pattern_hook_offset = 0x0,
        .payload_num_instructions = 13,
        .branch_back_offset = 0x8,
        .payload = MAKE_KERNEL_HOOK_NAME(500, proc_id_recv)
    }
};

#define KERNEL_HOOKS(vers) .num_hooks = sizeof(g_kernel_hooks_##vers)/sizeof(kernel_hook_t), .hooks = g_kernel_hooks_##vers,

/* Kernel Infos. */
static const kernel_info_t g_kernel_infos[] = {
    {   /* 1.0.0. */
        .free_code_space_offset = 0x4597C,
        KERNEL_HOOKS(100)
    },
    {   /* 2.0.0. */
        /* TODO */
        .free_code_space_offset = 0,
        KERNEL_HOOKS(200)
    },
    {   /* 3.0.0. */
        /* TODO */
        .free_code_space_offset = 0,
        KERNEL_HOOKS(300)
    },
    {   /* 3.0.2. */
        /* TODO */
        .free_code_space_offset = 0,
        KERNEL_HOOKS(302)
    },
    {   /* 4.0.0. */
        /* TODO */
        .free_code_space_offset = 0,
        KERNEL_HOOKS(400)
    },
    {   /* 5.0.0. */
        .hash = {0xB2, 0x38, 0x61, 0xA8, 0xE1, 0xE2, 0xE4, 0xE4, 0x17, 0x28, 0xED, 0xA9, 0xF6, 0xF6, 0xBD, 0xD2, 0x59, 0xDB, 0x1F, 0xEF, 0x4A, 0x8B, 0x2F, 0x1C, 0x64, 0x46, 0x06, 0x40, 0xF5, 0x05, 0x9C, 0x43},
        .free_code_space_offset = 0x5C020,
        KERNEL_HOOKS(500)
    }
};

/* Adapted from https://github.com/AuroraWright/Luma3DS/blob/master/sysmodules/loader/source/memory.c:35. */
uint8_t *search_pattern(void *_mem, size_t mem_size, const void *_pattern, size_t pattern_size) {
    const uint8_t *pattern = (const uint8_t *)_pattern;
    uint8_t *mem = (uint8_t *)_mem;
    
    uint32_t table[0x100];
    for (unsigned int i = 0; i < sizeof(table)/sizeof(uint32_t); i++) {
        table[i] = (uint32_t)pattern_size;
    }
    for (unsigned int i = 0; i < pattern_size - 1; i++) {
        table[pattern[i]] = (uint32_t)pattern_size - i - 1;
    }
    
    for (unsigned int i = 0; i <= mem_size - pattern_size; i += table[mem[i + pattern_size - 1]]) {
        if (pattern[pattern_size - 1] == table[mem[i + pattern_size - 1]] && memcmp(pattern, mem + i, pattern_size - 1) == 0) {
            return mem + i;
        }
    }
    return NULL;
}

const kernel_info_t *get_kernel_info(void *kernel, size_t size) {
    uint8_t calculated_hash[0x20];
    se_calculate_sha256(calculated_hash, kernel, size);
    for (unsigned int i = 0; i < sizeof(g_kernel_infos)/sizeof(kernel_info_t); i++) {
        if (memcmp(calculated_hash, g_kernel_infos[i].hash, sizeof(calculated_hash)) == 0) {
            return &g_kernel_infos[i];
        }
    }
    return NULL;
}

void package2_patch_kernel(void *_kernel, size_t size) {
    const kernel_info_t *kernel_info = get_kernel_info(_kernel, size);
    if (kernel_info == NULL) {
        /* Should this be fatal? */
        fatal_error("kernel_patcher: unable to identify kernel!\n");
    }
    
    /* Apply hooks. */
    uint8_t *kernel = (uint8_t *)_kernel;
    size_t free_space_offset = kernel_info->free_code_space_offset;
    size_t free_space_size = ((free_space_offset + 0xFFFULL) & ~0xFFFULL) - free_space_offset;
    for (unsigned int i = 0; i < kernel_info->num_hooks; i++) {
        size_t hook_size = sizeof(instruction_t) * kernel_info->hooks[i].payload_num_instructions;
        if (kernel_info->hooks[i].branch_back_offset) {
            hook_size += sizeof(instruction_t);
        }
        if (free_space_size < hook_size) {
            /* TODO: What should be done in this case? */
            fatal_error("kernel_patcher: insufficient space to apply patches!\n");
        }
        
        uint8_t *pattern_loc = search_pattern(kernel, size, kernel_info->hooks[i].pattern, kernel_info->hooks[i].pattern_size);
        if (pattern_loc == NULL) {
            /* TODO: Should we print an error/abort here? */
            continue;
        }
        /* Patch kernel to branch to our hook at the desired place. */
        instruction_t *hook_start = (instruction_t *)(pattern_loc + kernel_info->hooks[i].pattern_hook_offset);
        *hook_start = MAKE_BRANCH((uint32_t)((uintptr_t)hook_start - (uintptr_t)kernel), free_space_offset);
        
        /* Insert hook into free space. */
        instruction_t *payload = (instruction_t *)(kernel + free_space_offset);
        for (unsigned int p = 0; p < kernel_info->hooks[i].payload_num_instructions; p++) {
            payload[p] = kernel_info->hooks[i].payload[p];
        }
        if (kernel_info->hooks[i].branch_back_offset) {
            payload[kernel_info->hooks[i].payload_num_instructions] = MAKE_BRANCH(free_space_offset + sizeof(instruction_t) * kernel_info->hooks[i].payload_num_instructions, (uint32_t)(kernel_info->hooks[i].branch_back_offset + (uintptr_t)hook_start - (uintptr_t)kernel));
        }
        
        free_space_offset += hook_size;
        free_space_size -= hook_size;
    }
}