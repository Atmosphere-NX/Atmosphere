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

/* IPS Patching adapted from Luma3DS (https://github.com/AuroraWright/Luma3DS/blob/master/sysmodules/loader/source/patcher.c) */

namespace ams::patcher {

    namespace {

        /* Convenience definitions. */
        constexpr const char IpsHeadMagic[5] = {'P', 'A', 'T', 'C', 'H'};
        constexpr const char IpsTailMagic[3] = {'E', 'O', 'F'};
        constexpr const char Ips32HeadMagic[5] = {'I', 'P', 'S', '3', '2'};
        constexpr const char Ips32TailMagic[4] = {'E', 'E', 'O', 'F'};
        constexpr const char *IpsFileExtension = ".ips";
        constexpr size_t IpsFileExtensionLength = std::strlen(IpsFileExtension);
        constexpr size_t ModuleIpsPatchLength = 2 * sizeof(ro::ModuleId) + IpsFileExtensionLength;

        /* Global data. */
        os::Mutex apply_patch_lock(false);
        u8 g_patch_read_buffer[os::MemoryPageSize];

        /* Helpers. */
        inline u8 ConvertHexNybble(const char nybble) {
            if ('0' <= nybble && nybble <= '9') {
                return nybble - '0';
            } else if ('a' <= nybble && nybble <= 'f') {
                return nybble - 'a' + 0xa;
            } else {
                return nybble - 'A' + 0xA;
            }
        }

        bool ParseModuleIdFromPath(ro::ModuleId *out_module_id, const char *name, size_t name_len, size_t extension_len) {
            /* Validate name is hex module id. */
            for (unsigned int i = 0; i < name_len - extension_len; i++) {
                if (!std::isxdigit(static_cast<unsigned char>(name[i]))) {
                    return false;
                }
            }

            /* Read module id from name. */
            std::memset(out_module_id, 0, sizeof(*out_module_id));
            for (unsigned int name_ofs = 0, id_ofs = 0; name_ofs < name_len - extension_len && id_ofs < sizeof(*out_module_id); id_ofs++) {
                out_module_id->build_id[id_ofs] |= ConvertHexNybble(name[name_ofs++]) << 4;
                out_module_id->build_id[id_ofs] |= ConvertHexNybble(name[name_ofs++]);
            }

            return true;
        }

        bool MatchesModuleId(const char *name, size_t name_len, size_t extension_len, const ro::ModuleId *module_id) {
            /* Get module id. */
            ro::ModuleId module_id_from_name;
            if (!ParseModuleIdFromPath(&module_id_from_name, name, name_len, extension_len)) {
                return false;
            }

            return std::memcmp(&module_id_from_name, module_id, sizeof(*module_id)) == 0;
        }

        bool IsIpsFileForModule(const char *name, const ro::ModuleId *module_id) {
            const size_t name_len = std::strlen(name);

            /* The path must be correct size for a build id (with trailing zeroes optionally trimmed) + ".ips". */
            if (!(IpsFileExtensionLength < name_len && name_len <= ModuleIpsPatchLength)) {
                return false;
            }

            /* The path must be an even number of characters to conform. */
            if (!util::IsAligned(name_len, 2)) {
                return false;
            }

            /* The path needs to end with .ips. */
            if (std::strcmp(name + name_len - IpsFileExtensionLength, IpsFileExtension) != 0) {
                return false;
            }

            /* The path needs to match the module id. */
            return MatchesModuleId(name, name_len, IpsFileExtensionLength, module_id);
        }

        inline bool IsIpsTail(bool is_ips32, u8 *buffer) {
            if (is_ips32) {
                return std::memcmp(buffer, Ips32TailMagic, sizeof(Ips32TailMagic)) == 0;
            } else {
                return std::memcmp(buffer, IpsTailMagic, sizeof(IpsTailMagic)) == 0;
            }
        }

        inline u32 GetIpsPatchOffset(bool is_ips32, u8 *buffer) {
            if (is_ips32) {
                return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | (buffer[3]);
            } else {
                return (buffer[0] << 16) | (buffer[1] << 8) | (buffer[2]);
            }
        }

        inline u32 GetIpsPatchSize(bool is_ips32, u8 *buffer) {
            return (buffer[0] << 8) | (buffer[1]);
        }

