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
#include "pgl_remote_event_observer.hpp"

namespace ams::pgl {

    namespace {

        struct PglEventObserverAllocator;
        using RemoteAllocator     = ams::sf::ExpHeapStaticAllocator<1_KB, PglEventObserverAllocator>;
        using RemoteObjectFactory = ams::sf::ObjectFactory<typename RemoteAllocator::Policy>;

        class StaticAllocatorInitializer {
            public:
                StaticAllocatorInitializer() {
                    RemoteAllocator::Initialize(lmem::CreateOption_None);
                }
        } g_static_allocator_initializer;

        template<typename T, typename... Args>
        T *AllocateFromStaticExpHeap(Args &&... args) {
            T * const object = static_cast<T *>(RemoteAllocator::Allocate(sizeof(T)));
            if (AMS_LIKELY(object != nullptr)) {
                std::construct_at(object, std::forward<Args>(args)...);
            }
            return object;
        }

        template<typename T>
        void FreeToStaticExpHeap(T *object) {
            return RemoteAllocator::Deallocate(object, sizeof(T));
        }

        template<typename T, typename... Args> requires std::derived_from<T, impl::EventObserverInterface>
        EventObserver::UniquePtr MakeUniqueFromStaticExpHeap(Args &&... args) {
            return EventObserver::UniquePtr{AllocateFromStaticExpHeap<T>(std::forward<Args>(args)...)};
        }

    }

    void EventObserver::Deleter::operator()(impl::EventObserverInterface *obj) {
        FreeToStaticExpHeap(obj);
    }

    #if defined(ATMOSPHERE_OS_HORIZON)
    Result Initialize() {
        R_RETURN(::pglInitialize());
    }

    void Finalize() {
        return pglExit();
    }

    Result LaunchProgram(os::ProcessId *out, const ncm::ProgramLocation &loc, u32 process_flags, u8 pgl_flags) {
        static_assert(sizeof(*out) == sizeof(u64));
        static_assert(sizeof(loc)  == sizeof(::NcmProgramLocation));
        R_RETURN(::pglLaunchProgram(reinterpret_cast<u64 *>(out), reinterpret_cast<const ::NcmProgramLocation *>(std::addressof(loc)), process_flags, pgl_flags));
    }

    Result TerminateProcess(os::ProcessId process_id) {
        R_RETURN(::pglTerminateProcess(static_cast<u64>(process_id)));
    }

    Result LaunchProgramFromHost(os::ProcessId *out, const char *content_path, u32 process_flags) {
        static_assert(sizeof(*out) == sizeof(u64));
        R_RETURN(::pglLaunchProgramFromHost(reinterpret_cast<u64 *>(out), content_path, process_flags));
    }

    Result GetHostProgramLaunchProperty(pgl::ProgramLaunchProperty *out, const char *content_path) {
        static_assert(sizeof(*out) == sizeof(::PglProgramLaunchProperty));
        R_RETURN(::pglGetHostProgramLaunchProperty(reinterpret_cast<::PglProgramLaunchProperty *>(out), content_path));
    }

    Result GetRunningApplicationProcessId(os::ProcessId *out) {
        static_assert(sizeof(*out) == sizeof(u64));
        R_RETURN(::pglGetRunningApplicationProcessId(reinterpret_cast<u64 *>(out)));
    }

    Result BoostSystemMemoryResourceLimit(u64 size) {
        R_RETURN(::pglBoostSystemMemoryResourceLimit(size));
    }

    Result IsRunningProcess(bool *out, os::ProcessId process_id) {
        R_RETURN(::pglIsRunningProcess(out, static_cast<u64>(process_id)));
    }

    Result EnableApplicationCrashReport(bool enabled) {
        R_RETURN(::pglEnableApplicationCrashReport(enabled));
    }

    Result IsApplicationCrashReportEnabled(bool *out) {
        R_RETURN(::pglIsApplicationCrashReportEnabled(out));
    }

    Result EnableApplicationAllThreadDumpOnCrash(bool enabled) {
        R_RETURN(::pglEnableApplicationAllThreadDumpOnCrash(enabled));
    }

