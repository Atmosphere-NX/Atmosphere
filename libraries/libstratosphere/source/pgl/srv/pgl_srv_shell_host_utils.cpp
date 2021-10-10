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
#include "pgl_srv_shell_host_utils.hpp"
#include "pgl_srv_shell.hpp"

namespace ams::pgl::srv {

    namespace {

        constexpr inline char HostPackageMountName[] = "HostPackageRead";
        static_assert(sizeof(HostPackageMountName) - 1 <= fs::MountNameLengthMax);

        struct CaseInsensitiveCharTraits : public std::char_traits<char> {
            static constexpr char to_upper(char c) {
                return std::toupper(static_cast<unsigned char>(c));
            }
            static constexpr bool eq(char c1, char c2) {
                 return to_upper(c1) == to_upper(c2);
             }
            static constexpr bool lt(char c1, char c2) {
                 return to_upper(c1) <  to_upper(c2);
            }
            static constexpr int compare(const char *s1, const char *s2, size_t n) {
                while ( n-- != 0 ) {
                    if ( to_upper(*s1) < to_upper(*s2) ) return -1;
                    if ( to_upper(*s1) > to_upper(*s2) ) return 1;
                    ++s1; ++s2;
                }
                return 0;
            }
            static constexpr const char *find(const char *s, int n, char a) {
                auto const ua (to_upper(a));
                while ( n-- != 0 )
                {
                    if (to_upper(*s) == ua)
                        return s;
                    s++;
                }
                return nullptr;
            }
        };

        using PathView = util::basic_string_view<char, CaseInsensitiveCharTraits>;

        enum class ExtensionType {
            None = 0,
            Nsp  = 1,
            Nspd = 2,
        };

        bool HasSuffix(const char *str, const char *suffix) {
            const size_t suffix_len = std::strlen(suffix);
            const size_t str_len    = std::strlen(str);
            if (suffix_len > str_len) {
                return false;
            }
            return (PathView(str).substr(str_len - suffix_len) == PathView(suffix));
        }

        class HostPackageReader {
            NON_COPYABLE(HostPackageReader);
            NON_MOVEABLE(HostPackageReader);
            private:
                char m_content_path[fs::EntryNameLengthMax] = {};
                ExtensionType m_extension_type              = ExtensionType::None;
                char m_mount_name[fs::MountNameLengthMax]   = {};
                bool m_is_mounted                           = false;
                ncm::AutoBuffer m_content_meta_buffer;
                ncm::ProgramId m_program_id                 = ncm::InvalidProgramId;
                u32 m_program_version                       = 0;
                ncm::ContentMetaType m_content_meta_type    = static_cast<ncm::ContentMetaType>(0);
                u8 m_program_index                          = 0;
            public:
                HostPackageReader() : m_content_meta_buffer() { /* ... */ }
                ~HostPackageReader() {
                    if (m_is_mounted) {
                        fs::Unmount(m_mount_name);
                    }
                }

                Result Initialize(const char *package, const char *mount) {
                    /* Copy in the content path. */
                    R_UNLESS(strlen(package) <= sizeof(m_content_path) - 1, pgl::ResultBufferNotEnough());
                    std::strcpy(m_content_path, package);

                    /* Set the extension type. */
                    R_TRY(this->SetExtensionType());

                    /* Copy in mount name. */
                    R_UNLESS(strlen(mount) <= sizeof(m_mount_name) - 1, pgl::ResultBufferNotEnough());
                    std::strcpy(m_mount_name, mount);

                    /* Mount the package. */
                    R_TRY(fs::MountApplicationPackage(m_mount_name, m_content_path));
                    m_is_mounted = true;

                    /* Set the content meta buffer. */
                    R_TRY(this->SetContentMetaBuffer());

                    /* Ensure we have a content meta buffer. */
                    R_UNLESS(m_content_meta_buffer.Get() != nullptr, pgl::ResultContentMetaNotFound());

                    return ResultSuccess();
                }

                Result ReadProgramInfo() {
                    /* First, read the program index. */
                    R_TRY(this->GetProgramIndex(std::addressof(m_program_index)));

                    /* Next, create a key for the rest of the fields. */
                    const auto key = ncm::PackagedContentMetaReader(m_content_meta_buffer.Get(), m_content_meta_buffer.GetSize()).GetKey();

                    /* Set fields. */
                    m_program_id        = {key.id};
                    m_program_version   = key.version;
                    m_content_meta_type = key.type;
                    return ResultSuccess();
                }

