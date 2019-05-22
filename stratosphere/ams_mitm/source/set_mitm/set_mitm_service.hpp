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

#include "../utils.hpp"

enum SetCmd : u32 {
    SetCmd_GetLanguageCode = 0,
    SetCmd_GetRegionCode = 4,

    /* Commands for which set:sys *must* act as a passthrough. */
    /* TODO: Solve the relevant IPC detection problem. */
    SetCmd_GetAvailableLanguageCodes = 1,
};

class SetMitmService : public IMitmServiceObject {
    private:
        HosMutex lock;
        OverrideLocale locale;
        bool got_locale;
    public:
        SetMitmService(std::shared_ptr<Service> s, u64 pid) : IMitmServiceObject(s, pid) {
            this->got_locale = false;
        }

        static bool ShouldMitm(u64 pid, u64 tid) {
            /* Mitm all applications. */
            return tid == TitleId_Ns || TitleIdIsApplication(tid);
        }

        static void PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx);

    protected:
        static bool IsValidLanguageCode(u64 lang_code);
        static bool IsValidRegionCode(u32 region_code);

        void EnsureLocale();
    protected:
        /* Overridden commands. */
        Result GetLanguageCode(Out<u64> out_lang_code);
        Result GetRegionCode(Out<u32> out_region_code);

        /* Forced passthrough commands. */
        Result GetAvailableLanguageCodes(OutPointerWithClientSize<u64> out_language_codes, Out<s32> out_count);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MakeServiceCommandMeta<SetCmd_GetLanguageCode, &SetMitmService::GetLanguageCode>(),
            MakeServiceCommandMeta<SetCmd_GetRegionCode, &SetMitmService::GetRegionCode>(),

            MakeServiceCommandMeta<SetCmd_GetAvailableLanguageCodes, &SetMitmService::GetAvailableLanguageCodes>(),
        };
};
