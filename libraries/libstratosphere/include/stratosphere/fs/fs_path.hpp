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
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/fs/fs_memory_management.hpp>
#include <stratosphere/fs/impl/fs_common_mount_name.hpp>
#include <stratosphere/fs/fs_path_utility.hpp>

namespace ams::fs {

    class DirectoryPathParser;

    class Path {
        NON_COPYABLE(Path);
        NON_MOVEABLE(Path);
        private:
            static constexpr const char *EmptyPath = "";
            static constexpr size_t WriteBufferAlignmentLength = 8;
        private:
            friend class DirectoryPathParser;
        private:
            using WriteBuffer = std::unique_ptr<char[], ::ams::fs::impl::Deleter>;
        private:
            const char *m_str;
            util::TypedStorage<WriteBuffer> m_write_buffer;
            size_t m_write_buffer_length;
            bool m_is_normalized;
        public:
            Path() : m_str(EmptyPath), m_write_buffer_length(0), m_is_normalized(false) {
                util::ConstructAt(m_write_buffer, nullptr);
            }

            constexpr Path(const char *s, util::ConstantInitializeTag) : m_str(s), m_write_buffer(), m_write_buffer_length(0), m_is_normalized(true) {
                /* ... */
            }

            constexpr ~Path() {
                if (!std::is_constant_evaluated()) {
                    util::DestroyAt(m_write_buffer);
                }
            }

            WriteBuffer ReleaseBuffer() {
                /* Check pre-conditions. */
                AMS_ASSERT(util::GetReference(m_write_buffer) != nullptr);

                /* Reset. */
                m_str                 = EmptyPath;
                m_write_buffer_length = 0;

                /* Return our write buffer. */
                return std::move(util::GetReference(m_write_buffer));
            }

            constexpr Result SetShallowBuffer(const char *buffer) {
                /* Check pre-conditions. */
                AMS_ASSERT(m_write_buffer_length == 0);

                /* Check the buffer is valid. */
                R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

                /* Set buffer. */
                this->SetReadOnlyBuffer(buffer);

                /* Note that we're normalized. */
                m_is_normalized = true;

                R_SUCCEED();
            }

            const char *GetString() const {
                /* Check pre-conditions. */
                AMS_ASSERT(m_is_normalized);

                return m_str;
            }

            size_t GetLength() const {
                return std::strlen(this->GetString());
            }

            bool IsEmpty() const {
                return *m_str == '\x00';
            }

            bool IsMatchHead(const char *p, size_t len) const {
                return util::Strncmp(this->GetString(), p, len) == 0;
            }

            Result Initialize(const Path &rhs) {
                /* Check the other path is normalized. */
                R_UNLESS(rhs.m_is_normalized, fs::ResultNotNormalized());

                /* Allocate buffer for our path. */
                const auto len = rhs.GetLength();
                R_TRY(this->Preallocate(len + 1));

                /* Copy the path. */
                const size_t copied = util::Strlcpy<char>(util::GetReference(m_write_buffer).get(), rhs.GetString(), len + 1);
                R_UNLESS(copied == len, fs::ResultUnexpectedInPathA());

                /* Set normalized. */
                m_is_normalized = rhs.m_is_normalized;
                R_SUCCEED();
            }

            Result Initialize(const char *path, size_t len) {
                /* Check the path is valid. */
                R_UNLESS(path != nullptr, fs::ResultNullptrArgument());

                /* Initialize. */
                R_TRY(this->InitializeImpl(path, len));

                /* Set not normalized. */
                m_is_normalized = false;

                R_SUCCEED();
            }

            Result Initialize(const char *path) {
                /* Check the path is valid. */
                R_UNLESS(path != nullptr, fs::ResultNullptrArgument());

                return this->Initialize(path, std::strlen(path));
            }

