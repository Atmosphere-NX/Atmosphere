/*
 * Copyright (c) 2018-2020 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <switch.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    u32 version;
    bool exfat_supported;
    u32 num_firmware_variations;
    u32 firmware_variation_ids[16];
} AmsSuUpdateInformation;

typedef struct {
    Result result;
    Result exfat_result;
    NcmContentMetaKey invalid_key;
    NcmContentId invalid_content_id;
} AmsSuUpdateValidationInfo;

Result amssuInitialize();
void   amssuExit();
Service *amssuGetServiceSession(void);

Result amssuGetUpdateInformation(AmsSuUpdateInformation *out, const char *path);
Result amssuValidateUpdate(AmsSuUpdateValidationInfo *out, const char *path);
Result amssuSetupUpdate(void *buffer, size_t size, const char *path, bool exfat);
Result amssuSetupUpdateWithVariation(void *buffer, size_t size, const char *path, bool exfat, u32 variation);
Result amssuRequestPrepareUpdate(AsyncResult *a);
Result amssuGetPrepareUpdateProgress(NsSystemUpdateProgress *out);
Result amssuHasPreparedUpdate(bool *out);
Result amssuApplyPreparedUpdate();

#ifdef __cplusplus
}
#endif