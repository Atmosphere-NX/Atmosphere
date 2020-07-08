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

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::Spl;

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

    /* SPL doesn't really access any services... */

    ams::CheckApiVersion();
}

void __appExit(void) {
    /* SPL doesn't really access any services... */
}

namespace {

    struct SplServerOptions {
        static constexpr size_t PointerBufferSize = 0x800;
        static constexpr size_t MaxDomains = 0;
        static constexpr size_t MaxDomainObjects = 0;
    };

    constexpr sm::ServiceName RandomServiceName = sm::ServiceName::Encode("csrng");
    constexpr size_t          RandomMaxSessions = 3;

    constexpr sm::ServiceName DeprecatedServiceName = sm::ServiceName::Encode("spl:");
    constexpr size_t          DeprecatedMaxSessions = 13;

    constexpr sm::ServiceName GeneralServiceName = sm::ServiceName::Encode("spl:");
    constexpr size_t          GeneralMaxSessions = 7;

    constexpr sm::ServiceName CryptoServiceName = sm::ServiceName::Encode("spl:mig");
    constexpr size_t          CryptoMaxSessions = 7;

    constexpr sm::ServiceName SslServiceName = sm::ServiceName::Encode("spl:ssl");
    constexpr size_t          SslMaxSessions = 2;

    constexpr sm::ServiceName EsServiceName = sm::ServiceName::Encode("spl:es");
    constexpr size_t          EsMaxSessions = 2;

    constexpr sm::ServiceName FsServiceName = sm::ServiceName::Encode("spl:fs");
    constexpr size_t          FsMaxSessions = 3;

    constexpr sm::ServiceName ManuServiceName = sm::ServiceName::Encode("spl:manu");
    constexpr size_t          ManuMaxSessions = 1;

    /* csrng, spl:, spl:mig, spl:ssl, spl:es, spl:fs, spl:manu. */
    /* TODO: Consider max sessions enforcement? */
    constexpr size_t NumServers  = 7;
    constexpr size_t ModernMaxSessions = GeneralMaxSessions + CryptoMaxSessions + SslMaxSessions + EsMaxSessions + FsMaxSessions + ManuMaxSessions;
    constexpr size_t NumSessions = RandomMaxSessions + std::max(DeprecatedMaxSessions, ModernMaxSessions) + 1;
    sf::hipc::ServerManager<NumServers, SplServerOptions, NumSessions> g_server_manager;

}

int main(int argc, char **argv)
{
    /* Set thread name. */
    os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(spl, Main));
    AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(spl, Main));

    /* Initialize global context. */
    spl::impl::Initialize();

    /* Create services. */
    R_ABORT_UNLESS((g_server_manager.RegisterServer<spl::impl::IRandomInterface, spl::RandomService>(RandomServiceName, RandomMaxSessions)));
    if (hos::GetVersion() >= hos::Version_4_0_0) {
        R_ABORT_UNLESS((g_server_manager.RegisterServer<spl::impl::IGeneralInterface, spl::GeneralService>(GeneralServiceName, GeneralMaxSessions)));
        R_ABORT_UNLESS((g_server_manager.RegisterServer<spl::impl::ICryptoInterface,  spl::CryptoService>(CryptoServiceName, CryptoMaxSessions)));
        R_ABORT_UNLESS((g_server_manager.RegisterServer<spl::impl::ISslInterface,     spl::SslService>(SslServiceName, SslMaxSessions)));
        R_ABORT_UNLESS((g_server_manager.RegisterServer<spl::impl::IEsInterface,      spl::EsService>(EsServiceName, EsMaxSessions)));
        R_ABORT_UNLESS((g_server_manager.RegisterServer<spl::impl::IFsInterface,      spl::FsService>(FsServiceName, FsMaxSessions)));
        if (hos::GetVersion() >= hos::Version_5_0_0) {
            R_ABORT_UNLESS((g_server_manager.RegisterServer<spl::impl::IManuInterface, spl::ManuService>(ManuServiceName, ManuMaxSessions)));
        }
    } else {
        R_ABORT_UNLESS((g_server_manager.RegisterServer<spl::impl::IDeprecatedGeneralInterface, spl::DeprecatedService>(DeprecatedServiceName, DeprecatedMaxSessions)));
    }

    /* Loop forever, servicing our services. */
    g_server_manager.LoopProcess();

    return 0;
}