            Result InitializeWithFormat(const char *fmt, ...) __attribute__((format (printf, 2, 3))) {
                /* Check the format string is valid. */
                R_UNLESS(fmt != nullptr, fs::ResultNullptrArgument());

                /* Create the va_list for formatting. */
                std::va_list vl;
                va_start(vl, fmt);

                /* Determine how big the string will be. */
                char dummy;
                const auto len = util::VSNPrintf(std::addressof(dummy), 0, fmt, vl);

                /* Allocate buffer for our path. */
                R_TRY(this->Preallocate(len + 1));

                /* Format our path into our new buffer. */
                const auto real_len = util::VSNPrintf(util::GetReference(m_write_buffer).get(), m_write_buffer_length, fmt, vl);
                AMS_ASSERT(real_len == len);
                AMS_UNUSED(real_len);

                /* Finish the va_list. */
                va_end(vl);

                /* Set not normalized. */
                m_is_normalized = false;

                R_SUCCEED();
            }

            Result InitializeWithReplaceBackslash(const char *path) {
                /* Check the path is valid. */
                R_UNLESS(path != nullptr, fs::ResultNullptrArgument());

                /* Initialize. */
                R_TRY(this->InitializeImpl(path, std::strlen(path)));

                /* Replace slashes as desired. */
                if (m_write_buffer_length > 1) {
                    fs::Replace(this->GetWriteBuffer(), m_write_buffer_length - 1, '\\', '/');
                }

                /* Set not normalized. */
                m_is_normalized = false;

                R_SUCCEED();
            }

            Result InitializeWithReplaceForwardSlashes(const char *path) {
                /* Check the path is valid. */
                R_UNLESS(path != nullptr, fs::ResultNullptrArgument());

                /* Initialize. */
                R_TRY(this->InitializeImpl(path, std::strlen(path)));

                /* Replace slashes as desired. */
                if (m_write_buffer_length > 1) {
                    if (auto *p = this->GetWriteBuffer(); p[0] == '/' && p[1] == '/') {
                        p[0] = '\\';
                        p[1] = '\\';
                    }
                }

                /* Set not normalized. */
                m_is_normalized = false;

                R_SUCCEED();
            }

            Result InitializeWithReplaceUnc(const char *path) {
                /* Check the path is valid. */
                R_UNLESS(path != nullptr, fs::ResultNullptrArgument());

                /* Initialize. */
                R_TRY(this->InitializeImpl(path, std::strlen(path)));

                /* Set not normalized. */
                m_is_normalized = false;

                /* Replace unc as desired. */
                if (m_str[0]) {
                    auto *p = this->GetWriteBuffer();

                    /* Replace :/// -> \\ as needed. */
                    if (auto *sep = std::strstr(p, ":///"); sep != nullptr) {
                        sep[0] = '\\';
                        sep[1] = '\\';
                    }

                    /* Edit path prefix. */
                    if (!util::Strncmp(p, AMS_FS_IMPL_HOST_ROOT_FILE_SYSTEM_MOUNT_NAME AMS_FS_IMPL_MOUNT_NAME_DELIMITER "/", AMS_FS_IMPL_HOST_ROOT_FILE_SYSTEM_MOUNT_NAME_LEN + AMS_FS_IMPL_MOUNT_NAME_DELIMITER_LEN + 1)) {
                        static_assert((AMS_FS_IMPL_HOST_ROOT_FILE_SYSTEM_MOUNT_NAME AMS_FS_IMPL_MOUNT_NAME_DELIMITER)[AMS_FS_IMPL_HOST_ROOT_FILE_SYSTEM_MOUNT_NAME_LEN + AMS_FS_IMPL_MOUNT_NAME_DELIMITER_LEN - 1] == '/');

                        p[AMS_FS_IMPL_HOST_ROOT_FILE_SYSTEM_MOUNT_NAME_LEN + AMS_FS_IMPL_MOUNT_NAME_DELIMITER_LEN - 1] = '\\';
                        p[AMS_FS_IMPL_HOST_ROOT_FILE_SYSTEM_MOUNT_NAME_LEN + AMS_FS_IMPL_MOUNT_NAME_DELIMITER_LEN - 0] = '\\';
                    }

                    if (p[0] == '/' && p[1] == '/') {
                        p[0] = '\\';
                        p[1] = '\\';
                    }
                }

                R_SUCCEED();
            }

