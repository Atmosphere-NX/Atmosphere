#include "utils.h"
#include "package2.h"
#include "se.h"

void package2_decrypt(void *package2_address);
void package2_add_thermosphere_section(void *package2_address);
void package2_patch_kernel(void *package2_address);
void package2_patch_ini1(void *package2_address);
void package2_fixup_header_and_section_hashes(void *package2_address);

void package2_patch(void *package2_address) {
    /* First things first: Decrypt (TODO: Relocate?) Package2. */
    package2_decrypt(package2_address);
    
    /* Modify Package2 to add an additional thermosphere section. */
    package2_add_thermosphere_section(package2_address);
    
    /* Perform any patches we want to the NX kernel. */
    package2_patch_kernel(package2_address);
    
    /* Perform any patches we want to the INI1 (This is where our built-in sysmodules will be added.) */
    package2_patch_ini1(package2_address);
    
    /* Fix all necessary data in the header to accomodate for the new patches. */
    package2_fixup_header_and_section_hashes(package2_address);
}


void package2_decrypt(void *package2_address) {
    /* TODO */
}

void package2_add_thermosphere_section(void *package2_address) {
    /* TODO */
}

void package2_patch_kernel(void *package2_address) {
    /* TODO */
}

void package2_patch_ini1(void *package2_address) {
    /* TODO */
}

void package2_fixup_header_and_section_hashes(void *package2_address) {
    /* TODO */
}