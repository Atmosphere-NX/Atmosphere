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

namespace ams::ncm {

    Result IntegratedContentMetaDatabaseImpl::Set(const ContentMetaKey &key, const sf::InBuffer &value) {
        AMS_UNUSED(key, value);
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result IntegratedContentMetaDatabaseImpl::Get(sf::Out<u64> out_size, const ContentMetaKey &key, const sf::OutBuffer &out_value) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Check that our list has interfaces to check. */
        R_UNLESS(m_list.GetCount() > 0, ncm::ResultContentMetaNotFound());

        /* Check each interface in turn. */
        R_RETURN(m_list.TryEach([&](const auto &data) {
            /* Try the current interface. */
            R_RETURN(data.interface->Get(out_size, key, out_value));
        }));
    }

    Result IntegratedContentMetaDatabaseImpl::Remove(const ContentMetaKey &key) {
        AMS_UNUSED(key);
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result IntegratedContentMetaDatabaseImpl::GetContentIdByType(sf::Out<ContentId> out_content_id, const ContentMetaKey &key, ContentType type) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Check that our list has interfaces to check. */
        R_UNLESS(m_list.GetCount() > 0, ncm::ResultContentMetaNotFound());

        /* Check each interface in turn. */
        R_RETURN(m_list.TryEach([&](const auto &data) {
            /* Try the current interface. */
            R_RETURN(data.interface->GetContentIdByType(out_content_id, key, type));
        }));
    }

    Result IntegratedContentMetaDatabaseImpl::ListContentInfo(sf::Out<s32> out_entries_written, const sf::OutArray<ContentInfo> &out_info, const ContentMetaKey &key, s32 offset) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Check that our list has interfaces to check. */
        R_UNLESS(m_list.GetCount() > 0, ncm::ResultContentMetaNotFound());

