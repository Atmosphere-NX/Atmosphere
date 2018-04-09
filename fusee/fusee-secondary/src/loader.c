#include "utils.h"
#include <stdint.h>
#include "loader.h"
#include "sd_utils.h"
#include "stage2.h"
#include "lib/printk.h"
#include "lib/vsprintf.h"
#include "lib/ini.h"

const char *g_bct0 = NULL;

loader_ctx_t g_loader_ctx = {0};

loader_ctx_t *get_loader_ctx(void) {
    return &g_loader_ctx;
}

static int loadlist_entry_ini_handler(void *user, const char *section, const char *name, const char *value) {
    load_file_t *load_file_ctx = (load_file_t *)user;
    uintptr_t x = 0;
    const char *ext = NULL;
    
    if (strcmp(section, "stage2") == 0) {
        if (strstr(name, load_file_ctx->key) == name) {
            ext = name + strlen(load_file_ctx->key);
            if (strcmp(ext, "_path") == 0) {
                /* Copy in the path. */
                strncpy(load_file_ctx->path, value, sizeof(load_file_ctx->path));
            } else if (strcmp(ext, "_addr") == 0) {
                /* Read in load address as a hex string. */
                sscanf(value, "%x", &x);
                load_file_ctx->load_address = x;
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    } else {
        return 0;
    }
    return 1;
}

bool validate_load_address(uintptr_t load_addr) {
    /* TODO: Actually validate addresses. */
    (void)(load_addr);
    return true;
}

void load_list_entry(const char *key) {
    load_file_t load_file_ctx = {0};
    load_file_ctx.key = key;
    
    printk("Loading %s\n", key);
    
    if (ini_parse_string(g_bct0, loadlist_entry_ini_handler, &load_file_ctx) < 0) {
        printk("Error: Failed to parse BCT.ini!\n");
        generic_panic();
    }
    
    if (load_file_ctx.load_address == 0 || load_file_ctx.path[0] == '\x00') {
        printk("Error: Failed to determine where to load %s!\n", key);
        generic_panic();
    }
    
    printk("Loading %s from %s to 0x%08x\n", key, load_file_ctx.path, load_file_ctx.load_address);
    
    if (!validate_load_address(load_file_ctx.load_address)) {
        printk("Error: Load address 0x%08x is invalid!\n");
        generic_panic();
    }
    
    if (!read_sd_file((void *)load_file_ctx.load_address, LOADER_FILESIZE_MAX, load_file_ctx.path)) {
        printk("Error: Failed to read %s!\n", load_file_ctx.path);
        generic_panic();
    }
    
    /* Check for special keys. */
    if (strcmp(key, LOADER_PACKAGE2_KEY) == 0) {
        get_loader_ctx()->package2_loadfile = load_file_ctx;
    } else if (strcmp(key, LOADER_EXOSPHERE_KEY) == 0) {
        get_loader_ctx()->exosphere_loadfile = load_file_ctx;
    } else if (strcmp(key, LOADER_TSECFW_KEY) == 0) {
        get_loader_ctx()->tsecfw_loadfile = load_file_ctx;
    } else if (strcmp(key, LOADER_WARMBOOT_KEY) == 0) {
        get_loader_ctx()->warmboot_loadfile = load_file_ctx;
    }
}

void parse_loadlist(const char *ll) {
    printk("Parsing load list: %s\n", ll);
    
    char load_list[0x200] = {0};
    strncpy(load_list, ll, 0x200);
    
    char *entry, *p;
    
    /* entry will point to the start of the current entry. p will point to the current location in the list. */
    entry = load_list;
    p = load_list;
    
    while (*p) {
        if (*p == ' ' || *p == '\t') {
            /* We're at the end of an entry. */
            *p = '\x00';
            
            /* Load the entry. */
            load_list_entry(entry);
            /* Skip to the next delimiter. */
            for (; *p == ' ' || *p == '\t'; p++) { }
            if (*p == '\x00') { 
                break;
            } else if (*p != '|') { 
                printk("Error: Load list is malformed!\n");
                generic_panic();
            } else {
                /* Skip to the next entry. */
                for (; *p == ' ' || *p == '\t'; p++) { }
                entry = p;
            }
        }
        
        p++;
    }
}

static int loadlist_ini_handler(void *user, const char *section, const char *name, const char *value) {
    loader_ctx_t *loader_ctx = (loader_ctx_t *)user;
    uintptr_t x = 0;
    
    if (strcmp(section, "stage2") == 0) {
        if (strcmp(name, LOADER_LOADLIST_KEY) == 0) {
            parse_loadlist(value);
        } else if (strcmp(name, LOADER_ENTRYPOINT_KEY) == 0) {
            /* Read in entrypoint as a hex string. */
            sscanf(value, "%x", &x);
            loader_ctx->chainload_entrypoint = (entrypoint_t)x;
        } else {
            return 0;
        }
    } else {
        return 0;
    }
    return 1;
}

void load_payload(const char *bct0) {
    loader_ctx_t *ctx = get_loader_ctx();
    
    /* Set BCT0 global. */
    ctx->bct0 = bct0;
    
    if (ini_parse_string(ctx->bct0, loadlist_ini_handler, ctx) < 0) {
        printk("Error: Failed to parse BCT.ini!\n");
        generic_panic();
    }
}