            Result InitializeWithNormalization(const char *path, size_t size) {
                /* Check the path is valid. */
                R_UNLESS(path != nullptr, fs::ResultNullptrArgument());

                /* Initialize. */
                R_TRY(this->InitializeImpl(path, size));

                /* Set not normalized. */
                m_is_normalized = false;

                /* Perform normalization. */
                fs::PathFlags path_flags;
                if (fs::IsPathRelative(m_str)) {
                    path_flags.AllowRelativePath();
                } else if (fs::IsWindowsPath(m_str, true)) {
                    path_flags.AllowWindowsPath();
                } else {
                    /* NOTE: In this case, Nintendo checks is normalized, then sets is normalized, then returns success. */
                    /* This seems like a bug. */
                    size_t dummy;
                    R_TRY(PathFormatter::IsNormalized(std::addressof(m_is_normalized), std::addressof(dummy), m_str));

                    m_is_normalized = true;
                    R_SUCCEED();
                }

                /* Normalize. */
                R_TRY(this->Normalize(path_flags));

                m_is_normalized = true;
                R_SUCCEED();
            }

            Result InitializeWithNormalization(const char *path) {
                /* Check the path is valid. */
                R_UNLESS(path != nullptr, fs::ResultNullptrArgument());

                R_RETURN(this->InitializeWithNormalization(path, std::strlen(path)));
            }

            Result InitializeAsEmpty() {
                /* Clear our buffer. */
                this->ClearBuffer();

                /* Set normalized. */
                m_is_normalized = true;

                R_SUCCEED();
            }

            Result AppendChild(const char *child) {
                /* Check the path is valid. */
                R_UNLESS(child != nullptr, fs::ResultNullptrArgument());

                /* Basic checks. If we hvea a path and the child is empty, we have nothing to do. */
                const char *c = child;
                if (m_str[0]) {
                    /* Skip an early separator. */
                    if (*c == '/') {
                        ++c;
                    }

                    R_SUCCEED_IF(*c == '\x00');
                }

                /* If we don't have a string, we can just initialize. */
                auto cur_len = std::strlen(m_str);
                if (cur_len == 0) {
                    R_RETURN(this->Initialize(child));
                }

                /* Remove a trailing separator. */
                if (m_str[cur_len - 1] == '/' || m_str[cur_len - 1] == '\\') {
                    --cur_len;
                }

                /* Get the child path's length. */
                auto child_len = std::strlen(c);

                /* Reset our write buffer. */
                WriteBuffer old_write_buffer;
                if (util::GetReference(m_write_buffer) != nullptr) {
                    old_write_buffer = std::move(util::GetReference(m_write_buffer));
                    this->ClearBuffer();
                }

                /* Pre-allocate the new buffer. */
                R_TRY(this->Preallocate(cur_len + 1 + child_len + 1));

                /* Get our write buffer. */
                auto *dst = this->GetWriteBuffer();
                if (old_write_buffer != nullptr && cur_len > 0) {
                    util::Strlcpy<char>(dst, old_write_buffer.get(), cur_len + 1);
                }

                /* Add separator. */
                dst[cur_len] = '/';

                /* Copy the child path. */
                const size_t copied = util::Strlcpy<char>(dst + cur_len + 1, c, child_len + 1);
                R_UNLESS(copied == child_len, fs::ResultUnexpectedInPathA());

                R_SUCCEED();
            }

            Result AppendChild(const Path &rhs) {
                R_RETURN(this->AppendChild(rhs.GetString()));
            }

            Result Combine(const Path &parent, const Path &child) {
                /* Get the lengths. */
                const auto p_len = parent.GetLength();
                const auto c_len = child.GetLength();

                /* Allocate our buffer. */
                R_TRY(this->Preallocate(p_len + c_len + 1));

                /* Initialize as parent. */
                R_TRY(this->Initialize(parent));

                /* If we're empty, we can just initialize as child. */
                if (this->IsEmpty()) {
                    R_TRY(this->Initialize(child));
                } else {
                    /* Otherwise, we should append the child. */
                    R_TRY(this->AppendChild(child));
                }

                R_SUCCEED();
            }

