/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <stratosphere.hpp>

#include "spl_types.hpp"

class SecureMonitorWrapper {
    public:
        static constexpr size_t MaxAesKeyslots = 6;
        static constexpr size_t MaxAesKeyslotsDeprecated = 4;
    private:
        uintptr_t keyslot_owners[MaxAesKeyslots] = {};
        BootReasonValue boot_reason = {};
        bool boot_reason_set = false;
    private:
        static size_t GetMaxKeyslots() {
            return (GetRuntimeFirmwareVersion() >= FirmwareVersion_600) ? MaxAesKeyslots : MaxAesKeyslotsDeprecated;
        }
    private:
        BootReasonValue GetBootReason() const {
            return this->boot_reason;
        }
        bool IsBootReasonSet() const {
            return this->boot_reason_set;
        }
        static Result ConvertToSplResult(SmcResult result);
    private:
        static void InitializeCtrDrbg();
        static void InitializeSeInterruptEvent();
    public:
        static void Initialize();
    private:
        Result GenerateRandomBytesInternal(void *out, size_t size);
    public:
        Result GetConfig(u64 *out, SplConfigItem which);
        Result ExpMod(void *out, size_t out_size, const void *base, size_t base_size, const void *exp, size_t exp_size, const void *mod, size_t mod_size);
        Result SetConfig(SplConfigItem which, u64 value);
        Result GenerateRandomBytes(void *out, size_t size);
        Result IsDevelopment(bool *out);
        Result SetBootReason(BootReasonValue boot_reason);
        Result GetBootReason(BootReasonValue *out);
};