        void ApplyIpsPatch(u8 *mapped_module, size_t mapped_size, size_t protected_size, size_t offset, bool is_ips32, fs::FileHandle file) {
            /* Validate offset/protected size. */
            AMS_ABORT_UNLESS(offset <= protected_size);

            s64 file_offset = sizeof(IpsHeadMagic);
            auto ReadData = [&](void *dst, size_t size) ALWAYS_INLINE_LAMBDA {
                R_ABORT_UNLESS(fs::ReadFile(file, file_offset, dst, size));
                file_offset += size;
            };

            u8 buffer[sizeof(Ips32TailMagic)];
            while (true) {
                ReadData(buffer, is_ips32 ? sizeof(Ips32TailMagic) : sizeof(IpsTailMagic));

                if (IsIpsTail(is_ips32, buffer)) {
                    break;
                }

                /* Offset of patch. */
                u32 patch_offset = GetIpsPatchOffset(is_ips32, buffer);

                /* Size of patch. */
                ReadData(buffer, 2);
                u32 patch_size = GetIpsPatchSize(is_ips32, buffer);

                /* Check for RLE encoding. */
                if (patch_size == 0) {
                    /* Size of RLE. */
                    ReadData(buffer, 2);

                    u32 rle_size = (buffer[0] << 8) | (buffer[1]);

                    /* Value for RLE. */
                    ReadData(buffer, 1);

                    /* Ensure we don't write to protected region. */
                    if (patch_offset < protected_size) {
                        if (patch_offset + rle_size > protected_size) {
                            const u32 diff = protected_size - patch_offset;
                            patch_offset += diff;
                            rle_size -= diff;
                        } else {
                            continue;
                        }
                    }

                    /* Adjust offset, if relevant. */
                    patch_offset -= offset;

                    /* Apply patch. */
                    if (patch_offset + rle_size > mapped_size) {
                        rle_size = mapped_size - patch_offset;
                    }
                    std::memset(mapped_module + patch_offset, buffer[0], rle_size);
                } else {
                    /* Ensure we don't write to protected region. */
                    if (patch_offset < protected_size) {
                        if (patch_offset + patch_size > protected_size) {
                            const u32 diff = protected_size - patch_offset;
                            patch_offset += diff;
                            patch_size -= diff;
                            file_offset += diff;
                        } else {
                            file_offset += patch_size;
                            continue;
                        }
                    }

                    /* Adjust offset, if relevant. */
                    patch_offset -= offset;

                    /* Apply patch. */
                    u32 read_size = patch_size;
                    if (patch_offset + read_size > mapped_size) {
                        read_size = mapped_size - patch_offset;
                    }
                    {
                        size_t remaining   = read_size;
                        size_t copy_offset = patch_offset;
                        while (remaining > 0) {
                            const size_t cur_read = std::min(remaining, sizeof(g_patch_read_buffer));
                            ReadData(g_patch_read_buffer, cur_read);
                            std::memcpy(mapped_module + copy_offset, g_patch_read_buffer, cur_read);
                            remaining   -= cur_read;
                            copy_offset += cur_read;
                        }
                    }
                    if (patch_size > read_size) {
                        file_offset += patch_size - read_size;
                    }
                }
            }
        }

    }

    void LocateAndApplyIpsPatchesToModule(const char *mount_name, const char *patch_dir_name, size_t protected_size, size_t offset, const ro::ModuleId *module_id, u8 *mapped_module, size_t mapped_size) {
        /* Ensure only one thread tries to apply patches at a time. */
        std::scoped_lock lk(apply_patch_lock);

        /* Inspect all patches from /atmosphere/<patch_dir>/<*>/<*>.ips */
        char path[fs::EntryNameLengthMax + 1];
        std::snprintf(path, sizeof(path), "%s:/atmosphere/%s", mount_name, patch_dir_name);
        const size_t patches_dir_path_len = std::strlen(path);

        /* Open the patch directory. */
        fs::DirectoryHandle patches_dir;
        if (R_FAILED(fs::OpenDirectory(std::addressof(patches_dir), path, fs::OpenDirectoryMode_Directory))) {
            return;
        }
        ON_SCOPE_EXIT { fs::CloseDirectory(patches_dir); };

        /* Iterate over the patches directory to find patch subdirectories. */
        while (true) {
            /* Read the next entry. */
            s64 count;
            fs::DirectoryEntry entry;
            if (R_FAILED(fs::ReadDirectory(std::addressof(count), std::addressof(entry), patches_dir, 1)) || count == 0) {
                break;
            }

            /* Print the path for this directory. */
            std::snprintf(path + patches_dir_path_len, sizeof(path) - patches_dir_path_len, "/%s", entry.name);
            const size_t patch_dir_path_len = patches_dir_path_len + 1 + std::strlen(entry.name);

            /* Open the patch directory. */
            fs::DirectoryHandle patch_dir;
            if (R_FAILED(fs::OpenDirectory(std::addressof(patch_dir), path, fs::OpenDirectoryMode_File))) {
                continue;
            }
            ON_SCOPE_EXIT { fs::CloseDirectory(patch_dir); };

            /* Iterate over files in the patch directory. */
            while (true) {
                if (R_FAILED(fs::ReadDirectory(std::addressof(count), std::addressof(entry), patch_dir, 1)) || count == 0) {
                    break;
                }

                /* Check if this file is an ips. */
                if (!IsIpsFileForModule(entry.name, module_id)) {
                    continue;
                }

                /* Print the path for this file. */
                std::snprintf(path + patch_dir_path_len, sizeof(path) - patch_dir_path_len, "/%s", entry.name);

                /* Open the file. */
                fs::FileHandle file;
                if (R_FAILED(fs::OpenFile(std::addressof(file), path, fs::OpenMode_Read))) {
                    continue;
                }
                ON_SCOPE_EXIT { fs::CloseFile(file); };

                /* Read the header. */
                u8 header[sizeof(IpsHeadMagic)];
                if (R_SUCCEEDED(fs::ReadFile(file, 0, header, sizeof(header)))) {
                    if (std::memcmp(header, IpsHeadMagic, sizeof(header)) == 0) {
                        ApplyIpsPatch(mapped_module, mapped_size, protected_size, offset, false, file);
                    } else if (std::memcmp(header, Ips32HeadMagic, sizeof(header)) == 0) {
                        ApplyIpsPatch(mapped_module, mapped_size, protected_size, offset, true, file);
                    }
                }
            }
        }
    }

}
