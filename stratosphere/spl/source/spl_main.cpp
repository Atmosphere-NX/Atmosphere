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

    #define INNER_HEAP_SIZE 0x0
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char   nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void);
    void __appInit(void);
    void __appExit(void);

    /* Exception handling. */
    alignas(16) u8 __nx_exception_stack[ams::os::MemoryPageSize];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump *ctx);

    void *__libnx_alloc(size_t size);
    void *__libnx_aligned_alloc(size_t alignment, size_t size);
    void __libnx_free(void *mem);
}

namespace ams {

    ncm::ProgramId CurrentProgramId = ncm::SystemProgramId::Spl;

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

    enum PortIndex {
        PortIndex_General,
        PortIndex_Random,
        PortIndex_Crypto,
        PortIndex_Fs,
        PortIndex_Ssl,
        PortIndex_Es,
        PortIndex_Manu,
        PortIndex_Count,
    };

    constexpr sm::ServiceName RandomServiceName = sm::ServiceName::Encode("csrng");
    constexpr size_t          RandomMaxSessions = 3;

    constexpr sm::ServiceName GeneralServiceName = sm::ServiceName::Encode("spl:");
    constexpr size_t          DeprecatedMaxSessions = 13;
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
    constexpr size_t ModernMaxSessions = GeneralMaxSessions + CryptoMaxSessions + SslMaxSessions + EsMaxSessions + FsMaxSessions + ManuMaxSessions;
    constexpr size_t NumSessions = RandomMaxSessions + std::max(DeprecatedMaxSessions, ModernMaxSessions) + 1;

    class ServerManager final : public sf::hipc::ServerManager<PortIndex_Count, SplServerOptions, NumSessions> {
        private:
            sf::ExpHeapAllocator *m_allocator;
            spl::GeneralService m_general_service;
            sf::UnmanagedServiceObjectByPointer<spl::impl::IGeneralInterface, spl::GeneralService> m_general_service_object;
            spl::RandomService m_random_service;
            sf::UnmanagedServiceObjectByPointer<spl::impl::IRandomInterface, spl::RandomService> m_random_service_object;
        public:
            ServerManager(sf::ExpHeapAllocator *allocator) : m_allocator(allocator), m_general_service(), m_general_service_object(std::addressof(m_general_service)), m_random_service(), m_random_service_object(std::addressof(m_random_service)) {
                /* ... */
            }
        private:
            virtual ams::Result OnNeedsToAccept(int port_index, Server *server) override;
    };

    using Allocator     = sf::ExpHeapAllocator;
    using ObjectFactory = sf::ObjectFactory<sf::ExpHeapAllocator::Policy>;

    alignas(0x40) constinit u8 g_server_allocator_buffer[8_KB];
    Allocator g_server_allocator;

    ServerManager g_server_manager(std::addressof(g_server_allocator));

    ams::Result ServerManager::OnNeedsToAccept(int port_index, Server *server) {
        switch (port_index) {
            case PortIndex_General:
                if (hos::GetVersion() >= hos::Version_4_0_0) {
                    return this->AcceptImpl(server, m_general_service_object.GetShared());
                } else {
                    return this->AcceptImpl(server, ObjectFactory::CreateSharedEmplaced<spl::impl::IDeprecatedGeneralInterface, spl::DeprecatedService>(m_allocator));
                }
            case PortIndex_Random:
                return this->AcceptImpl(server, m_random_service_object.GetShared());
            case PortIndex_Crypto:
                return this->AcceptImpl(server, ObjectFactory::CreateSharedEmplaced<spl::impl::ICryptoInterface, spl::CryptoService>(m_allocator));
            case PortIndex_Fs:
                return this->AcceptImpl(server, ObjectFactory::CreateSharedEmplaced<spl::impl::IFsInterface,     spl::FsService>(m_allocator));
            case PortIndex_Ssl:
                return this->AcceptImpl(server, ObjectFactory::CreateSharedEmplaced<spl::impl::ISslInterface,    spl::SslService>(m_allocator));
            case PortIndex_Es:
                return this->AcceptImpl(server, ObjectFactory::CreateSharedEmplaced<spl::impl::IEsInterface,     spl::EsService>(m_allocator));
            case PortIndex_Manu:
                return this->AcceptImpl(server, ObjectFactory::CreateSharedEmplaced<spl::impl::IManuInterface,   spl::ManuService>(m_allocator));
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
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

int main(int argc, char **argv)
{
    /* Set thread name. */
    os::SetThreadNamePointer(os::GetCurrentThread(), AMS_GET_SYSTEM_THREAD_NAME(spl, Main));
    AMS_ASSERT(os::GetThreadPriority(os::GetCurrentThread()) == AMS_GET_SYSTEM_THREAD_PRIORITY(spl, Main));

    /* Setup server allocator. */
    g_server_allocator.Attach(lmem::CreateExpHeap(g_server_allocator_buffer, sizeof(g_server_allocator_buffer), lmem::CreateOption_None));

    /* Initialize global context. */
    spl::impl::Initialize();

    /* Create services. */
    const auto fw_ver = hos::GetVersion();
    R_ABORT_UNLESS(g_server_manager.RegisterServer(PortIndex_General, GeneralServiceName, fw_ver >= hos::Version_4_0_0 ? GeneralMaxSessions : DeprecatedMaxSessions));
    R_ABORT_UNLESS(g_server_manager.RegisterServer(PortIndex_Random, RandomServiceName, RandomMaxSessions));
    if (fw_ver >= hos::Version_4_0_0) {
        R_ABORT_UNLESS(g_server_manager.RegisterServer(PortIndex_Crypto, CryptoServiceName, CryptoMaxSessions));
        R_ABORT_UNLESS(g_server_manager.RegisterServer(PortIndex_Fs,     FsServiceName,     FsMaxSessions));
        R_ABORT_UNLESS(g_server_manager.RegisterServer(PortIndex_Ssl,    SslServiceName,    SslMaxSessions));
        R_ABORT_UNLESS(g_server_manager.RegisterServer(PortIndex_Es,     EsServiceName,     EsMaxSessions));
        if (fw_ver >= hos::Version_5_0_0) {
            g_server_manager.RegisterServer(PortIndex_Manu, ManuServiceName, ManuMaxSessions);
        }
    }

    /* Loop forever, servicing our services. */
    g_server_manager.LoopProcess();

    return 0;
}
