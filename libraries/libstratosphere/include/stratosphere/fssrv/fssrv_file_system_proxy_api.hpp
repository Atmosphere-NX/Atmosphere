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

namespace ams::fssrv::fscreator {

    struct FileSystemCreatorInterfaces;

}

namespace ams::fssrv {

    class BaseStorageServiceImpl;
    class BaseFileSystemServiceImpl;
    class NcaFileSystemServiceImpl;
    class SaveDataFileSystemServiceImpl;
    class AccessFailureManagementServiceImpl;
    class TimeServiceImpl;
    class StatusReportServiceImpl;
    class ProgramRegistryServiceImpl;
    class AccessLogServiceImpl;
    class DebugConfigurationServiceImpl;

    /* ACCURATE_TO_VERSION: Unknown */
    struct FileSystemProxyConfiguration {
        fscreator::FileSystemCreatorInterfaces *m_fs_creator_interfaces;
        BaseStorageServiceImpl *m_base_storage_service_impl;
        BaseFileSystemServiceImpl *m_base_file_system_service_impl;
        NcaFileSystemServiceImpl *m_nca_file_system_service_impl;
        SaveDataFileSystemServiceImpl *m_save_data_file_system_service_impl;
        AccessFailureManagementServiceImpl *m_access_failure_management_service_impl;
        TimeServiceImpl *m_time_service_impl;
        StatusReportServiceImpl *m_status_report_service_impl;
        ProgramRegistryServiceImpl *m_program_registry_service_impl;
        AccessLogServiceImpl *m_access_log_service_impl;
        DebugConfigurationServiceImpl *m_debug_configuration_service_impl;
    };

    struct InternalProgramIdRangeForSpeedEmulation {
        u64 program_id_value_min;
        u64 program_id_value_max;
    };

}

namespace ams::fssrv {

    void InitializeForFileSystemProxy(const FileSystemProxyConfiguration &config);

    void InitializeFileSystemProxyServer(int threads);

}