            Result RemoveChild() {
                /* If we don't have a write-buffer, ensure that we have one. */
                if (util::GetReference(m_write_buffer) == nullptr) {
                    if (const auto len = std::strlen(m_str); len > 0) {
                        R_TRY(this->Preallocate(len));
                        util::Strlcpy<char>(util::GetReference(m_write_buffer).get(), m_str, len + 1);
                    }
                }

                /* Check that it's possible for us to remove a child. */
                auto *p = this->GetWriteBuffer();
                s32 len = std::strlen(p);
                R_UNLESS(len != 1 || (p[0] != '/' && p[0] != '.'), fs::ResultNotImplemented());

                /* Handle a trailing separator. */
                if (len > 0 && (p[len - 1] == '\\' || p[len - 1] == '/')) {
                    --len;
                }

                /* Remove the child path segment. */
                while ((--len) >= 0 && p[len]) {
                    if (p[len] == '/' || p[len] == '\\') {
                        if (len > 0) {
                            p[len] = 0;
                        } else {
                            p[1] = 0;
                            len  = 1;
                        }
                        break;
                    }
                }

                /* Check that length remains > 0. */
                R_UNLESS(len > 0, fs::ResultNotImplemented());

                R_SUCCEED();
            }

            Result Normalize(const PathFlags &flags) {
                /* If we're already normalized, nothing to do. */
                R_SUCCEED_IF(m_is_normalized);

                /* Check if we're normalized. */
                bool normalized;
                size_t dummy;
                R_TRY(PathFormatter::IsNormalized(std::addressof(normalized), std::addressof(dummy), m_str, flags));

                /* If we're not normalized, normalize. */
                if (!normalized) {
                    /* Determine necessary buffer length. */
                    auto len = m_write_buffer_length;
                    if (flags.IsRelativePathAllowed() && fs::IsPathRelative(m_str)) {
                        len += 2;
                    }
                    if (flags.IsWindowsPathAllowed() && fs::IsWindowsPath(m_str, true)) {
                        len += 1;
                    }

                    /* Allocate a new buffer. */
                    const size_t size = util::AlignUp(len, WriteBufferAlignmentLength);
                    auto buf = fs::impl::MakeUnique<char[]>(size);
                    R_UNLESS(buf != nullptr, fs::ResultAllocationFailureInMakeUnique());

                    /* Normalize into it. */
                    R_TRY(PathFormatter::Normalize(buf.get(), size, util::GetReference(m_write_buffer).get(), m_write_buffer_length, flags));

                    /* Set the normalized buffer as our buffer. */
                    this->SetModifiableBuffer(std::move(buf), size);
                }

                /* Set normalized. */
                m_is_normalized = true;
                R_SUCCEED();
            }
        private:
            void ClearBuffer() {
                util::GetReference(m_write_buffer).reset();
                m_write_buffer_length = 0;
                m_str = EmptyPath;
            }

            void SetModifiableBuffer(WriteBuffer &&buffer, size_t size) {
                /* Check pre-conditions. */
                AMS_ASSERT(buffer.get() != nullptr);
                AMS_ASSERT(size > 0);
                AMS_ASSERT(util::IsAligned(size, WriteBufferAlignmentLength));

                /* Set write buffer. */
                util::GetReference(m_write_buffer) = std::move(buffer);
                m_write_buffer_length              = size;
                m_str                              = util::GetReference(m_write_buffer).get();
            }

            constexpr void SetReadOnlyBuffer(const char *buffer) {
                m_str = buffer;
                if (!std::is_constant_evaluated()) {
                    util::GetReference(m_write_buffer) = nullptr;
                    m_write_buffer_length              = 0;
                }
            }

            Result Preallocate(size_t length) {
                /* Allocate additional space, if needed. */
                if (length > m_write_buffer_length) {
                    /* Allocate buffer. */
                    const size_t size = util::AlignUp(length, WriteBufferAlignmentLength);
                    auto buf = fs::impl::MakeUnique<char[]>(size);
                    R_UNLESS(buf != nullptr, fs::ResultAllocationFailureInMakeUnique());

                    /* Set write buffer. */
                    this->SetModifiableBuffer(std::move(buf), size);
                }

                R_SUCCEED();
            }

