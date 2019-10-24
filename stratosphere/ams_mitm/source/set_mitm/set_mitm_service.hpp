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

class SetMitmService : public IMitmServiceObject {
    private:
        enum class CommandId {
            GetLanguageCode = 0,
            GetRegionCode   = 4,
        };
    private:
        ams::os::Mutex lock;
        OverrideLocale locale;
        bool got_locale;
    public:
        SetMitmService(std::shared_ptr<Service> s, u64 pid, ams::ncm::TitleId tid) : IMitmServiceObject(s, pid, tid) {
            this->got_locale = false;
        }

        static bool ShouldMitm(u64 pid, ams::ncm::TitleId tid) {
            /* Mitm all applications. */
            return tid == ams::ncm::TitleId::Ns || ams::ncm::IsApplicationTitleId(tid);
        }

        static void PostProcess(IMitmServiceObject *obj, IpcResponseContext *ctx);

    protected:
        static bool IsValidLanguageCode(u64 lang_code);
        static bool IsValidRegionCode(u32 region_code);

        Result EnsureLocale();
    protected:
        /* Overridden commands. */
        Result GetLanguageCode(Out<u64> out_lang_code);
        Result GetRegionCode(Out<u32> out_region_code);
    public:
        DEFINE_SERVICE_DISPATCH_TABLE {
            MAKE_SERVICE_COMMAND_META(SetMitmService, GetLanguageCode),
            MAKE_SERVICE_COMMAND_META(SetMitmService, GetRegionCode),
        };
};
