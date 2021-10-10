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
#include <stratosphere.hpp>
#include "sysupdater_apply_manager.hpp"

namespace ams::mitm::sysupdater {

    constexpr inline size_t FirmwareVariationCountMax = 16;

    struct UpdateInformation {
        u32 version;
        bool exfat_supported;
        u32 firmware_variation_count;
        ncm::FirmwareVariationId firmware_variation_ids[FirmwareVariationCountMax];
    };

    struct UpdateValidationInfo {
        ncm::ContentMetaKey invalid_key;
        ncm::ContentId invalid_content_id;
    };

    struct SystemUpdateProgress {
        s64 current_size;
        s64 total_size;
    };

}

#define AMS_SYSUPDATER_SYSTEM_UPDATE_INTERFACE_INFO(C, H)                                                                                                                                                                                                                                                                             \
    AMS_SF_METHOD_INFO(C, H, 0, Result, GetUpdateInformation,     (sf::Out<mitm::sysupdater::UpdateInformation> out, const ncm::Path &path),                                                                                                  (out, path))                                                                            \
    AMS_SF_METHOD_INFO(C, H, 1, Result, ValidateUpdate,           (sf::Out<Result> out_validate_result, sf::Out<Result> out_validate_exfat_result, sf::Out<mitm::sysupdater::UpdateValidationInfo> out_validate_info, const ncm::Path &path), (out_validate_result, out_validate_exfat_result, out_validate_info, path))              \
    AMS_SF_METHOD_INFO(C, H, 2, Result, SetupUpdate,              (sf::CopyHandle &&transfer_memory, u64 transfer_memory_size, const ncm::Path &path, bool exfat),                                                                            (std::move(transfer_memory), transfer_memory_size, path, exfat))                        \
    AMS_SF_METHOD_INFO(C, H, 3, Result, SetupUpdateWithVariation, (sf::CopyHandle &&transfer_memory, u64 transfer_memory_size, const ncm::Path &path, bool exfat, ncm::FirmwareVariationId firmware_variation_id),                            (std::move(transfer_memory), transfer_memory_size, path, exfat, firmware_variation_id)) \
    AMS_SF_METHOD_INFO(C, H, 4, Result, RequestPrepareUpdate,     (sf::OutCopyHandle out_event_handle, sf::Out<sf::SharedPointer<ns::impl::IAsyncResult>> out_async),                                                                         (out_event_handle, out_async))                                                          \
    AMS_SF_METHOD_INFO(C, H, 5, Result, GetPrepareUpdateProgress, (sf::Out<mitm::sysupdater::SystemUpdateProgress> out),                                                                                                                      (out))                                                                                  \
    AMS_SF_METHOD_INFO(C, H, 6, Result, HasPreparedUpdate,        (sf::Out<bool> out),                                                                                                                                                        (out))                                                                                  \
    AMS_SF_METHOD_INFO(C, H, 7, Result, ApplyPreparedUpdate,      (),                                                                                                                                                                         ())

AMS_SF_DEFINE_INTERFACE(ams::mitm::sysupdater::impl, ISystemUpdateInterface, AMS_SYSUPDATER_SYSTEM_UPDATE_INTERFACE_INFO)

namespace ams::mitm::sysupdater {

    class SystemUpdateService {
        private:
            SystemUpdateApplyManager m_apply_manager;
            util::optional<ncm::PackageSystemDowngradeTask> m_update_task;
            util::optional<os::TransferMemory> m_update_transfer_memory;
            bool m_setup_update;
            bool m_requested_update;
        public:
            constexpr SystemUpdateService() : m_apply_manager(), m_update_task(), m_update_transfer_memory(), m_setup_update(false), m_requested_update(false) { /* ... */ }
        private:
            Result SetupUpdateImpl(sf::NativeHandle &&transfer_memory, u64 transfer_memory_size, const ncm::Path &path, bool exfat, ncm::FirmwareVariationId firmware_variation_id);
            Result InitializeUpdateTask(sf::NativeHandle &&transfer_memory, u64 transfer_memory_size, const ncm::Path &path, bool exfat, ncm::FirmwareVariationId firmware_variation_id);
        public:
            Result GetUpdateInformation(sf::Out<UpdateInformation> out, const ncm::Path &path);
            Result ValidateUpdate(sf::Out<Result> out_validate_result, sf::Out<Result> out_validate_exfat_result, sf::Out<UpdateValidationInfo> out_validate_info, const ncm::Path &path);
            Result SetupUpdate(sf::CopyHandle &&transfer_memory, u64 transfer_memory_size, const ncm::Path &path, bool exfat);
            Result SetupUpdateWithVariation(sf::CopyHandle &&transfer_memory, u64 transfer_memory_size, const ncm::Path &path, bool exfat, ncm::FirmwareVariationId firmware_variation_id);
            Result RequestPrepareUpdate(sf::OutCopyHandle out_event_handle, sf::Out<sf::SharedPointer<ns::impl::IAsyncResult>> out_async);
            Result GetPrepareUpdateProgress(sf::Out<SystemUpdateProgress> out);
            Result HasPreparedUpdate(sf::Out<bool> out);
            Result ApplyPreparedUpdate();
    };
    static_assert(impl::IsISystemUpdateInterface<SystemUpdateService>);

}
