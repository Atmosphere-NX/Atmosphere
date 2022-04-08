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

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    /* NOTE: Intentional inaccuracy in custom WriteBuffer class, to save 0x10 bytes (0x28 -> 0x18) over Nintendo's implementation. */
    class Path {
        NON_COPYABLE(Path);
        NON_MOVEABLE(Path);
        private:
            static constexpr const char *EmptyPath = "";
            static constexpr size_t WriteBufferAlignmentLength = 8;
        private:
            friend class DirectoryPathParser;
        public:
            class WriteBuffer {
                NON_COPYABLE(WriteBuffer);
                private:
                    char *m_buffer;
                    size_t m_length_and_is_normalized;
                public:
                    constexpr WriteBuffer() : m_buffer(nullptr), m_length_and_is_normalized(0) { /* ... */ }

                    constexpr ~WriteBuffer() {
                        if (m_buffer != nullptr) {
                            ::ams::fs::impl::Deallocate(m_buffer, this->GetLength());
                            this->ResetBuffer();
                        }
                    }

                    constexpr WriteBuffer(WriteBuffer &&rhs) : m_buffer(rhs.m_buffer), m_length_and_is_normalized(rhs.m_length_and_is_normalized) {
                        rhs.ResetBuffer();
                    }

                    constexpr WriteBuffer &operator=(WriteBuffer &&rhs) {
                        if (m_buffer != nullptr) {
                            ::ams::fs::impl::Deallocate(m_buffer, this->GetLength());
                        }

                        m_buffer                   = rhs.m_buffer;
                        m_length_and_is_normalized = rhs.m_length_and_is_normalized;

                        rhs.ResetBuffer();

                        return *this;
                    }

                    std::unique_ptr<char[], ::ams::fs::impl::Deleter> ReleaseBuffer() {
                        auto released = std::unique_ptr<char[], ::ams::fs::impl::Deleter>(m_buffer, ::ams::fs::impl::Deleter(this->GetLength()));
                        this->ResetBuffer();
                        return released;
                    }

                    constexpr ALWAYS_INLINE void ResetBuffer() {
                        m_buffer = nullptr;
                        this->SetLength(0);
                    }

                    constexpr ALWAYS_INLINE char *Get() const {
                        return m_buffer;
                    }

                    constexpr ALWAYS_INLINE size_t GetLength() const {
                        return m_length_and_is_normalized >> 1;
                    }

                    constexpr ALWAYS_INLINE bool IsNormalized() const {
                        return static_cast<bool>(m_length_and_is_normalized & 1);
                    }

                    constexpr ALWAYS_INLINE void SetNormalized() {
                        m_length_and_is_normalized |= static_cast<size_t>(1);
                    }

                    constexpr ALWAYS_INLINE void SetNotNormalized() {
                        m_length_and_is_normalized &= ~static_cast<size_t>(1);
                    }
                private:
                    constexpr ALWAYS_INLINE WriteBuffer(char *buffer, size_t length) : m_buffer(buffer), m_length_and_is_normalized(0) {
                        this->SetLength(length);
                    }
                public:
                    static WriteBuffer Make(size_t length) {
                        if (void *alloc = ::ams::fs::impl::Allocate(length); alloc != nullptr) {
                            return WriteBuffer(static_cast<char *>(alloc), length);
                        } else {
                            return WriteBuffer();
                        }
                    }
                private:

                    constexpr ALWAYS_INLINE void SetLength(size_t size) {
                        m_length_and_is_normalized = (m_length_and_is_normalized & 1) | (size << 1);
                    }
            };
        private:
            const char *m_str;
            WriteBuffer m_write_buffer;
        public:
            constexpr Path() : m_str(EmptyPath), m_write_buffer() {
                /* ... */
            }

            constexpr Path(const char *s, util::ConstantInitializeTag) : m_str(s), m_write_buffer() {
                m_write_buffer.SetNormalized();
            }

            constexpr ~Path() { /* ... */ }

            std::unique_ptr<char[], ::ams::fs::impl::Deleter> ReleaseBuffer() {
                /* Check pre-conditions. */
                AMS_ASSERT(m_write_buffer.Get() != nullptr);

                /* Reset. */
                m_str = EmptyPath;

                /* Release our write buffer. */
                return m_write_buffer.ReleaseBuffer();
            }

            constexpr Result SetShallowBuffer(const char *buffer) {
                /* Check pre-conditions. */
                AMS_ASSERT(m_write_buffer.GetLength() == 0);

                /* Check the buffer is valid. */
                R_UNLESS(buffer != nullptr, fs::ResultNullptrArgument());

                /* Set buffer. */
                this->SetReadOnlyBuffer(buffer);

                /* Note that we're normalized. */
                this->SetNormalized();

                R_SUCCEED();
            }

            constexpr const char *GetString() const {
                /* Check pre-conditions. */
                AMS_ASSERT(this->IsNormalized());

                return m_str;
            }

            constexpr size_t GetLength() const {
                if (std::is_constant_evaluated()) {
                    return util::Strlen(this->GetString());
                } else {
                    return std::strlen(this->GetString());
                }
            }

            constexpr bool IsEmpty() const {
                return *m_str == '\x00';
            }

           constexpr  bool IsMatchHead(const char *p, size_t len) const {
                return util::Strncmp(this->GetString(), p, len) == 0;
            }

            Result Initialize(const Path &rhs) {
                /* Check the other path is normalized. */
                const bool normalized = rhs.IsNormalized();
                R_UNLESS(normalized, fs::ResultNotNormalized());

                /* Allocate buffer for our path. */
                const auto len = rhs.GetLength();
                R_TRY(this->Preallocate(len + 1));

                /* Copy the path. */
                const size_t copied = util::Strlcpy<char>(m_write_buffer.Get(), rhs.GetString(), len + 1);
                R_UNLESS(copied == len, fs::ResultUnexpectedInPathA());

                /* Set normalized. */
                this->SetNormalized();
                R_SUCCEED();
            }

            Result Initialize(const char *path, size_t len) {
                /* Check the path is valid. */
                R_UNLESS(path != nullptr, fs::ResultNullptrArgument());

                /* Initialize. */
                R_TRY(this->InitializeImpl(path, len));

                /* Set not normalized. */
                this->SetNotNormalized();

                R_SUCCEED();
            }

            Result Initialize(const char *path) {
                /* Check the path is valid. */
                R_UNLESS(path != nullptr, fs::ResultNullptrArgument());

                R_RETURN(this->Initialize(path, std::strlen(path)));
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
                const auto real_len = util::VSNPrintf(m_write_buffer.Get(), m_write_buffer.GetLength(), fmt, vl);
                AMS_ASSERT(real_len == len);
                AMS_UNUSED(real_len);

                /* Finish the va_list. */
                va_end(vl);

                /* Set not normalized. */
                this->SetNotNormalized();

                R_SUCCEED();
            }

            Result InitializeWithReplaceBackslash(const char *path) {
                /* Check the path is valid. */
                R_UNLESS(path != nullptr, fs::ResultNullptrArgument());

                /* Initialize. */
                R_TRY(this->InitializeImpl(path, std::strlen(path)));

                /* Replace slashes as desired. */
                if (const auto write_buffer_length = m_write_buffer.GetLength(); write_buffer_length > 1) {
                    fs::Replace(m_write_buffer.Get(), write_buffer_length - 1, '\\', '/');
                }

                /* Set not normalized. */
                this->SetNotNormalized();

                R_SUCCEED();
            }

            Result InitializeWithReplaceForwardSlashes(const char *path) {
                /* Check the path is valid. */
                R_UNLESS(path != nullptr, fs::ResultNullptrArgument());

                /* Initialize. */
                R_TRY(this->InitializeImpl(path, std::strlen(path)));

                /* Replace slashes as desired. */
                if (m_write_buffer.GetLength() > 1) {
                    if (auto *p = m_write_buffer.Get(); p[0] == '/' && p[1] == '/') {
                        p[0] = '\\';
                        p[1] = '\\';
                    }
                }

                /* Set not normalized. */
                this->SetNotNormalized();

                R_SUCCEED();
            }

            Result InitializeWithReplaceUnc(const char *path) {
                /* Check the path is valid. */
                R_UNLESS(path != nullptr, fs::ResultNullptrArgument());

                /* Initialize. */
                R_TRY(this->InitializeImpl(path, std::strlen(path)));

                /* Set not normalized. */
                this->SetNotNormalized();

                /* Replace unc as desired. */
                if (m_str[0]) {
                    auto *p = m_write_buffer.Get();

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
                this->SetNotNormalized();

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
                    bool normalized;
                    R_TRY(PathFormatter::IsNormalized(std::addressof(normalized), std::addressof(dummy), m_str));

                    this->SetNormalized();
                    R_SUCCEED();
                }

                /* Normalize. */
                R_TRY(this->Normalize(path_flags));

                this->SetNormalized();
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
                this->SetNormalized();

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
                if (m_write_buffer.Get() != nullptr) {
                    old_write_buffer = std::move(m_write_buffer);
                    this->ClearBuffer();
                }

                /* Pre-allocate the new buffer. */
                R_TRY(this->Preallocate(cur_len + 1 + child_len + 1));

                /* Get our write buffer. */
                auto *dst = m_write_buffer.Get();
                if (old_write_buffer.Get() != nullptr && cur_len > 0) {
                    util::Strlcpy<char>(dst, old_write_buffer.Get(), cur_len + 1);
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
                if (m_write_buffer.Get() == nullptr) {
                    if (const auto len = std::strlen(m_str); len > 0) {
                        R_TRY(this->Preallocate(len));
                        util::Strlcpy<char>(m_write_buffer.Get(), m_str, len + 1);
                    }
                }

                /* Check that it's possible for us to remove a child. */
                auto *p = m_write_buffer.Get();
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
                R_SUCCEED_IF(this->IsNormalized());

                /* Check if we're normalized. */
                bool normalized;
                size_t dummy;
                R_TRY(PathFormatter::IsNormalized(std::addressof(normalized), std::addressof(dummy), m_str, flags));

                /* If we're not normalized, normalize. */
                if (!normalized) {
                    /* Determine necessary buffer length. */
                    auto len = m_write_buffer.GetLength();
                    if (flags.IsRelativePathAllowed() && fs::IsPathRelative(m_str)) {
                        len += 2;
                    }
                    if (flags.IsWindowsPathAllowed() && fs::IsWindowsPath(m_str, true)) {
                        len += 1;
                    }

                    /* Allocate a new buffer. */
                    const size_t size = util::AlignUp(len, WriteBufferAlignmentLength);
                    auto buf = WriteBuffer::Make(size);
                    R_UNLESS(buf.Get() != nullptr, fs::ResultAllocationMemoryFailedMakeUnique());

                    /* Normalize into it. */
                    R_TRY(PathFormatter::Normalize(buf.Get(), size, m_write_buffer.Get(), m_write_buffer.GetLength(), flags));

                    /* Set the normalized buffer as our buffer. */
                    this->SetModifiableBuffer(std::move(buf));
                }

                /* Set normalized. */
                this->SetNormalized();
                R_SUCCEED();
            }
        private:
            void ClearBuffer() {
                m_write_buffer.ResetBuffer();
                m_str = EmptyPath;
            }

            void SetModifiableBuffer(WriteBuffer &&buffer) {
                /* Check pre-conditions. */
                AMS_ASSERT(buffer.Get() != nullptr);
                AMS_ASSERT(buffer.GetLength() > 0);
                AMS_ASSERT(util::IsAligned(buffer.GetLength(), WriteBufferAlignmentLength));

                /* Get whether we're normalized. */
                if (m_write_buffer.IsNormalized()) {
                    buffer.SetNormalized();
                } else {
                    buffer.SetNotNormalized();
                }

                /* Set write buffer. */
                m_write_buffer = std::move(buffer);
                m_str          = m_write_buffer.Get();
            }

            constexpr void SetReadOnlyBuffer(const char *buffer) {
                m_str = buffer;
                m_write_buffer.ResetBuffer();
            }

            Result Preallocate(size_t length) {
                /* Allocate additional space, if needed. */
                if (length > m_write_buffer.GetLength()) {
                    /* Allocate buffer. */
                    const size_t size = util::AlignUp(length, WriteBufferAlignmentLength);
                    auto buf = WriteBuffer::Make(size);
                    R_UNLESS(buf.Get() != nullptr, fs::ResultAllocationMemoryFailedMakeUnique());

                    /* Set write buffer. */
                    this->SetModifiableBuffer(std::move(buf));
                }

                R_SUCCEED();
            }

            Result InitializeImpl(const char *path, size_t size) {
                if (size > 0 && path[0]) {
                    /* Pre allocate a buffer for the path. */
                    R_TRY(this->Preallocate(size + 1));

                    /* Copy the path. */
                    const size_t copied = util::Strlcpy<char>(m_write_buffer.Get(), path, size + 1);
                    R_UNLESS(copied >= size, fs::ResultUnexpectedInPathA());
                } else {
                    /* We can just clear the buffer. */
                    this->ClearBuffer();
                }

                R_SUCCEED();
            }

            constexpr char *GetWriteBuffer() {
                AMS_ASSERT(m_write_buffer.Get() != nullptr);
                return m_write_buffer.Get();
            }

            constexpr ALWAYS_INLINE size_t GetWriteBufferLength() const {
                return m_write_buffer.GetLength();
            }

            constexpr ALWAYS_INLINE bool IsNormalized() const { return m_write_buffer.IsNormalized(); }

            constexpr ALWAYS_INLINE void SetNormalized() { m_write_buffer.SetNormalized(); }

            constexpr ALWAYS_INLINE void SetNotNormalized() { m_write_buffer.SetNotNormalized(); }
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

    constexpr inline bool IsWindowsDriveRootPath(const fs::Path &path) {
        const char * const str = path.GetString();
        return fs::IsWindowsDrive(str) && (str[2] == StringTraits::DirectorySeparator || str[2] == StringTraits::AlternateDirectorySeparator) && str[3] == StringTraits::NullTerminator;
    }

}
