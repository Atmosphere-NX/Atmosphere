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

#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <malloc.h>

#include <switch.h>
#include <stratosphere.hpp>

#include "spl_api_impl.hpp"

#include "spl_random_service.hpp"
#include "spl_general_service.hpp"
#include "spl_crypto_service.hpp"
#include "spl_ssl_service.hpp"
#include "spl_es_service.hpp"
#include "spl_fs_service.hpp"
#include "spl_manu_service.hpp"

#include "spl_deprecated_service.hpp"

extern "C" {
    extern u32 __start__;

    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x28000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    /* Exception handling. */
    alignas(16) u8 __nx_exception_stack[0x1000];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump *ctx);
    void __libstratosphere_exception_handler(AtmosphereFatalErrorContext *ctx);
}

sts::ncm::TitleId __stratosphere_title_id = sts::ncm::TitleId::Spl;

void __libnx_exception_handler(ThreadExceptionDump *ctx) {
    StratosphereCrashHandler(ctx);
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
    SetFirmwareVersionForLibnx();

    /* SPL doesn't really access any services... */
}

void __appExit(void) {
    /* SPL doesn't really access any services... */
}

struct SplServerOptions {
    static constexpr size_t PointerBufferSize = 0x800;
    static constexpr size_t MaxDomains = 0;
    static constexpr size_t MaxDomainObjects = 0;
};

int main(int argc, char **argv)
{
    /* Initialize global context. */
    sts::spl::impl::Initialize();

    /* Create server manager. */
    static auto s_server_manager = WaitableManager<SplServerOptions>(1);

    /* Create services. */
    s_server_manager.AddWaitable(new ServiceServer<sts::spl::RandomService>("csrng", 3));
    if (GetRuntimeFirmwareVersion() >= FirmwareVersion_400) {
        s_server_manager.AddWaitable(new ServiceServer<sts::spl::GeneralService>("spl:", 9));
        s_server_manager.AddWaitable(new ServiceServer<sts::spl::CryptoService>("spl:mig", 6));
        s_server_manager.AddWaitable(new ServiceServer<sts::spl::SslService>("spl:ssl", 2));
        s_server_manager.AddWaitable(new ServiceServer<sts::spl::EsService>("spl:es", 2));
        s_server_manager.AddWaitable(new ServiceServer<sts::spl::FsService>("spl:fs", 2));
        if (GetRuntimeFirmwareVersion() >= FirmwareVersion_500) {
            s_server_manager.AddWaitable(new ServiceServer<sts::spl::ManuService>("spl:manu", 1));
        }
    } else {
        s_server_manager.AddWaitable(new ServiceServer<sts::spl::DeprecatedService>("spl:", 12));
    }

    /* Loop forever, servicing our services. */
    s_server_manager.Process();

    return 0;
}
