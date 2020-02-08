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
#include <mesosphere.hpp>

namespace ams::kern::svc {

    /* =============================    Common    ============================= */

    namespace {



    }

    /* =============================    64 ABI    ============================= */

    void FlushEntireDataCache64() {
        MESOSPHERE_PANIC("Stubbed SvcFlushEntireDataCache64 was called.");
    }

    Result FlushDataCache64(ams::svc::Address address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcFlushDataCache64 was called.");
    }

    Result InvalidateProcessDataCache64(ams::svc::Handle process_handle, uint64_t address, uint64_t size) {
        MESOSPHERE_PANIC("Stubbed SvcInvalidateProcessDataCache64 was called.");
    }

    Result StoreProcessDataCache64(ams::svc::Handle process_handle, uint64_t address, uint64_t size) {
        MESOSPHERE_PANIC("Stubbed SvcStoreProcessDataCache64 was called.");
    }

    Result FlushProcessDataCache64(ams::svc::Handle process_handle, uint64_t address, uint64_t size) {
        MESOSPHERE_PANIC("Stubbed SvcFlushProcessDataCache64 was called.");
    }

    /* ============================= 64From32 ABI ============================= */

    void FlushEntireDataCache64From32() {
        MESOSPHERE_PANIC("Stubbed SvcFlushEntireDataCache64From32 was called.");
    }

    Result FlushDataCache64From32(ams::svc::Address address, ams::svc::Size size) {
        MESOSPHERE_PANIC("Stubbed SvcFlushDataCache64From32 was called.");
    }

    Result InvalidateProcessDataCache64From32(ams::svc::Handle process_handle, uint64_t address, uint64_t size) {
        MESOSPHERE_PANIC("Stubbed SvcInvalidateProcessDataCache64From32 was called.");
    }

    Result StoreProcessDataCache64From32(ams::svc::Handle process_handle, uint64_t address, uint64_t size) {
        MESOSPHERE_PANIC("Stubbed SvcStoreProcessDataCache64From32 was called.");
    }

    Result FlushProcessDataCache64From32(ams::svc::Handle process_handle, uint64_t address, uint64_t size) {
        MESOSPHERE_PANIC("Stubbed SvcFlushProcessDataCache64From32 was called.");
    }

}
