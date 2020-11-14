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

    namespace impl {

        #define AMS_SYSUPDATER_SYSTEM_UPDATE_INTERFACE_INFO(C, H)                                                                                                                                                                   \
            AMS_SF_METHOD_INFO(C, H, 0, Result, GetUpdateInformation,     (sf::Out<UpdateInformation> out, const ncm::Path &path))                                                                                                  \
            AMS_SF_METHOD_INFO(C, H, 1, Result, ValidateUpdate,           (sf::Out<Result> out_validate_result, sf::Out<Result> out_validate_exfat_result, sf::Out<UpdateValidationInfo> out_validate_info, const ncm::Path &path)) \
            AMS_SF_METHOD_INFO(C, H, 2, Result, SetupUpdate,              (sf::CopyHandle transfer_memory, u64 transfer_memory_size, const ncm::Path &path, bool exfat))                                                            \
            AMS_SF_METHOD_INFO(C, H, 3, Result, SetupUpdateWithVariation, (sf::CopyHandle transfer_memory, u64 transfer_memory_size, const ncm::Path &path, bool exfat, ncm::FirmwareVariationId firmware_variation_id))            \
            AMS_SF_METHOD_INFO(C, H, 4, Result, RequestPrepareUpdate,     (sf::OutCopyHandle out_event_handle, sf::Out<std::shared_ptr<ns::impl::IAsyncResult>> out_async))                                                         \
            AMS_SF_METHOD_INFO(C, H, 5, Result, GetPrepareUpdateProgress, (sf::Out<SystemUpdateProgress> out))                                                                                                                      \
            AMS_SF_METHOD_INFO(C, H, 6, Result, HasPreparedUpdate,        (sf::Out<bool> out))                                                                                                                                      \
            AMS_SF_METHOD_INFO(C, H, 7, Result, ApplyPreparedUpdate,      ())

        AMS_SF_DEFINE_INTERFACE(ISystemUpdateInterface, AMS_SYSUPDATER_SYSTEM_UPDATE_INTERFACE_INFO)



    }

    class SystemUpdateService final {
        private:
            SystemUpdateApplyManager apply_manager;
            std::optional<ncm::PackageSystemDowngradeTask> update_task;
            std::optional<os::TransferMemory> update_transfer_memory;
            bool setup_update;
            bool requested_update;
        public:
            constexpr SystemUpdateService() : apply_manager(), update_task(), update_transfer_memory(), setup_update(false), requested_update(false) { /* ... */ }
        private:
            Result SetupUpdateImpl(os::ManagedHandle transfer_memory, u64 transfer_memory_size, const ncm::Path &path, bool exfat, ncm::FirmwareVariationId firmware_variation_id);
            Result InitializeUpdateTask(os::ManagedHandle &transfer_memory, u64 transfer_memory_size, const ncm::Path &path, bool exfat, ncm::FirmwareVariationId firmware_variation_id);
        public:
            Result GetUpdateInformation(sf::Out<UpdateInformation> out, const ncm::Path &path);
            Result ValidateUpdate(sf::Out<Result> out_validate_result, sf::Out<Result> out_validate_exfat_result, sf::Out<UpdateValidationInfo> out_validate_info, const ncm::Path &path);
            Result SetupUpdate(sf::CopyHandle transfer_memory, u64 transfer_memory_size, const ncm::Path &path, bool exfat);
            Result SetupUpdateWithVariation(sf::CopyHandle transfer_memory, u64 transfer_memory_size, const ncm::Path &path, bool exfat, ncm::FirmwareVariationId firmware_variation_id);
            Result RequestPrepareUpdate(sf::OutCopyHandle out_event_handle, sf::Out<std::shared_ptr<ns::impl::IAsyncResult>> out_async);
            Result GetPrepareUpdateProgress(sf::Out<SystemUpdateProgress> out);
            Result HasPreparedUpdate(sf::Out<bool> out);
            Result ApplyPreparedUpdate();
    };
    static_assert(impl::IsISystemUpdateInterface<SystemUpdateService>);

}
