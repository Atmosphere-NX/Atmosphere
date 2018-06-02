#include <string.h>
#include "utils.h"
#include "se.h"
#include "kernel_patches.h"

#define MAKE_BRANCH(a, o) 0x14000000 | ((((o) - (a)) >> 2) & 0x3FFFFFF)

typedef uint32_t instruction_t;

typedef struct {
    size_t pattern_size;
    const uint8_t *pattern;
    size_t pattern_hook_offset;
    size_t payload_num_instructions;
    const instruction_t *payload;
} kernel_hook_t;

typedef struct {
    uint8_t hash[0x20]; /* TODO: Come up with a better way to identify kernels, that doesn't rely on hashing them. */
    size_t free_code_space_offset;
    unsigned int num_hooks;
    const kernel_hook_t *hooks;
} kernel_info_t;

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
    /* TODO */
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
        
        free_space_offset += hook_size;
        free_space_size -= hook_size;
    }
}