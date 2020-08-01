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

namespace ams::kern {

    /* Static initializations. */
    constinit bool KTrace::s_is_active = false;

    namespace {

        constinit KSpinLock g_ktrace_lock;
        constinit KVirtualAddress g_ktrace_buffer_address = Null<KVirtualAddress>;
        constinit size_t g_ktrace_buffer_size = 0;

        struct KTraceHeader {
            u32 magic;
            u32 offset;
            u32 index;
            u32 count;

            static constexpr u32 Magic = util::FourCC<'K','T','R','0'>::Code;
        };
        static_assert(util::is_pod<KTraceHeader>::value);

        struct KTraceRecord {
            u8 core_id;
            u8 type;
            u16 process_id;
            u32 thread_id;
            u64 tick;
            u64 data[6];
        };
        static_assert(util::is_pod<KTraceRecord>::value);
        static_assert(sizeof(KTraceRecord) == 0x40);

    }

    void KTrace::Initialize(KVirtualAddress address, size_t size) {
        /* Only perform tracing when on development hardware. */
        if (KTargetSystem::IsDebugMode()) {
            const size_t offset = util::AlignUp(sizeof(KTraceHeader), sizeof(KTraceRecord));
            if (offset < size) {
                /* Clear the trace buffer. */
                std::memset(GetVoidPointer(address), 0, size);

                /* Initialize the KTrace header. */
                KTraceHeader *header = GetPointer<KTraceHeader>(address);
                header->magic  = KTraceHeader::Magic;
                header->offset = offset;
                header->index  = 0;
                header->count  = (size - offset) / sizeof(KTraceRecord);

                /* Set the global data. */
                g_ktrace_buffer_address = address;
                g_ktrace_buffer_size    = size;
            }
        }
    }

    void KTrace::Start() {
        if (g_ktrace_buffer_address != Null<KVirtualAddress>) {
            /* Get exclusive access to the trace buffer. */
            KScopedInterruptDisable di;
            KScopedSpinLock lk(g_ktrace_lock);

            /* Reset the header. */
            KTraceHeader *header = GetPointer<KTraceHeader>(g_ktrace_buffer_address);
            header->index = 0;

            /* Reset the records. */
            KTraceRecord *records = GetPointer<KTraceRecord>(g_ktrace_buffer_address + header->offset);
            std::memset(records, 0, sizeof(*records) * header->count);

            /* Note that we're active. */
            s_is_active = true;
        }
    }

    void KTrace::Stop() {
        if (g_ktrace_buffer_address != Null<KVirtualAddress>) {
            /* Get exclusive access to the trace buffer. */
            KScopedInterruptDisable di;
            KScopedSpinLock lk(g_ktrace_lock);

            /* Note that we're paused. */
            s_is_active = false;
        }
    }

}
