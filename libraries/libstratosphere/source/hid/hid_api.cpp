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
#include <stratosphere.hpp>

namespace ams::hid {

    namespace {

        /* Global lock. */
        os::Mutex g_hid_lock(false);
        bool g_initialized_hid = false;

        /* Set of supported NpadIds (we want to read from any connected controllers). */
        constexpr const HidNpadIdType NpadIdTypes[] = {
            HidNpadIdType_No1,
            HidNpadIdType_No2,
            HidNpadIdType_No3,
            HidNpadIdType_No4,
            HidNpadIdType_No5,
            HidNpadIdType_No6,
            HidNpadIdType_No7,
            HidNpadIdType_No8,
            HidNpadIdType_Handheld,
        };

        constexpr const size_t NumNpadIdTypes = util::size(NpadIdTypes);

        /* Helper. */
        void InitializeHid() {
            R_ABORT_UNLESS(smInitialize());
            ON_SCOPE_EXIT { smExit(); };
            {
                R_ABORT_UNLESS(hidInitialize());
                hidInitializeNpad();
                R_ABORT_UNLESS(hidSetSupportedNpadIdType(NpadIdTypes, NumNpadIdTypes));
                R_ABORT_UNLESS(hidSetSupportedNpadStyleSet(HidNpadStyleSet_NpadStandard | HidNpadStyleTag_NpadSystemExt));
            }
        }

        Result EnsureHidInitialized() {
            if (!g_initialized_hid) {
                if (!serviceIsActive(hidGetServiceSession())) {
                    if (!pm::info::HasLaunchedProgram(ncm::SystemProgramId::Hid)) {
                        return MAKERESULT(Module_Libnx, LibnxError_InitFail_HID);
                    }
                    InitializeHid();
                }
                g_initialized_hid = true;
            }

            return ResultSuccess();
        }

        u64 ReadHidNpad(HidNpadIdType id) {
            HidNpadSystemExtState state;

            size_t count = hidGetNpadStatesSystemExt(id, std::addressof(state), 1);
            if (count != 0 && (state.attributes & HidNpadAttribute_IsConnected)) {
                return state.buttons;
            }

            return 0;
        }

    }

    Result GetKeysHeld(u64 *out) {
        std::scoped_lock lk(g_hid_lock);

        R_TRY(EnsureHidInitialized());

        *out = ReadHidNpad(HidNpadIdType_Handheld);

        for (size_t controller = 0; controller < 8; controller++) {
            *out |= ReadHidNpad(static_cast<HidNpadIdType>(controller));
        }

        return ResultSuccess();
    }

}
