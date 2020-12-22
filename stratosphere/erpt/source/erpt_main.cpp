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

    #define INNER_HEAP_SIZE 0x4000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    /* Exception handling. */
    alignas(16) u8 __nx_exception_stack[ams::os::MemoryPageSize];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump *ctx);
}

namespace ams {

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::Erpt;

    namespace result {

        bool CallFatalOnResultAssertion = false;

    }

}

using namespace ams;

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    ams::CrashHandler(ctx);
}

void __libnx_initheap(void) {
	void*  addr = nx_inner_heap;
	size_t size = nx_inner_heap_size;

	/* Newlib */
	extern char* fake_heap_start;
	extern char* fake_heap_end;

	fake_heap_start = (char*)addr;
	fake_heap_end   = (char*)addr + size;
}

void __appInit(void) {
    hos::InitializeForStratosphere();

    sm::DoWithSession([&]() {
        R_ABORT_UNLESS(setInitialize());
        R_ABORT_UNLESS(setsysInitialize());
        R_ABORT_UNLESS(pscmInitialize());
        R_ABORT_UNLESS(time::Initialize());
        if (hos::GetVersion() >= hos::Version_11_0_0) {
            R_ABORT_UNLESS(ectxrInitialize());
        }
        R_ABORT_UNLESS(fsInitialize());
    });

    ams::CheckApiVersion();
}

void __appExit(void) {
    fsExit();
    time::Finalize();
    pscmExit();
    setsysExit();
    setExit();
}

namespace ams::erpt {

    namespace {

        constexpr size_t MemoryHeapSize = 196_KB;
        alignas(os::MemoryPageSize) u8 g_memory_heap[MemoryHeapSize];

    }

    int MakeProductModelString(char *dst, size_t dst_size, settings::system::ProductModel model) {
        switch (model) {
            case settings::system::ProductModel_Invalid: return util::Strlcpy(dst, "Invalid", static_cast<int>(dst_size));
            case settings::system::ProductModel_Nx:      return util::Strlcpy(dst, "NX", static_cast<int>(dst_size));
            default:                                     return std::snprintf(dst, dst_size, "%d", static_cast<int>(model));
        }
    }

    const char *GetRegionString(settings::system::RegionCode code) {
        switch (code) {
            case settings::system::RegionCode_Japan:               return "Japan";
            case settings::system::RegionCode_Usa:                 return "Usa";
            case settings::system::RegionCode_Europe:              return "Europe";
            case settings::system::RegionCode_Australia:           return "Australia";
            case settings::system::RegionCode_HongKongTaiwanKorea: return "HongKongTaiwanKorea";
            case settings::system::RegionCode_China:               return "China";
            default:                                               return "RegionUnknown";
        }
    }

}

int main(int argc, char **argv)
{
    /* Set thread name. */
    os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(erpt, Main));
    AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(erpt, Main));

    /* Set the memory heap for erpt::srv namespace. */
    R_ABORT_UNLESS(erpt::srv::Initialize(erpt::g_memory_heap, erpt::MemoryHeapSize));

    /* Atmosphere always wants to redirect new reports to the SD card, to prevent them from being logged. */
    erpt::srv::SetRedirectNewReportsToSdCard(true);

    /* Configure the OS version. */
    {
        settings::system::FirmwareVersion firmware_version = {};
        settings::system::SerialNumber serial_number = {};
        settings::system::GetFirmwareVersion(std::addressof(firmware_version));
        settings::system::GetSerialNumber(std::addressof(serial_number));

        char os_private[0x60];
        const auto os_priv_len = std::snprintf(os_private, sizeof(os_private), "%s (%.8s)", firmware_version.display_name, firmware_version.revision);
        AMS_ASSERT(static_cast<size_t>(os_priv_len) < sizeof(os_private));

        R_ABORT_UNLESS(erpt::srv::SetSerialNumberAndOsVersion(serial_number.str,
                                                              strnlen(serial_number.str, sizeof(serial_number.str) - 1) + 1,
                                                              firmware_version.display_version,
                                                              strnlen(firmware_version.display_version, sizeof(firmware_version.display_version) - 1) + 1,
                                                              os_private,
                                                              strnlen(os_private, sizeof(os_private) - 1) + 1));
    }

    /* Configure the product model. */
    {
        char product_model[0x10];
        const auto pm_len = erpt::MakeProductModelString(product_model, sizeof(product_model), settings::system::GetProductModel());
        AMS_ASSERT(static_cast<size_t>(pm_len) < sizeof(product_model));
        R_ABORT_UNLESS(erpt::srv::SetProductModel(product_model, static_cast<u32>(std::strlen(product_model))));
    }

    /* Configure the region. */
    {
        settings::system::RegionCode code;
        settings::system::GetRegionCode(std::addressof(code));
        const char *region_str = erpt::GetRegionString(code);
        R_ABORT_UNLESS(erpt::srv::SetRegionSetting(region_str, static_cast<u32>(std::strlen(region_str))));
    }

    /* Start the erpt server. */
    R_ABORT_UNLESS(erpt::srv::InitializeAndStartService());

    /* Wait forever. */
    erpt::srv::Wait();

    /* Cleanup */
    return 0;
}

