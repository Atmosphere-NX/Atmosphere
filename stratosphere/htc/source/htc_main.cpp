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

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;
    u32 __nx_fs_num_sessions = 1;

    #define INNER_HEAP_SIZE 0x0
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    void *__libnx_alloc(size_t size);
    void *__libnx_aligned_alloc(size_t alignment, size_t size);
    void __libnx_free(void *mem);
}

namespace ams {

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::Htc;

}

using namespace ams;

namespace ams::htc {

    namespace {

        alignas(0x40) constinit u8 g_heap_buffer[4_KB];
        lmem::HeapHandle g_heap_handle;

        void *Allocate(size_t size) {
            return lmem::AllocateFromExpHeap(g_heap_handle, size);
        }

        void Deallocate(void *p, size_t size) {
            return lmem::FreeToExpHeap(g_heap_handle, p);
        }

        void InitializeHeap() {
            /* Setup server allocator. */
            g_heap_handle = lmem::CreateExpHeap(g_heap_buffer, sizeof(g_heap_buffer), lmem::CreateOption_ThreadSafe);
        }

    }

}

void __libnx_initheap(void) {
	void*  addr = nx_inner_heap;
	size_t size = nx_inner_heap_size;

	/* Newlib */
	extern char* fake_heap_start;
	extern char* fake_heap_end;

	fake_heap_start = (char*)addr;
	fake_heap_end   = (char*)addr + size;

    ams::htc::InitializeHeap();
}

void __appInit(void) {
    hos::InitializeForStratosphere();

    fs::SetAllocator(htc::Allocate, htc::Deallocate);

    sm::DoWithSession([&]() {
        R_ABORT_UNLESS(setsysInitialize());
        R_ABORT_UNLESS(fsInitialize());
    });

    R_ABORT_UNLESS(fs::MountSdCard("sdmc"));

    ams::CheckApiVersion();
}

void __appExit(void) {
    fsExit();
    setsysExit();
}

namespace ams {

    void *Malloc(size_t size) {
        AMS_ABORT("ams::Malloc was called");
    }

    void Free(void *ptr) {
        AMS_ABORT("ams::Free was called");
    }

    void *MallocForRapidJson(size_t size) {
        AMS_ABORT("ams::MallocForRapidJson was called");
    }

    void *ReallocForRapidJson(void *ptr, size_t size) {
        AMS_ABORT("ams::ReallocForRapidJson was called");
    }

    void FreeForRapidJson(void *ptr) {
        if (ptr == nullptr) {
            return;
        }
        AMS_ABORT("ams::FreeForRapidJson was called");
    }

}

void *operator new(size_t size) {
    AMS_ABORT("operator new(size_t) was called");
}

void operator delete(void *p) {
    AMS_ABORT("operator delete(void *) was called");
}

void *__libnx_alloc(size_t size) {
    AMS_ABORT("__libnx_alloc was called");
}

void *__libnx_aligned_alloc(size_t alignment, size_t size) {
    AMS_ABORT("__libnx_aligned_alloc was called");
}

void __libnx_free(void *mem) {
    AMS_ABORT("__libnx_free was called");
}

namespace ams::htc {

    namespace {

        constexpr htclow::impl::DriverType DefaultHtclowDriverType = htclow::impl::DriverType::Usb;

        htclow::impl::DriverType GetHtclowDriverType() {
            /* Get the transport type. */
            char transport[0x10];
            if (settings::fwdbg::GetSettingsItemValue(transport, sizeof(transport), "bsp0", "tm_transport") == 0) {
                return DefaultHtclowDriverType;
            }

            /* Make the transport type case insensitive. */
            transport[util::size(transport) - 1] = '\x00';
            for (size_t i = 0; i < util::size(transport); ++i) {
                transport[i] = std::tolower(static_cast<unsigned char>(transport[i]));
            }

            /* Select the transport. */
            if (std::strstr(transport, "usb")) {
                return htclow::impl::DriverType::Usb;
            } else if (std::strstr(transport, "hb")) {
                return htclow::impl::DriverType::HostBridge;
            } else if (std::strstr(transport, "plainchannel")) {
                return htclow::impl::DriverType::PlainChannel;
            } else {
                return DefaultHtclowDriverType;
            }
        }

    }

}

int main(int argc, char **argv)
{
    /* Set thread name. */
    os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(htc, Main));
    AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(htc, Main));

    /* Get and set the default driver type. */
    const auto driver_type = htc::GetHtclowDriverType();
    htclow::HtclowManagerHolder::SetDefaultDriver(driver_type);

    /* Initialize the htclow manager. */
    htclow::HtclowManagerHolder::AddReference();

    /* TODO */

    /* Cleanup */
    return 0;
}