        /* Check each interface in turn. */
        R_RETURN(m_list.TryEach([&](const auto &data) {
            /* Check if the current interface has it. */
            bool has;
            R_TRY(data.interface->Has(std::addressof(has), key));

            /* If it doesn't, continue on. */
            R_UNLESS(has, ncm::ResultContentMetaNotFound());


            /* If it does, list the content infos. */
            R_RETURN(data.interface->ListContentInfo(out_entries_written, out_info, key, offset));
        }));
    }

    Result IntegratedContentMetaDatabaseImpl::List(sf::Out<s32> out_entries_total, sf::Out<s32> out_entries_written, const sf::OutArray<ContentMetaKey> &out_info, ContentMetaType meta_type, ApplicationId application_id, u64 min, u64 max, ContentInstallType install_type) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* List on all databases. */
        s32 entries_total   = 0;
        s32 entries_written = 0;
        R_TRY(m_list.ForAll([&](const auto &data) {
            /* List on the current database. */
            s32 cur_total;
            s32 cur_written;
            R_TRY(data.interface->List(std::addressof(cur_total), std::addressof(cur_written), sf::OutArray<ContentMetaKey>{out_info.GetPointer() + entries_written, out_info.GetSize() - entries_written}, meta_type, application_id, min, max, install_type));

            /* Add to the totals. */
            entries_total   += cur_total;
            entries_written += cur_written;
            R_SUCCEED();
        }));

        /* Set output. */
        *out_entries_total   = entries_total;
        *out_entries_written = entries_written;
        R_SUCCEED();
    }
    Result IntegratedContentMetaDatabaseImpl::GetLatestContentMetaKey(sf::Out<ContentMetaKey> out_key, u64 id) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Check that our list has interfaces to check. */
        R_UNLESS(m_list.GetCount() > 0, ncm::ResultContentMetaNotFound());

        /* Check each interface in turn. */
        R_RETURN(m_list.TryEach([&](const auto &data) {
            /* Try the current interface. */
            R_RETURN(data.interface->GetLatestContentMetaKey(out_key, id));
        }));
    }

    Result IntegratedContentMetaDatabaseImpl::ListApplication(sf::Out<s32> out_entries_total, sf::Out<s32> out_entries_written, const sf::OutArray<ApplicationContentMetaKey> &out_keys, ContentMetaType meta_type) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* List on all databases. */
        s32 entries_total   = 0;
        s32 entries_written = 0;
        R_TRY(m_list.ForAll([&](const auto &data) {
            /* List on the current database. */
            s32 cur_total;
            s32 cur_written;
            R_TRY(data.interface->ListApplication(std::addressof(cur_total), std::addressof(cur_written), sf::OutArray<ApplicationContentMetaKey>{out_keys.GetPointer() + entries_written, out_keys.GetSize() - entries_written}, meta_type));

            /* Add to the totals. */
            entries_total   += cur_total;
            entries_written += cur_written;
            R_SUCCEED();
        }));

        /* Set output. */
        *out_entries_total   = entries_total;
        *out_entries_written = entries_written;
        R_SUCCEED();
    }

    Result IntegratedContentMetaDatabaseImpl::Has(sf::Out<bool> out, const ContentMetaKey &key) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* If we don't locate the content meta, set the output to false. */
        *out = false;
        ON_RESULT_INCLUDED(ncm::ResultContentMetaNotFound) { *out = false; };

        /* Check each interface in turn. */
        R_RETURN(m_list.TryEach([&](const auto &data) {
            /* Check if the current interface has it. */
            R_TRY(data.interface->Has(out, key));

            /* If it doesn't, continue on. */
            R_UNLESS(*out, ncm::ResultContentMetaNotFound());

            /* If it does, we're done looking. */
            R_SUCCEED();
        }));
    }

    Result IntegratedContentMetaDatabaseImpl::HasAll(sf::Out<bool> out, const sf::InArray<ContentMetaKey> &keys) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());
        *out = false;

        /* Check if keys are present. */
        for (size_t i = 0; i < keys.GetSize(); i++) {
            /* Check if we have the current key. */
            bool has;
            R_TRY(this->Has(std::addressof(has), keys[i]));

            /* If we don't, then we can early return because we don't have all. */
            R_SUCCEED_IF(!has);
        }

        *out = true;
        R_SUCCEED();
    }

    Result IntegratedContentMetaDatabaseImpl::GetSize(sf::Out<u64> out_size, const ContentMetaKey &key) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Check that our list has interfaces to check. */
        R_UNLESS(m_list.GetCount() > 0, ncm::ResultContentMetaNotFound());

        /* Check each interface in turn. */
        R_RETURN(m_list.TryEach([&](const auto &data) {
            /* Try the current interface. */
            R_RETURN(data.interface->GetSize(out_size, key));
        }));
    }

    Result IntegratedContentMetaDatabaseImpl::GetRequiredSystemVersion(sf::Out<u32> out_version, const ContentMetaKey &key) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Check that our list has interfaces to check. */
        R_UNLESS(m_list.GetCount() > 0, ncm::ResultContentMetaNotFound());

        /* Check each interface in turn. */
        R_RETURN(m_list.TryEach([&](const auto &data) {
            /* Try the current interface. */
            R_RETURN(data.interface->GetRequiredSystemVersion(out_version, key));
        }));
    }

    Result IntegratedContentMetaDatabaseImpl::GetPatchContentMetaId(sf::Out<u64> out_patch_id, const ContentMetaKey &key) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Check that our list has interfaces to check. */
        R_UNLESS(m_list.GetCount() > 0, ncm::ResultContentMetaNotFound());

        /* Check each interface in turn. */
        R_RETURN(m_list.TryEach([&](const auto &data) {
            /* Try the current interface. */
            R_RETURN(data.interface->GetPatchContentMetaId(out_patch_id, key));
        }));
    }

    Result IntegratedContentMetaDatabaseImpl::DisableForcibly() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        m_disabled = true;
        R_SUCCEED();
    }

    Result IntegratedContentMetaDatabaseImpl::LookupOrphanContent(const sf::OutArray<bool> &out_orphaned, const sf::InArray<ContentId> &content_ids) {
        AMS_UNUSED(out_orphaned, content_ids);
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result IntegratedContentMetaDatabaseImpl::Commit() {
        R_THROW(ncm::ResultInvalidOperation());
    }

    Result IntegratedContentMetaDatabaseImpl::HasContent(sf::Out<bool> out, const ContentMetaKey &key, const ContentId &content_id) {
        /* Check each interface in turn. */
        R_RETURN(m_list.TryEach([&](const auto &data) {
            /* Check if the current interface has it. */
            /* NOTE: Nintendo bug: Nintendo calls this->Has(), which is likely a copy paste error from ::HasAll... */
            bool has;
            R_TRY(data.interface->Has(std::addressof(has), key));

            /* If it doesn't, continue on. */
            R_UNLESS(has, ncm::ResultContentMetaNotFound());

            /* If it does, list the content infos. */
            R_RETURN(data.interface->HasContent(out, key, content_id));
        }));
    }

    Result IntegratedContentMetaDatabaseImpl::ListContentMetaInfo(sf::Out<s32> out_entries_written, const sf::OutArray<ContentMetaInfo> &out_meta_info, const ContentMetaKey &key, s32 offset) {
        /* Check each interface in turn. */
        R_RETURN(m_list.TryEach([&](const auto &data) {
            /* Check if the current interface has it. */
            bool has;
            R_TRY(data.interface->Has(std::addressof(has), key));

            /* If it doesn't, continue on. */
            R_UNLESS(has, ncm::ResultContentMetaNotFound());

            /* If it does, list the content infos. */
            R_RETURN(data.interface->ListContentMetaInfo(out_entries_written, out_meta_info, key, offset));
        }));
    }

    Result IntegratedContentMetaDatabaseImpl::GetAttributes(sf::Out<u8> out_attributes, const ContentMetaKey &key) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Check that our list has interfaces to check. */
        R_UNLESS(m_list.GetCount() > 0, ncm::ResultContentMetaNotFound());

        /* Check each interface in turn. */
        R_RETURN(m_list.TryEach([&](const auto &data) {
            /* Try the current interface. */
            R_RETURN(data.interface->GetAttributes(out_attributes, key));
        }));
    }

    Result IntegratedContentMetaDatabaseImpl::GetRequiredApplicationVersion(sf::Out<u32> out_version, const ContentMetaKey &key) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Check that our list has interfaces to check. */
        R_UNLESS(m_list.GetCount() > 0, ncm::ResultContentMetaNotFound());

        /* Check each interface in turn. */
        R_RETURN(m_list.TryEach([&](const auto &data) {
            /* Try the current interface. */
            R_RETURN(data.interface->GetRequiredApplicationVersion(out_version, key));
        }));
    }

    Result IntegratedContentMetaDatabaseImpl::GetContentIdByTypeAndIdOffset(sf::Out<ContentId> out_content_id, const ContentMetaKey &key, ContentType type, u8 id_offset) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Check that our list has interfaces to check. */
        R_UNLESS(m_list.GetCount() > 0, ncm::ResultContentMetaNotFound());

        /* Check each interface in turn. */
        R_RETURN(m_list.TryEach([&](const auto &data) {
            /* Try the current interface. */
            R_RETURN(data.interface->GetContentIdByTypeAndIdOffset(out_content_id, key, type, id_offset));
        }));
    }

    Result IntegratedContentMetaDatabaseImpl::GetCount(sf::Out<u32> out_count) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* List on all databases. */
        u32 total = 0;
        R_TRY(m_list.ForAll([&](const auto &data) {
            /* List on the current database. */
            u32 cur;
            R_TRY(data.interface->GetCount(std::addressof(cur)));

            /* Add to the totals. */
            total += cur;
            R_SUCCEED();
        }));

        /* Set output. */
        *out_count = total;
        R_SUCCEED();
    }

    Result IntegratedContentMetaDatabaseImpl::GetOwnerApplicationId(sf::Out<ApplicationId> out_id, const ContentMetaKey &key) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Check that our list has interfaces to check. */
        R_UNLESS(m_list.GetCount() > 0, ncm::ResultContentMetaNotFound());

        /* Check each interface in turn. */
        R_RETURN(m_list.TryEach([&](const auto &data) {
            /* Try the current interface. */
            R_RETURN(data.interface->GetOwnerApplicationId(out_id, key));
        }));
    }

    Result IntegratedContentMetaDatabaseImpl::GetContentAccessibilities(sf::Out<u8> out_accessibilities, const ContentMetaKey &key) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Check that our list has interfaces to check. */
        R_UNLESS(m_list.GetCount() > 0, ncm::ResultContentMetaNotFound());

        /* Check each interface in turn. */
        R_RETURN(m_list.TryEach([&](const auto &data) {
            /* Try the current interface. */
            R_RETURN(data.interface->GetContentAccessibilities(out_accessibilities, key));
        }));
    }

    Result IntegratedContentMetaDatabaseImpl::GetContentInfoByType(sf::Out<ContentInfo> out_content_info, const ContentMetaKey &key, ContentType type) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Check that our list has interfaces to check. */
        R_UNLESS(m_list.GetCount() > 0, ncm::ResultContentMetaNotFound());

        /* Check each interface in turn. */
        R_RETURN(m_list.TryEach([&](const auto &data) {
            /* Try the current interface. */
            R_RETURN(data.interface->GetContentInfoByType(out_content_info, key, type));
        }));
    }

    Result IntegratedContentMetaDatabaseImpl::GetContentInfoByTypeAndIdOffset(sf::Out<ContentInfo> out_content_info, const ContentMetaKey &key, ContentType type, u8 id_offset) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Check that our list has interfaces to check. */
        R_UNLESS(m_list.GetCount() > 0, ncm::ResultContentMetaNotFound());

        /* Check each interface in turn. */
        R_RETURN(m_list.TryEach([&](const auto &data) {
            /* Try the current interface. */
            R_RETURN(data.interface->GetContentInfoByTypeAndIdOffset(out_content_info, key, type, id_offset));
        }));
    }

    Result IntegratedContentMetaDatabaseImpl::GetPlatform(sf::Out<ncm::ContentMetaPlatform> out, const ContentMetaKey &key) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Check that our list has interfaces to check. */
        R_UNLESS(m_list.GetCount() > 0, ncm::ResultContentMetaNotFound());

        /* Check each interface in turn. */
        R_RETURN(m_list.TryEach([&](const auto &data) {
            /* Try the current interface. */
            R_RETURN(data.interface->GetPlatform(out, key));
        }));
    }

    Result IntegratedContentMetaDatabaseImpl::HasAttributes(sf::Out<u8> out, u8 attr_mask) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're enabled. */
        R_TRY(this->EnsureEnabled());

        /* Test whether we have the attributes on all databases. */
        u8 combined_attributes = 0;
        R_TRY(m_list.ForAll([&](const auto &data) {
            /* Check the current database. */
            u8 cur_attr = 0;
            R_TRY(data.interface->HasAttributes(std::addressof(cur_attr), attr_mask));

            /* Accumulate the attributes found in the current interface. */
            combined_attributes |= cur_attr;
            R_SUCCEED();
        }));

        /* Set the output. */
        *out = combined_attributes;
        R_SUCCEED();
    }

}
