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

namespace ams::ncm {

    #if defined(ATMOSPHERE_OS_HORIZON)
    class RemoteContentStorageImpl {
        private:
            ::NcmContentStorage m_srv;
        public:
            RemoteContentStorageImpl(::NcmContentStorage &cs) : m_srv(cs) { /* ... */ }

            ~RemoteContentStorageImpl() { ::ncmContentStorageClose(std::addressof(m_srv)); }
        private:
            ALWAYS_INLINE ::NcmPlaceHolderId *Convert(PlaceHolderId *p) {
                static_assert(sizeof(PlaceHolderId) == sizeof(::NcmPlaceHolderId));
                return reinterpret_cast<::NcmPlaceHolderId *>(p);
            }

            ALWAYS_INLINE ::NcmPlaceHolderId *Convert(PlaceHolderId &p) {
                static_assert(sizeof(PlaceHolderId) == sizeof(::NcmPlaceHolderId));
                return reinterpret_cast<::NcmPlaceHolderId *>(std::addressof(p));
            }

            ALWAYS_INLINE ::NcmContentId *Convert(ContentId *c) {
                static_assert(sizeof(ContentId)     == sizeof(::NcmContentId));
                return reinterpret_cast<::NcmContentId *>(c);
            }

            ALWAYS_INLINE ::NcmContentId *Convert(ContentId &c) {
                static_assert(sizeof(ContentId)     == sizeof(::NcmContentId));
                return reinterpret_cast<::NcmContentId *>(std::addressof(c));
            }

            ALWAYS_INLINE const ::NcmContentId *Convert(const ContentId *c) {
                static_assert(sizeof(ContentId)     == sizeof(::NcmContentId));
                return reinterpret_cast<const ::NcmContentId *>(c);
            }

            ALWAYS_INLINE const ::NcmContentId *Convert(const ContentId &c) {
                static_assert(sizeof(ContentId)     == sizeof(::NcmContentId));
                return reinterpret_cast<const ::NcmContentId *>(std::addressof(c));
            }

            ALWAYS_INLINE ::FsContentAttributes Convert(fs::ContentAttributes attr) {
                return static_cast<::FsContentAttributes>(static_cast<u8>(attr));
            }
        public:
            Result GeneratePlaceHolderId(sf::Out<PlaceHolderId> out) {
                R_RETURN(ncmContentStorageGeneratePlaceHolderId(std::addressof(m_srv), Convert(out.GetPointer())));
            }

            Result CreatePlaceHolder(PlaceHolderId placeholder_id, ContentId content_id, s64 size) {
                R_RETURN(ncmContentStorageCreatePlaceHolder(std::addressof(m_srv), Convert(content_id), Convert(placeholder_id), size));
            }

            Result DeletePlaceHolder(PlaceHolderId placeholder_id) {
                R_RETURN(ncmContentStorageDeletePlaceHolder(std::addressof(m_srv), Convert(placeholder_id)));
            }

            Result HasPlaceHolder(sf::Out<bool> out, PlaceHolderId placeholder_id) {
                R_RETURN(ncmContentStorageHasPlaceHolder(std::addressof(m_srv), out.GetPointer(), Convert(placeholder_id)));
            }

            Result WritePlaceHolder(PlaceHolderId placeholder_id, s64 offset, const sf::InBuffer &data) {
                R_RETURN(ncmContentStorageWritePlaceHolder(std::addressof(m_srv), Convert(placeholder_id), offset, data.GetPointer(), data.GetSize()));
            }

            Result Register(PlaceHolderId placeholder_id, ContentId content_id) {
                R_RETURN(ncmContentStorageRegister(std::addressof(m_srv), Convert(content_id), Convert(placeholder_id)));
            }

            Result Delete(ContentId content_id) {
                R_RETURN(ncmContentStorageDelete(std::addressof(m_srv), Convert(content_id)));
            }

            Result Has(sf::Out<bool> out, ContentId content_id) {
                R_RETURN(ncmContentStorageHas(std::addressof(m_srv), out.GetPointer(), Convert(content_id)));
            }

            Result GetPath(sf::Out<Path> out, ContentId content_id) {
                R_RETURN(ncmContentStorageGetPath(std::addressof(m_srv), out.GetPointer()->str, sizeof(out.GetPointer()->str), Convert(content_id)));
            }

            Result GetPlaceHolderPath(sf::Out<Path> out, PlaceHolderId placeholder_id) {
                R_RETURN(ncmContentStorageGetPlaceHolderPath(std::addressof(m_srv), out.GetPointer()->str, sizeof(out.GetPointer()->str), Convert(placeholder_id)));
            }