                Result GetContentPath(lr::Path *out, ncm::ContentType type, util::optional<u8> index) const {
                    switch (m_extension_type) {
                        case ExtensionType::Nsp:  return this->GetContentPathInNsp(out, type, index);
                        case ExtensionType::Nspd: return this->GetContentPathInNspd(out, type, index);
                        AMS_UNREACHABLE_DEFAULT_CASE();
                    }
                }

                ncm::ProgramId GetProgramId() const {
                    return m_program_id;
                }

                u32 GetProgramVersion() const {
                    return m_program_version;
                }

                ncm::ContentMetaType GetContentMetaType() const {
                    return m_content_meta_type;
                }

                u8 GetProgramIndex() const {
                    return m_program_index;
                }
            private:
                Result GetContentPathInNsp(lr::Path *out, ncm::ContentType type, util::optional<u8> index) const {
                    /* Create a reader. */
                    auto reader = ncm::PackagedContentMetaReader(m_content_meta_buffer.Get(), m_content_meta_buffer.GetSize());

                    /* Get the content info. */
                    const ncm::PackagedContentInfo *content_info = nullptr;
                    if (index) {
                        content_info = reader.GetContentInfo(type, *index);
                    } else {
                        content_info = reader.GetContentInfo(type);
                    }
                    R_UNLESS(content_info != nullptr, pgl::ResultApplicationContentNotFound());

                    /* Get the content id string. */
                    ncm::ContentIdString id_str;
                    ncm::GetStringFromContentId(id_str.data, sizeof(id_str.data), content_info->GetId());

                    /* Get the file name. */
                    char file_name[ncm::ContentIdStringLength + 5];
                    const size_t len = util::SNPrintf(file_name, sizeof(file_name), "%s.nca", id_str.data);
                    R_UNLESS(len + 1 == sizeof(file_name), pgl::ResultBufferNotEnough());

                    /* Ensure we have the content. */
                    bool has_content;
                    R_TRY(this->SearchContent(std::addressof(has_content), out, file_name, fs::OpenDirectoryMode_File));
                    R_UNLESS(has_content, pgl::ResultApplicationContentNotFound());

                    return ResultSuccess();
                }

                Result GetContentPathInNspd(lr::Path *out, ncm::ContentType type, util::optional<u8> index) const {
                    AMS_UNUSED(index);

                    /* Get the content name. */
                    const char *content_name = nullptr;
                    switch (type) {
                        case ncm::ContentType::Program:          content_name = "program";          break;
                        case ncm::ContentType::Control:          content_name = "control";          break;
                        case ncm::ContentType::HtmlDocument:     content_name = "htmlDocument";     break;
                        case ncm::ContentType::LegalInformation: content_name = "legalInformation"; break;
                        AMS_UNREACHABLE_DEFAULT_CASE();
                    }

                    /* Get the file name. */
                    /* NSPD does not support indexed content, so we always use 0 as the index. */
                    char file_name[0x20];
                    const size_t len = util::SNPrintf(file_name, sizeof(file_name), "%s%d.ncd", content_name, 0);
                    R_UNLESS(len + 1 <= sizeof(file_name), pgl::ResultBufferNotEnough());

                    /* Ensure we have the content. */
                    bool has_content;
                    R_TRY(this->SearchContent(std::addressof(has_content), out, file_name, fs::OpenDirectoryMode_Directory));
                    R_UNLESS(has_content, pgl::ResultApplicationContentNotFound());

                    return ResultSuccess();
                }

                Result GetProgramIndex(u8 *out) {
                    /* Nspd programs do not have indices. */
                    if (m_extension_type == ExtensionType::Nspd) {
                        *out = 0;
                        return ResultSuccess();
                    }

                    /* Create a reader. */
                    auto reader = ncm::PackagedContentMetaReader(m_content_meta_buffer.Get(), m_content_meta_buffer.GetSize());

                    /* Get the program content info. */
                    auto program_content_info = reader.GetContentInfo(ncm::ContentType::Program);
                    R_UNLESS(program_content_info, pgl::ResultApplicationContentNotFound());

                    /* Return the index. */
                    *out = program_content_info->GetIdOffset();
                    return ResultSuccess();
                }

                Result SetExtensionType() {
                    /* First, clear the suffix if the path is a program ncd. */
                    if (HasSuffix(m_content_path, "program0.ncd/")) {
                        m_content_path[strnlen(m_content_path, sizeof(m_content_path)) - std::strlen("program0.ncd/")] = 0;
                    }

                    if (HasSuffix(m_content_path, ".nsp")) {
                        m_extension_type = ExtensionType::Nsp;
                        return ResultSuccess();
                    } else if (HasSuffix(m_content_path, ".nspd") || HasSuffix(m_content_path, ".nspd/")) {
                        m_extension_type = ExtensionType::Nspd;
                        return ResultSuccess();
                    } else {
                        return fs::ResultPathNotFound();
                    }
                }

