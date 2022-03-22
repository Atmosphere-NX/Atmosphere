/*
 * Copyright (c) Atmosphère-NX
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
#include "psc_remote_pm_module.hpp"

namespace ams::psc {

    /* TODO: Nintendo uses sf::ShimLobraryObjectHolder here, we should similarly consider switching. */
    namespace {

        struct PscRemotePmModuleTag;
        using RemoteAllocator     = ams::sf::ExpHeapStaticAllocator<2_KB, PscRemotePmModuleTag>;
        using RemoteObjectFactory = ams::sf::ObjectFactory<typename RemoteAllocator::Policy>;

        class StaticAllocatorInitializer {
            public:
                StaticAllocatorInitializer() {
                    RemoteAllocator::Initialize(lmem::CreateOption_None);
                }
        } g_static_allocator_initializer;

    }

    PmModule::PmModule() : m_intf(nullptr), m_initialized(false), m_reserved(0) { /* ... */ }

    PmModule::~PmModule() {
        if (m_initialized) {
            m_intf = nullptr;
            os::DestroySystemEvent(m_system_event.GetBase());
        }
    }

    Result PmModule::Initialize(const PmModuleId mid, const PmModuleId *dependencies, u32 dependency_count, os::EventClearMode clear_mode) {
        R_UNLESS(!m_initialized, psc::ResultAlreadyInitialized());

        static_assert(sizeof(*dependencies) == sizeof(u32));
        ::PscPmModule module;
        R_TRY(::pscmGetPmModule(std::addressof(module), static_cast<::PscPmModuleId>(mid), reinterpret_cast<const u32 *>(dependencies), dependency_count, clear_mode == os::EventClearMode_AutoClear));

        m_intf = RemoteObjectFactory::CreateSharedEmplaced<psc::sf::IPmModule, RemotePmModule>(module);
        m_system_event.AttachReadableHandle(module.event.revent, false, clear_mode);
        m_initialized = true;
        return ResultSuccess();
    }

    Result PmModule::Finalize() {
        R_UNLESS(m_initialized, psc::ResultNotInitialized());

        R_TRY(m_intf->Finalize());
        m_intf = nullptr;
        os::DestroySystemEvent(m_system_event.GetBase());
        m_initialized = false;
        return ResultSuccess();
    }

    Result PmModule::GetRequest(PmState *out_state, PmFlagSet *out_flags) {
        R_UNLESS(m_initialized, psc::ResultNotInitialized());

        return m_intf->GetRequest(out_state, out_flags);
    }

    Result PmModule::Acknowledge(PmState state, Result res) {
        R_ABORT_UNLESS(res);
        R_UNLESS(m_initialized, psc::ResultNotInitialized());

        if (hos::GetVersion() >= hos::Version_5_1_0) {
            return m_intf->AcknowledgeEx(state);
        } else {
            return m_intf->Acknowledge();
        }
    }

    os::SystemEvent *PmModule::GetEventPointer() {
        return std::addressof(m_system_event);
    }

}