            Result CleanupAllPlaceHolder() {
                R_RETURN(ncmContentStorageCleanupAllPlaceHolder(std::addressof(m_srv)));
            }

            Result ListPlaceHolder(sf::Out<s32> out_count, const sf::OutArray<PlaceHolderId> &out_buf) {
                R_RETURN(ncmContentStorageListPlaceHolder(std::addressof(m_srv), Convert(out_buf.GetPointer()), out_buf.GetSize(), out_count.GetPointer()));
            }

            Result GetContentCount(sf::Out<s32> out_count) {
                R_RETURN(ncmContentStorageGetContentCount(std::addressof(m_srv), out_count.GetPointer()));
            }

            Result ListContentId(sf::Out<s32> out_count, const sf::OutArray<ContentId> &out_buf, s32 offset) {
                R_RETURN(ncmContentStorageListContentId(std::addressof(m_srv), Convert(out_buf.GetPointer()), out_buf.GetSize(), out_count.GetPointer(), offset));
            }

            Result GetSizeFromContentId(sf::Out<s64> out_size, ContentId content_id) {
                R_RETURN(ncmContentStorageGetSizeFromContentId(std::addressof(m_srv), out_size.GetPointer(), Convert(content_id)));
            }

            Result DisableForcibly() {
                R_RETURN(ncmContentStorageDisableForcibly(std::addressof(m_srv)));
            }

            Result RevertToPlaceHolder(PlaceHolderId placeholder_id, ContentId old_content_id, ContentId new_content_id) {
                R_RETURN(ncmContentStorageRevertToPlaceHolder(std::addressof(m_srv), Convert(placeholder_id), Convert(old_content_id), Convert(new_content_id)));
            }

            Result SetPlaceHolderSize(PlaceHolderId placeholder_id, s64 size) {
                R_RETURN(ncmContentStorageSetPlaceHolderSize(std::addressof(m_srv), Convert(placeholder_id), size));
            }

            Result ReadContentIdFile(const sf::OutBuffer &buf, ContentId content_id, s64 offset) {
                R_RETURN(ncmContentStorageReadContentIdFile(std::addressof(m_srv), buf.GetPointer(), buf.GetSize(), Convert(content_id), offset));
            }

            Result GetRightsIdFromPlaceHolderIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, PlaceHolderId placeholder_id) {
                ::NcmRightsId rights_id;
                R_TRY(ncmContentStorageGetRightsIdFromPlaceHolderId(std::addressof(m_srv), std::addressof(rights_id), Convert(placeholder_id), Convert(fs::ContentAttributes_None)));

                static_assert(sizeof(*out_rights_id.GetPointer()) <= sizeof(rights_id));
                std::memcpy(out_rights_id.GetPointer(), std::addressof(rights_id), sizeof(*out_rights_id.GetPointer()));
                R_SUCCEED();
            }

            Result GetRightsIdFromPlaceHolderIdDeprecated2(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id) {
                R_RETURN(this->GetRightsIdFromPlaceHolderId(out_rights_id, placeholder_id, fs::ContentAttributes_None));
            }

            Result GetRightsIdFromPlaceHolderId(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, fs::ContentAttributes attr) {
                ::NcmRightsId rights_id;
                R_TRY(ncmContentStorageGetRightsIdFromPlaceHolderId(std::addressof(m_srv), std::addressof(rights_id), Convert(placeholder_id), Convert(attr)));

                static_assert(sizeof(*out_rights_id.GetPointer()) <= sizeof(rights_id));
                std::memcpy(out_rights_id.GetPointer(), std::addressof(rights_id), sizeof(*out_rights_id.GetPointer()));
                R_SUCCEED();
            }

            Result GetRightsIdFromContentIdDeprecated(sf::Out<ams::fs::RightsId> out_rights_id, ContentId content_id) {
                ::NcmRightsId rights_id;
                R_TRY(ncmContentStorageGetRightsIdFromContentId(std::addressof(m_srv), std::addressof(rights_id), Convert(content_id), Convert(fs::ContentAttributes_None)));

                static_assert(sizeof(*out_rights_id.GetPointer()) <= sizeof(rights_id));
                std::memcpy(out_rights_id.GetPointer(), std::addressof(rights_id), sizeof(*out_rights_id.GetPointer()));
                R_SUCCEED();
            }

            Result GetRightsIdFromContentIdDeprecated2(sf::Out<ncm::RightsId> out_rights_id, ContentId content_id) {
                R_RETURN(this->GetRightsIdFromContentId(out_rights_id, content_id, fs::ContentAttributes_None));
            }

