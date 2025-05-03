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
#pragma once
#include <vapours.hpp>

namespace ams::impl {

    struct SystemThreadDefinition {
        s32 priority;
        const char *name;
    };

    #define AMS_DEFINE_SYSTEM_THREAD(__AMS_THREAD_PRIORITY__, __AMS_MODULE__, __AMS_THREAD_NAME__) \
        constexpr inline const ::ams::impl::SystemThreadDefinition SystemThreadDefinition_##__AMS_MODULE__##_##__AMS_THREAD_NAME__ = { __AMS_THREAD_PRIORITY__, "ams." # __AMS_MODULE__ "." #__AMS_THREAD_NAME__ }

    /* sm. */
    AMS_DEFINE_SYSTEM_THREAD(-1, sm, Main);
    AMS_DEFINE_SYSTEM_THREAD(-1, sm, DispatcherThread);

    /* spl. */
    AMS_DEFINE_SYSTEM_THREAD(-1, spl, Main);

    /* Loader. */
    AMS_DEFINE_SYSTEM_THREAD(21, ldr, Main);

    /* Process Manager. */
    AMS_DEFINE_SYSTEM_THREAD(21, pm, Main);
    AMS_DEFINE_SYSTEM_THREAD(21, pm, ProcessTrack);


    /* NCM. */
    AMS_DEFINE_SYSTEM_THREAD(21, ncm, MainWaitThreads);
    AMS_DEFINE_SYSTEM_THREAD(21, ncm, ContentManagerServerIpcSession);
    AMS_DEFINE_SYSTEM_THREAD(21, ncm, LocationResolverServerIpcSession);

    /* FS. */
    AMS_DEFINE_SYSTEM_THREAD(11, sdmmc, DeviceDetector);
    AMS_DEFINE_SYSTEM_THREAD(16, fs,    WorkerThreadPool);
    AMS_DEFINE_SYSTEM_THREAD(17, fs,    Main);
    AMS_DEFINE_SYSTEM_THREAD(17, fs,    WorkerRealTimeAccess);
    AMS_DEFINE_SYSTEM_THREAD(18, fs,    WorkerNormalPriorityAccess);
    AMS_DEFINE_SYSTEM_THREAD(19, fs,    WorkerLowPriorityAccess);
    AMS_DEFINE_SYSTEM_THREAD(30, fs,    WorkerBackgroundAccess);
    AMS_DEFINE_SYSTEM_THREAD(30, fs,    PatrolReader);

    /* Boot. */
    AMS_DEFINE_SYSTEM_THREAD(-1, boot, Main);

    /* Mitm. */
    AMS_DEFINE_SYSTEM_THREAD(-7, mitm,            InitializeThread);
    AMS_DEFINE_SYSTEM_THREAD(-1, mitm_sf,         QueryServerProcessThread);
    AMS_DEFINE_SYSTEM_THREAD(16, mitm_fs,         RomFileSystemInitializeThread);
    AMS_DEFINE_SYSTEM_THREAD(16, mitm_fs,         RomFileSystemFinalizeThread);
    AMS_DEFINE_SYSTEM_THREAD(21, mitm,            DebugThrowThread);
    AMS_DEFINE_SYSTEM_THREAD(21, mitm_sysupdater, IpcServer);
    AMS_DEFINE_SYSTEM_THREAD(21, mitm_sysupdater, AsyncPrepareSdCardUpdateTask);

    /* boot2. */
    AMS_DEFINE_SYSTEM_THREAD(20, boot2, Main);

    /* LogManager. */
    AMS_DEFINE_SYSTEM_THREAD(10, LogManager, MainThread);
    AMS_DEFINE_SYSTEM_THREAD(10, lm, IpcServer);
    AMS_DEFINE_SYSTEM_THREAD(10, lm, Flush);
    AMS_DEFINE_SYSTEM_THREAD(10, lm, HtcsConnection);

    /* dmnt. */
    AMS_DEFINE_SYSTEM_THREAD(-3, dmnt, MultiCoreEventManager);
    AMS_DEFINE_SYSTEM_THREAD(-1, dmnt, CheatDebugEvents);
    AMS_DEFINE_SYSTEM_THREAD(-1, dmnt, MultiCoreBP);
    AMS_DEFINE_SYSTEM_THREAD(11, dmnt, Main);
    AMS_DEFINE_SYSTEM_THREAD(11, dmnt, Ipc);
    AMS_DEFINE_SYSTEM_THREAD(11, dmnt, CheatDetect);
    AMS_DEFINE_SYSTEM_THREAD(20, dmnt, CheatVirtualMachine);

    /* fatal */
    AMS_DEFINE_SYSTEM_THREAD(-13, fatal, Main);
    AMS_DEFINE_SYSTEM_THREAD(-13, fatalsrv, FatalTaskThread);
    AMS_DEFINE_SYSTEM_THREAD(  9, fatalsrv, IpcDispatcher);

    /* creport. */
    AMS_DEFINE_SYSTEM_THREAD(16, creport, Main);

    /* ro. */
    AMS_DEFINE_SYSTEM_THREAD(21, ro, Main);

    /* gpio. */
    AMS_DEFINE_SYSTEM_THREAD(-12, gpio, InterruptHandler);

