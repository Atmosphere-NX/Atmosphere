/*
 * Copyright (c) 2019 Adubbz
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
#include <switch.h>
#include <stratosphere.hpp>

namespace sts::ncm {

    struct MountName {
        char name[0x10];
    };

    struct Uuid {
        u8 uuid[0x10];

        bool operator==(const Uuid& other) const {
            return memcmp(this->uuid, other.uuid, sizeof(Uuid)) == 0;
        }

        bool operator!=(const Uuid& other) const {
            return !(*this == other);
        }
    };

    static_assert(sizeof(Uuid) == 0x10, "Uuid definition!");

    static constexpr Uuid InvalidUuid = { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

    typedef Uuid ContentId;
    typedef Uuid PlaceHolderId;

    enum class ContentMetaType : u8 {
        SystemProgram          = 0x1,
        SystemData             = 0x2,
        SystemUpdate           = 0x3,
        BootImagePackage       = 0x4,
        BootImagePackageSafe   = 0x5,
        Application            = 0x80,
        Patch                  = 0x81,
        AddOnContent           = 0x82,
        Delta                  = 0x83,
    };

    enum class ContentType : u8 {
        Meta = 0,
        Program = 1,
        Data = 2,
        Control = 3,
        HtmlDocument = 4,
        LegalInformation = 5,
        DeltaFragment = 6,
    };

    enum class ContentMetaAttribute : u8 {
        None = 0,
        IncludesExFatDriver = 1,
        Rebootless = 2,
    };

    enum class ContentInstallType : u8 {
        Full = 0,
        FragmentOnly = 1,
        Unknown = 7,
    };

    struct ContentMetaKey {
        TitleId id;
        u32 version;
        ContentMetaType type;
        ContentInstallType install_type;
        u8 padding[2];

        bool operator<(const ContentMetaKey& other) const {
            if (this->id < other.id) {
                return true;
            } else if (this->id != other.id) {
                return false;
            }
            
            if (this->version < other.version) {
                return true;
            } else if (this->version != other.version) {
                return false;
            }
            
            if (this->type < other.type) {
                return true;
            } else if (this->type != other.type) {
                return false;
            }
            
            return this->install_type < other.install_type;
        }

        bool operator==(const ContentMetaKey& other) const {
            if (this->id != other.id) {
                return false;
            }

            if (this->version != other.version) {
                return false;
            }

            if (this->type != other.type) {
                return false;
            }

            if (this->install_type != other.install_type) {
                return false;
            }
            
            return true;
        }

        bool operator!=(const ContentMetaKey& other) const {
            return !(*this == other);
        }
    } PACKED;

    static_assert(sizeof(ContentMetaKey) == 0x10, "ContentMetaKey definition!");

    // Used by system updates. They share the exact same struct as ContentMetaKey;
    typedef ContentMetaKey ContentMetaInfo;

    struct ApplicationContentMetaKey {
        ContentMetaKey key;
        TitleId application_title_id;
    } PACKED;

    struct ContentInfo {
        ContentId content_id;
        u8 size[6];
        ContentType content_type;
        u8 id_offset;
    } PACKED;

    static_assert(sizeof(ContentInfo) == 0x18, "ContentInfo definition!");

    typedef void (*MakeContentPathFunc)(char* out, ContentId content_id, const char* root);
    typedef void (*MakePlaceHolderPathFunc)(char* out, PlaceHolderId placeholder_id, const char* root);

    // TODO: Move to libstrat
    static constexpr Result ResultNcmStoragePathNotFound              = MAKERESULT(Module_Ncm, 1);
    static constexpr Result ResultNcmInvalidPlaceHolderDirectoryEntry = MAKERESULT(Module_Ncm, 170);
    static constexpr Result ResultNcmInvalidContentStorageOperation   = MAKERESULT(Module_Ncm, 190);
    static constexpr Result ResultNcmStorageRootNotFound              = MAKERESULT(Module_Ncm, 310);
    static constexpr Result ResultFsUnqualifiedPath                   = MAKERESULT(Module_Fs, 6065);
    static constexpr Result ResultFsMountNameNotFound                 = MAKERESULT(Module_Fs, 6905);

}