            Result GetRightsIdFromContentId(sf::Out<ncm::RightsId> out_rights_id, ContentId content_id, fs::ContentAttributes attr) {
                ::NcmRightsId rights_id;
                R_TRY(ncmContentStorageGetRightsIdFromContentId(std::addressof(m_srv), std::addressof(rights_id), Convert(content_id), Convert(attr)));

                static_assert(sizeof(*out_rights_id.GetPointer()) <= sizeof(rights_id));
                std::memcpy(out_rights_id.GetPointer(), std::addressof(rights_id), sizeof(*out_rights_id.GetPointer()));
                R_SUCCEED();
            }

            Result WriteContentForDebug(ContentId content_id, s64 offset, const sf::InBuffer &data) {
                R_RETURN(ncmContentStorageWriteContentForDebug(std::addressof(m_srv), Convert(content_id), offset, data.GetPointer(), data.GetSize()));
            }

            Result GetFreeSpaceSize(sf::Out<s64> out_size) {
                R_RETURN(ncmContentStorageGetFreeSpaceSize(std::addressof(m_srv), out_size.GetPointer()));
            }

            Result GetTotalSpaceSize(sf::Out<s64> out_size) {
                R_RETURN(ncmContentStorageGetTotalSpaceSize(std::addressof(m_srv), out_size.GetPointer()));
            }

            Result FlushPlaceHolder() {
                R_RETURN(ncmContentStorageFlushPlaceHolder(std::addressof(m_srv)));
            }

            Result GetSizeFromPlaceHolderId(sf::Out<s64> out_size, PlaceHolderId placeholder_id) {
                R_RETURN(ncmContentStorageGetSizeFromPlaceHolderId(std::addressof(m_srv), out_size.GetPointer(), Convert(placeholder_id)));
            }

            Result RepairInvalidFileAttribute() {
                R_RETURN(ncmContentStorageRepairInvalidFileAttribute(std::addressof(m_srv)));
            }

            Result GetRightsIdFromPlaceHolderIdWithCache(sf::Out<ncm::RightsId> out_rights_id, PlaceHolderId placeholder_id, ContentId cache_content_id, fs::ContentAttributes attr) {
                static_assert(sizeof(::NcmRightsId) == sizeof(ncm::RightsId));
                ::NcmRightsId *out = reinterpret_cast<::NcmRightsId *>(out_rights_id.GetPointer());
                R_RETURN(ncmContentStorageGetRightsIdFromPlaceHolderIdWithCache(std::addressof(m_srv), out, Convert(placeholder_id), Convert(cache_content_id), Convert(attr)));
            }

            Result RegisterPath(const ContentId &content_id, const Path &path) {
                R_RETURN(ncmContentStorageRegisterPath(std::addressof(m_srv), Convert(content_id), path.str));
            }

            Result ClearRegisteredPath() {
                R_RETURN(ncmContentStorageClearRegisteredPath(std::addressof(m_srv)));
            }

            Result GetProgramId(sf::Out<ncm::ProgramId> out, ContentId content_id, fs::ContentAttributes attr) {
                static_assert(sizeof(ncm::ProgramId) == sizeof(u64));
                R_RETURN(ncmContentStorageGetProgramId(std::addressof(m_srv), reinterpret_cast<u64 *>(out.GetPointer()), Convert(content_id), Convert(attr)));
            }

            /* 16.0.0 Alignment change hacks. */
            Result CreatePlaceHolder_AtmosphereAlignmentFix(ContentId content_id, PlaceHolderId placeholder_id, s64 size) { R_RETURN(this->CreatePlaceHolder(placeholder_id, content_id, size)); }
            Result Register_AtmosphereAlignmentFix(ContentId content_id, PlaceHolderId placeholder_id) { R_RETURN(this->Register(placeholder_id, content_id)); }
            Result RevertToPlaceHolder_AtmosphereAlignmentFix(ncm::ContentId old_content_id, ncm::ContentId new_content_id, ncm::PlaceHolderId placeholder_id) { R_RETURN(this->RevertToPlaceHolder(placeholder_id, old_content_id, new_content_id)); }
            Result GetRightsIdFromPlaceHolderIdWithCacheDeprecated(sf::Out<ncm::RightsId> out_rights_id, ContentId cache_content_id, PlaceHolderId placeholder_id) { R_RETURN(this->GetRightsIdFromPlaceHolderIdWithCache(out_rights_id, placeholder_id, cache_content_id, fs::ContentAttributes_None)); }
    };
    static_assert(ncm::IsIContentStorage<RemoteContentStorageImpl>);
    #endif

}