    Result TriggerSnapShotDumper(const char *arg, SnapShotDumpType dump_type) {
        R_RETURN(::pglTriggerSnapShotDumper(static_cast<::PglSnapShotDumpType>(dump_type), arg));
    }

    Result CreateShellEvent(pgl::EventObserver *out) {
        ::PglEventObserver obs;
        R_TRY(::pglCreateShellEvent(std::addressof(obs)));

        if (hos::GetVersion() >= hos::Version_12_0_0) {
            auto observer_holder = MakeUniqueFromStaticExpHeap<impl::EventObserverByTipc<RemoteEventObserver>>(obs);
            R_UNLESS(observer_holder != nullptr, pgl::ResultOutOfMemory());

            *out = pgl::EventObserver(std::move(observer_holder));
        } else {
            auto remote_observer = RemoteObjectFactory::CreateSharedEmplaced<pgl::sf::IEventObserver, RemoteEventObserver>(obs);
            R_UNLESS(remote_observer != nullptr, pgl::ResultOutOfMemory());

            auto observer_holder = MakeUniqueFromStaticExpHeap<impl::EventObserverByCmif>(std::move(remote_observer));
            R_UNLESS(observer_holder != nullptr, pgl::ResultOutOfMemory());

            *out = pgl::EventObserver(std::move(observer_holder));
        }

        R_SUCCEED();
    }
    
    Result EnableApplicationCrashReport2(os::ProcessId process_id, bool enabled) {
        R_RETURN(::pglEnableApplicationCrashReport2(static_cast<u64>(process_id), enabled));
    }
    #else
    Result Initialize() {
        AMS_ABORT("TODO");
    }

    void Finalize() {
        AMS_ABORT("TODO");
    }

    Result LaunchProgram(os::ProcessId *out, const ncm::ProgramLocation &loc, u32 process_flags, u8 pgl_flags) {
        AMS_UNUSED(out, loc, process_flags, pgl_flags);
        AMS_ABORT("TODO");
    }

    Result TerminateProcess(os::ProcessId process_id) {
        AMS_UNUSED(process_id);
        AMS_ABORT("TODO");
    }

    Result LaunchProgramFromHost(os::ProcessId *out, const char *content_path, u32 process_flags) {
        AMS_UNUSED(out, content_path, process_flags);
        AMS_ABORT("TODO");
    }

    Result GetHostProgramLaunchProperty(pgl::ProgramLaunchProperty *out, const char *content_path) {
        AMS_UNUSED(out, content_path);
        AMS_ABORT("TODO");
    }

    Result GetRunningApplicationProcessId(os::ProcessId *out) {
        AMS_UNUSED(out);
        AMS_ABORT("TODO");
    }

    Result BoostSystemMemoryResourceLimit(u64 size) {
        AMS_UNUSED(size);
        AMS_ABORT("TODO");
    }

    Result IsRunningProcess(bool *out, os::ProcessId process_id) {
        AMS_UNUSED(out, process_id);
        AMS_ABORT("TODO");
    }

    Result EnableApplicationCrashReport(bool enabled) {
        AMS_UNUSED(enabled);
        AMS_ABORT("TODO");
    }

    Result IsApplicationCrashReportEnabled(bool *out) {
        AMS_UNUSED(out);
        AMS_ABORT("TODO");
    }

    Result EnableApplicationAllThreadDumpOnCrash(bool enabled) {
        AMS_UNUSED(enabled);
        AMS_ABORT("TODO");
    }

    Result TriggerSnapShotDumper(const char *arg, SnapShotDumpType dump_type) {
        AMS_UNUSED(arg, dump_type);
        AMS_ABORT("TODO");
    }

    Result CreateShellEvent(pgl::EventObserver *out) {
        AMS_UNUSED(out);
        AMS_ABORT("TODO");
    }
    
    Result EnableApplicationCrashReport2(os::ProcessId process_id, bool enabled) {
        AMS_UNUSED(process_id, enabled);
        AMS_ABORT("TODO");
    }
    #endif

}