    /* bpc. */
    AMS_DEFINE_SYSTEM_THREAD(4, bpc, IpcServer);

    /* powctl. */
    AMS_DEFINE_SYSTEM_THREAD(9, powctl, InterruptHandler);

    /* hid. */
    AMS_DEFINE_SYSTEM_THREAD(-10, hid, IpcServer);

    /* ns.*/
    AMS_DEFINE_SYSTEM_THREAD(21, ns, ApplicationManagerIpcSession);
    AMS_DEFINE_SYSTEM_THREAD(21, nssrv, AsyncPrepareCardUpdateTask);

    /* settings. */
    AMS_DEFINE_SYSTEM_THREAD(21, settings, Main);
    AMS_DEFINE_SYSTEM_THREAD(21, settings, IpcServer);
    AMS_DEFINE_SYSTEM_THREAD(21, settings, LazyWriter);

    /* erpt. */
    AMS_DEFINE_SYSTEM_THREAD(21, erpt, Main);
    AMS_DEFINE_SYSTEM_THREAD(21, erpt, IpcServer);

    /* socket. */
    AMS_DEFINE_SYSTEM_THREAD(29, socket, ResolverIpcServer);

    /* jpegdec. */
    AMS_DEFINE_SYSTEM_THREAD(21, jpegdec, Main);

    /* pgl. */
    AMS_DEFINE_SYSTEM_THREAD(21, pgl, Main);
    AMS_DEFINE_SYSTEM_THREAD(21, pgl, ProcessControlTask);

    /* htc. */
    AMS_DEFINE_SYSTEM_THREAD(10, htc, Main);
    AMS_DEFINE_SYSTEM_THREAD(10, htc, HtcIpc);
    AMS_DEFINE_SYSTEM_THREAD(10, htc, HtcsIpc);
    AMS_DEFINE_SYSTEM_THREAD(10, htc, HtcsMonitor);
    AMS_DEFINE_SYSTEM_THREAD(10, htc, HtcfsIpc);
    AMS_DEFINE_SYSTEM_THREAD(10, htc, HtcfsMonitor);
    AMS_DEFINE_SYSTEM_THREAD(10, htc, HtclowDiscovery);
    AMS_DEFINE_SYSTEM_THREAD(10, htc, HtclowTcpServer);
    AMS_DEFINE_SYSTEM_THREAD(10, htc, HtclowUsbIndication);
    AMS_DEFINE_SYSTEM_THREAD(10, htc, HtclowListen);
    AMS_DEFINE_SYSTEM_THREAD(10, htc, HtclowObserver);
    AMS_DEFINE_SYSTEM_THREAD(10, htc, HtclowSend);
    AMS_DEFINE_SYSTEM_THREAD(10, htc, HtclowReceive);
    AMS_DEFINE_SYSTEM_THREAD(10, htc, Htcmisc);
    AMS_DEFINE_SYSTEM_THREAD(10, htc, HtcmiscReceive);
    AMS_DEFINE_SYSTEM_THREAD(10, htc, HtcmiscSend);
    AMS_DEFINE_SYSTEM_THREAD(10, htc, HtcObserver);

    AMS_DEFINE_SYSTEM_THREAD(10, tma, BridgePcieDriver);

    /* cs/scs. */
    AMS_DEFINE_SYSTEM_THREAD(20, cs, Main);
    AMS_DEFINE_SYSTEM_THREAD(20, cs, HidctlService);
    AMS_DEFINE_SYSTEM_THREAD(20, cs, HidctlLegacyServer);
    AMS_DEFINE_SYSTEM_THREAD(20, cs, AudioServer);
    AMS_DEFINE_SYSTEM_THREAD(10, cs, GrcVideoSender);
    AMS_DEFINE_SYSTEM_THREAD(10, cs, GrcVideoReader);
    AMS_DEFINE_SYSTEM_THREAD(10, cs, GrcAudioSender);
    AMS_DEFINE_SYSTEM_THREAD(10, cs, GrcAudioReader);

    AMS_DEFINE_SYSTEM_THREAD(21, scs, ShellServer);
    AMS_DEFINE_SYSTEM_THREAD(21, scs, ShellEventHandler);

    /* DevServer/TioServer. */
    AMS_DEFINE_SYSTEM_THREAD(21, TioServer, Main);
    AMS_DEFINE_SYSTEM_THREAD(21, TioServer, FileServerHtcsServer);
    AMS_DEFINE_SYSTEM_THREAD(21, TioServer, SdCardObserver);

    AMS_DEFINE_SYSTEM_THREAD(16, memlet, Main);

    /* ServiceProfile */
    AMS_DEFINE_SYSTEM_THREAD(-1, sprofile, IpcServer);


    #undef AMS_DEFINE_SYSTEM_THREAD

}

#define AMS_GET_SYSTEM_THREAD_PRIORITY(__AMS_MODULE__, __AMS_THREAD_NAME__) ( ::ams::impl::SystemThreadDefinition_##__AMS_MODULE__##_##__AMS_THREAD_NAME__ ).priority
#define AMS_GET_SYSTEM_THREAD_NAME(__AMS_MODULE__, __AMS_THREAD_NAME__)     ( ::ams::impl::SystemThreadDefinition_##__AMS_MODULE__##_##__AMS_THREAD_NAME__ ).name