                Result SetContentMetaBuffer() {
                    constexpr const char ContentMetaFileExtension[] = ".cnmt.nca";
                    constexpr const char ContentMetaDirectoryExtension[] = "meta0.ncd";

                    /* Find the Content meta path. */
                    bool has_content = false;
                    lr::Path meta_path;
                    switch (m_extension_type) {
                        case ExtensionType::Nsp:   R_TRY(this->SearchContent(std::addressof(has_content), std::addressof(meta_path), ContentMetaFileExtension, fs::OpenDirectoryMode_File)); break;
                        case ExtensionType::Nspd:  R_TRY(this->SearchContent(std::addressof(has_content), std::addressof(meta_path), ContentMetaDirectoryExtension, fs::OpenDirectoryMode_Directory)); break;
                        AMS_UNREACHABLE_DEFAULT_CASE();
                    }
                    R_UNLESS(has_content, pgl::ResultContentMetaNotFound());

                    /* Read the content meta buffer. */
                    return ncm::ReadContentMetaPath(std::addressof(m_content_meta_buffer), meta_path.str);
                }

                Result SearchContent(bool *out, lr::Path *out_path, const char *extension, fs::OpenDirectoryMode mode) const {
                    /* Generate the root directory path. */
                    char root_dir[sizeof(m_mount_name) + 2];
                    util::SNPrintf(root_dir, sizeof(root_dir), "%s:/", m_mount_name);

                    /* Open the root directory. */
                    fs::DirectoryHandle dir;
                    R_TRY(fs::OpenDirectory(std::addressof(dir), root_dir, mode));
                    ON_SCOPE_EXIT { fs::CloseDirectory(dir); };

                    /* Iterate over directory entries. */
                    while (true) {
                        fs::DirectoryEntry entry;
                        s64 count;
                        R_TRY(fs::ReadDirectory(std::addressof(count), std::addressof(entry), dir, 1));
                        if (count == 0) {
                            break;
                        }

                        /* Check if we match the suffix. */
                        if (HasSuffix(entry.name, extension)) {
                            *out = true;
                            if (out_path) {
                                const size_t len = util::SNPrintf(out_path->str, sizeof(out_path->str), "%s/%s", m_content_path, entry.name);
                                R_UNLESS(len + 1 < sizeof(out_path->str), pgl::ResultBufferNotEnough());
                                if (entry.type == fs::DirectoryEntryType_Directory) {
                                    out_path->str[len]     = '/';
                                    out_path->str[len + 1] = 0;
                                }
                            }

                            return ResultSuccess();
                        }
                    }

                    /* We didn't find a match. */
                    *out = false;
                    return ResultSuccess();
                }
        };

    }

    Result LaunchProgramFromHost(os::ProcessId *out, const char *package_path, u32 pm_flags) {
        /* Read the package. */
        HostPackageReader reader;
        R_TRY(reader.Initialize(package_path, HostPackageMountName));

        /* Read the program info. */
        R_TRY(reader.ReadProgramInfo());

        /* Open a host location resolver. */
        lr::LocationResolver host_resolver;
        R_TRY(lr::OpenLocationResolver(std::addressof(host_resolver), ncm::StorageId::Host));

        /* Get the content path. */
        lr::Path content_path;
        R_TRY(reader.GetContentPath(std::addressof(content_path), ncm::ContentType::Program, reader.GetProgramIndex()));

        /* Erase the program redirection. */
        R_TRY(host_resolver.EraseProgramRedirection(reader.GetProgramId()));

        /* Redirect the program path to point to the new path. */
        host_resolver.RedirectProgramPath(content_path, reader.GetProgramId());

        /* Launch the program. */
        return pgl::srv::LaunchProgram(out, ncm::ProgramLocation::Make(reader.GetProgramId(), ncm::StorageId::Host), pm_flags, pgl::LaunchFlags_None);
    }

    Result GetHostContentMetaInfo(pgl::ContentMetaInfo *out, const char *package_path) {
        /* Read the package. */
        HostPackageReader reader;
        R_TRY(reader.Initialize(package_path, HostPackageMountName));

        /* Read the program info. */
        R_TRY(reader.ReadProgramInfo());

        /* Get the content meta info. */
        *out = {
            .id           = reader.GetProgramId().value,
            .version      = reader.GetProgramVersion(),
            .content_type = ncm::ContentType::Program,
            .id_offset    = reader.GetProgramIndex(),
        };

        return ResultSuccess();
    }

}