            Result InitializeImpl(const char *path, size_t size) {
                if (size > 0 && path[0]) {
                    /* Pre allocate a buffer for the path. */
                    R_TRY(this->Preallocate(size + 1));

                    /* Copy the path. */
                    const size_t copied = util::Strlcpy<char>(this->GetWriteBuffer(), path, size + 1);
                    R_UNLESS(copied >= size, fs::ResultUnexpectedInPathA());
                } else {
                    /* We can just clear the buffer. */
                    this->ClearBuffer();
                }

                R_SUCCEED();
            }

            char *GetWriteBuffer() {
                AMS_ASSERT(util::GetReference(m_write_buffer) != nullptr);
                return util::GetReference(m_write_buffer).get();
            }

            size_t GetWriteBufferLength() const {
                return m_write_buffer_length;
            }
        public:
            ALWAYS_INLINE bool operator==(const fs::Path &rhs) const { return std::strcmp(this->GetString(), rhs.GetString()) == 0; }
            ALWAYS_INLINE bool operator!=(const fs::Path &rhs) const { return !(*this == rhs); }
            ALWAYS_INLINE bool operator==(const char *p) const { return std::strcmp(this->GetString(), p) == 0; }
            ALWAYS_INLINE bool operator!=(const char *p) const { return !(*this == p); }
    };

    consteval fs::Path MakeConstantPath(const char *s) { return fs::Path(s, util::ConstantInitializeTag{}); }

    inline Result SetUpFixedPath(fs::Path *out, const char *s) {
        /* Verify the path is normalized. */
        bool normalized;
        size_t dummy;
        R_TRY(PathNormalizer::IsNormalized(std::addressof(normalized), std::addressof(dummy), s));

        R_UNLESS(normalized, fs::ResultInvalidPathFormat());

        /* Set the fixed path. */
        R_RETURN(out->SetShallowBuffer(s));
    }

    inline Result SetUpFixedPathSingleEntry(fs::Path *out, char *buf, size_t buf_size, const char *e) {
        /* Print the path into the buffer. */
        const size_t len = util::TSNPrintf(buf, buf_size, "/%s", e);
        R_UNLESS(len < buf_size, fs::ResultInvalidArgument());

        /* Set up the path. */
        R_RETURN(SetUpFixedPath(out, buf));
    }

    inline Result SetUpFixedPathDoubleEntry(fs::Path *out, char *buf, size_t buf_size, const char *e, const char *e2) {
        /* Print the path into the buffer. */
        const size_t len = util::TSNPrintf(buf, buf_size, "/%s/%s", e, e2);
        R_UNLESS(len < buf_size, fs::ResultInvalidArgument());

        /* Set up the path. */
        R_RETURN(SetUpFixedPath(out, buf));
    }

    inline Result SetUpFixedPathSaveId(fs::Path *out, char *buf, size_t buf_size, u64 id) {
        /* Print the path into the buffer. */
        const size_t len = util::TSNPrintf(buf, buf_size, "/%016" PRIx64 "", id);
        R_UNLESS(len < buf_size, fs::ResultInvalidArgument());

        /* Set up the path. */
        R_RETURN(SetUpFixedPath(out, buf));
    }

    inline Result SetUpFixedPathSaveMetaName(fs::Path *out, char *buf, size_t buf_size, u32 type) {
        /* Print the path into the buffer. */
        const size_t len = util::TSNPrintf(buf, buf_size, "/%08" PRIx32 ".meta", type);
        R_UNLESS(len < buf_size, fs::ResultInvalidArgument());

        /* Set up the path. */
        R_RETURN(SetUpFixedPath(out, buf));
    }

    inline Result SetUpFixedPathSaveMetaDir(fs::Path *out, char *buf, size_t buf_size, u64 id) {
        /* Print the path into the buffer. */
        const size_t len = util::TSNPrintf(buf, buf_size, "/saveMeta/%016" PRIx64 "", id);
        R_UNLESS(len < buf_size, fs::ResultInvalidArgument());

        /* Set up the path. */
        R_RETURN(SetUpFixedPath(out, buf));
    }

}
