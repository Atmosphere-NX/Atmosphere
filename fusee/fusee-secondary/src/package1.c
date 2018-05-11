#include <errno.h>
#include <stdlib.h>
#include "package1.h"
#include "bct.h"

int package1_parse_boot0(void **package1, size_t *package1_size, nx_keyblob_t *keyblobs, uint32_t *revision, FILE *boot0) {
    static nvboot_config_table bct = {0}; /* Normal firmware BCT, primary. TODO: check? */
    nv_bootloader_info *pk1_info = &bct.bootloader[0]; /* TODO: check? */

    size_t fpos, pk1_offset;

    if (package1 == NULL || package1_size != NULL || keyblobs == NULL || revision == NULL || boot0 == NULL) {
        errno = EINVAL;
        return -1;
    }

    fpos = ftell(boot0);

    /* Read the BCT. */
    if (fread(&bct, sizeof(nvboot_config_table), 1, boot0) == 0) {
        return -1;
    }
    if (bct.bootloader_used < 1) {
        errno = EILSEQ;
        return -1;
    }

    *revision = pk1_info->attribute;
    *package1_size = pk1_info->length;

    pk1_offset = 0x4000 * pk1_info->start_blk + 0x200 * pk1_info->start_page;

    (*package1) = malloc(*package1_size);

    if (*package1 == NULL) {
        errno = ENOMEM;
        return -1;
    }

    /* Read the pk1/pk1l. */
    if (fseek(boot0, fpos + pk1_offset, SEEK_SET) != 0) {
        return -1;
    }
    if (fread(*package1, *package1_size, 1, boot0) == 0) {
        return -1;
    }

    /* Skip the backup pk1/pk1l. */
    if (fseek(boot0, *package1_size, SEEK_CUR) != 0) {
        return -1;
    }

    /* Read the full keyblob area.*/
    for (size_t i = 0; i < 32; i++) {
        if (fread(&keyblobs[i], sizeof(nx_keyblob_t), 1, boot0) == 0) {
            return -1;
        }
        if (fseek(boot0, 0x200 - sizeof(nx_keyblob_t), SEEK_CUR)) {
            return -1;
        }
    }

    return 0;
}
