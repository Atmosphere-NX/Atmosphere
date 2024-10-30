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
#if defined(ATMOSPHERE_OS_WINDOWS)
#include <stratosphere/windows.hpp>
#include <winerror.h>
#include <winioctl.h>
#elif defined(ATMOSPHERE_OS_LINUX) || defined(ATMOSPHERE_OS_MACOS)
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <dirent.h>
#endif

#if defined(ATMOSPHERE_OS_LINUX)
#include <sys/syscall.h>
#elif defined(ATMOSPHERE_OS_MACOS)
extern "C" ssize_t __getdirentries64(int fd, char *buffer, size_t buffer_size, uintptr_t *basep);
#endif

#if !defined(ATMOSPHERE_OS_HORIZON)
namespace ams::fssystem {

    namespace {

        constexpr s64 NanoSecondsPerWindowsTick  = 100;
        constexpr s64 WindowsTicksPerSecond      = TimeSpan::FromSeconds(1).GetNanoSeconds() / TimeSpan::FromNanoSeconds(NanoSecondsPerWindowsTick).GetNanoSeconds();
        constexpr s64 OffsetToConvertToPosixTime = 11644473600;

        [[maybe_unused]] constexpr ALWAYS_INLINE s64 ConvertWindowsTimeToPosixTime(s64 windows_ticks) {
            return (windows_ticks / WindowsTicksPerSecond) - OffsetToConvertToPosixTime;
        }

        [[maybe_unused]] constexpr ALWAYS_INLINE s64 ConvertPosixTimeToWindowsTime(s64 posix_sec, s64 posix_ns = 0) {
            return ((posix_sec + OffsetToConvertToPosixTime) * WindowsTicksPerSecond) + util::DivideUp<s64>(posix_ns, NanoSecondsPerWindowsTick);
        }

        #if defined(ATMOSPHERE_OS_WINDOWS)
        constexpr int MaxFilePathLength      = MAX_PATH - 1;
        constexpr int MaxDirectoryPathLength = MaxFilePathLength - (8 + 1 + 3);
        static_assert(MaxFilePathLength      == 259);
        static_assert(MaxDirectoryPathLength == 247);

        bool AreLongPathsEnabledImpl() {
            /* Get handle to ntdll. */
            const HMODULE module = ::GetModuleHandleW(L"ntdll");
            if (module == nullptr) {
                return true;
            }

            /* Get function pointer to long paths enabled. */
            const auto enabled_funcptr = ::GetProcAddress(module, "RtlAreLongPathsEnabled");
            if (enabled_funcptr == nullptr) {
                return true;
            }

            /* Get whether long paths are enabled. */
            using FunctionType = BOOLEAN (NTAPI *)();
            return reinterpret_cast<FunctionType>(reinterpret_cast<uintptr_t>(enabled_funcptr))();
        }

        Result ConvertLastErrorToResult() {
            switch (::GetLastError()) {
                case ERROR_FILE_NOT_FOUND:
                case ERROR_PATH_NOT_FOUND:
                case ERROR_NO_MORE_FILES:
                case ERROR_BAD_NETPATH:
                case ERROR_BAD_NET_NAME:
                case ERROR_DIRECTORY:
                case ERROR_BAD_DEVICE:
                case ERROR_CONNECTION_UNAVAIL:
                case ERROR_NO_NET_OR_BAD_PATH:
                case ERROR_NOT_CONNECTED:
                    R_THROW(fs::ResultPathNotFound());
                case ERROR_ACCESS_DENIED:
                case ERROR_SHARING_VIOLATION:
                    R_THROW(fs::ResultTargetLocked());
                case ERROR_HANDLE_EOF:
                    R_THROW(fs::ResultOutOfRange());
                case ERROR_FILE_EXISTS:
                case ERROR_ALREADY_EXISTS:
                    R_THROW(fs::ResultPathAlreadyExists());
                case ERROR_DISK_FULL:
                case ERROR_SPACES_NOT_ENOUGH_DRIVES:
                    R_THROW(fs::ResultNotEnoughFreeSpace());
                case ERROR_DIR_NOT_EMPTY:
                    R_THROW(fs::ResultDirectoryNotEmpty());
                case ERROR_BAD_PATHNAME:
                    R_THROW(fs::ResultInvalidPathFormat());
                case ERROR_FILENAME_EXCED_RANGE:
                    R_THROW(fs::ResultTooLongPath());
                default:
                    //printf("Returning ConvertLastErrorToResult() -> ResultUnexpectedInLocalFileSystemE, last_error=0x%08x\n", static_cast<u32>(::GetLastError()));
                    R_THROW(fs::ResultUnexpectedInLocalFileSystemE());
            }
        }

        Result WaitDeletionCompletion(const wchar_t *native_path) {
            /* Wait for the path to be deleted. */
            constexpr int MaxTryCount = 25;
            for (int i = 0; i < MaxTryCount; ++i) {
                /* Get the file attributes. */
                const auto attr = ::GetFileAttributesW(native_path);

                /* If they're not invalid, we're done. */
                R_SUCCEED_IF(attr != INVALID_FILE_ATTRIBUTES);

                /* Get last error. */
                const auto err = ::GetLastError();

                /* If error was file not found, the delete is complete. */
                R_SUCCEED_IF(err == ERROR_FILE_NOT_FOUND);

                /* If the error was access denied, we want to try again. */
                R_UNLESS(err == ERROR_ACCESS_DENIED, ConvertLastErrorToResult());

                /* Sleep before checking again. */
                ::Sleep(2);
            }

            /* We received access denied 25 times in a row. */
            R_THROW(fs::ResultTargetLocked());
        }

        Result GetEntryTypeImpl(fs::DirectoryEntryType *out, const wchar_t *native_path) {
            const auto res = ::GetFileAttributesW(native_path);
            if (res == INVALID_FILE_ATTRIBUTES) {
                switch (::GetLastError()) {
                    case ERROR_FILE_NOT_FOUND:
                    case ERROR_PATH_NOT_FOUND:
                    case ERROR_ACCESS_DENIED:
                    case ERROR_BAD_NETPATH:
                    case ERROR_BAD_NET_NAME:
                    case ERROR_BAD_DEVICE:
                    case ERROR_CONNECTION_UNAVAIL:
                    case ERROR_NO_NET_OR_BAD_PATH:
                    case ERROR_NOT_CONNECTED:
                        R_THROW(fs::ResultPathNotFound());
                    default:
                        //printf("Returning GetEntryTypeImpl() -> ResultUnexpectedInLocalFileSystemF, last_error=0x%08x\n", static_cast<u32>(::GetLastError()));
                        R_THROW(fs::ResultUnexpectedInLocalFileSystemF());
                }
            }

            *out = (res & FILE_ATTRIBUTE_DIRECTORY) ? fs::DirectoryEntryType_Directory : fs::DirectoryEntryType_File;
            R_SUCCEED();
        }

        Result SetFileSizeImpl(HANDLE handle, s64 size) {
            /* Seek to the desired size. */
            LARGE_INTEGER seek;
            seek.QuadPart = size;
            R_UNLESS(::SetFilePointerEx(handle, seek, nullptr, FILE_BEGIN) != 0, ConvertLastErrorToResult());

            /* Try to set the file size. */
            if (::SetEndOfFile(handle) == 0) {
                /* Check if the error resulted from too large size. */
                R_UNLESS(::GetLastError() == ERROR_INVALID_PARAMETER, ConvertLastErrorToResult());
                R_UNLESS(size <= INT64_C(0x00000FFFFFFF0000),         ConvertLastErrorToResult());

                /* The file size is too large. */
                R_THROW(fs::ResultTooLargeSize());
            }

            R_SUCCEED();
        }

        class LocalFile : public ::ams::fs::fsa::IFile, public ::ams::fs::impl::Newable {
            private:
                const HANDLE m_handle;
                const fs::OpenMode m_open_mode;
            public:
                LocalFile(HANDLE h, fs::OpenMode m) : m_handle(h), m_open_mode(m) { /* ... */ }

                virtual ~LocalFile() {
                    ::CloseHandle(m_handle);
                }
            public:
                virtual Result DoRead(size_t *out, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) override {
                    /* Check that read is possible. */
                    size_t dry_read_size;
                    R_TRY(this->DryRead(std::addressof(dry_read_size), offset, size, option, m_open_mode));

                    /* If we have nothing to read, we don't need to do anything. */
                    if (dry_read_size == 0) {
                        *out = 0;
                        R_SUCCEED();
                    }

                    /* Prepare to do asynchronous IO. */
                    OVERLAPPED overlapped = {};
                    overlapped.Offset     = static_cast<DWORD>(offset);
                    overlapped.OffsetHigh = static_cast<DWORD>(offset >> BITSIZEOF(DWORD));
                    overlapped.hEvent     = ::CreateEvent(nullptr, true, false, nullptr);
                    R_UNLESS(overlapped.hEvent != nullptr, fs::ResultUnexpectedInLocalFileSystemA());
                    ON_SCOPE_EXIT { ::CloseHandle(overlapped.hEvent); };

                    /* Read from the file. */
                    DWORD size_read;
                    if (!::ReadFile(m_handle, buffer, static_cast<DWORD>(size), std::addressof(size_read), std::addressof(overlapped))) {
                        /* If we fail for reason other than io pending, return the error result. */
                        const auto err = ::GetLastError();
                        R_UNLESS(err == ERROR_IO_PENDING, ConvertLastErrorToResult());

                        /* Get the wait result. */
                        if (!::GetOverlappedResult(m_handle, std::addressof(overlapped), std::addressof(size_read), true)) {
                            /* We failed...check if it's because we're at the end of the file. */
                            R_UNLESS(::GetLastError() == ERROR_HANDLE_EOF, ConvertLastErrorToResult());

                            /* Get the file size. */
                            LARGE_INTEGER file_size;
                            R_UNLESS(::GetFileSizeEx(m_handle, std::addressof(file_size)), ConvertLastErrorToResult());

                            /* Check the filesize matches offset. */
                            R_UNLESS(file_size.QuadPart == offset, ConvertLastErrorToResult());
                        }
                    }

                    /* Set the output read size. */
                    *out = size_read;
                    R_SUCCEED();
                }

                virtual Result DoGetSize(s64 *out) override {
                    /* Get the file size. */
                    LARGE_INTEGER size;
                    R_UNLESS(::GetFileSizeEx(m_handle, std::addressof(size)), fs::ResultUnexpectedInLocalFileSystemD());

                    /* Set the output. */
                    *out = size.QuadPart;
                    R_SUCCEED();
                }

                virtual Result DoFlush() override {
                    /* If we're not writable, we have nothing to flush. */
                    R_SUCCEED_IF((m_open_mode & fs::OpenMode_Write) == 0);

                    /* Flush our buffer. */
                    R_UNLESS(::FlushFileBuffers(m_handle), fs::ResultUnexpectedInLocalFileSystemC());
                    R_SUCCEED();
                }

                virtual Result DoWrite(s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) override {
                    /* Verify that we can write. */
                    bool needs_append;
                    R_TRY(this->DryWrite(std::addressof(needs_append), offset, size, option, m_open_mode));

                    /* If we need to, perform the write. */
                    if (size != 0) {
                        /* Prepare to do asynchronous IO. */
                        OVERLAPPED overlapped = {};
                        overlapped.Offset     = static_cast<DWORD>(offset);
                        overlapped.OffsetHigh = static_cast<DWORD>(offset >> BITSIZEOF(DWORD));
                        overlapped.hEvent     = ::CreateEvent(nullptr, true, false, nullptr);
                        R_UNLESS(overlapped.hEvent != nullptr, fs::ResultUnexpectedInLocalFileSystemA());
                        ON_SCOPE_EXIT { ::CloseHandle(overlapped.hEvent); };

                        /* Write to the file. */
                        DWORD size_written;
                        if (!::WriteFile(m_handle, buffer, static_cast<DWORD>(size), std::addressof(size_written), std::addressof(overlapped))) {
                            /* If we fail for reason other than io pending, return the error result. */
                            const auto err = ::GetLastError();
                            R_UNLESS(err == ERROR_IO_PENDING, ConvertLastErrorToResult());

                            /* Get the wait result. */
                            R_UNLESS(::GetOverlappedResult(m_handle, std::addressof(overlapped), std::addressof(size_written), true), ConvertLastErrorToResult());
                        }

                        /* Check that a correct amount of data was written. */
                        R_UNLESS(size_written >= size, fs::ResultNotEnoughFreeSpace());

                        /* Sanity check that we wrote the right amount. */
                        AMS_ASSERT(size_written == size);
                    }

                    /* If we need to, flush. */
                    if (option.HasFlushFlag()) {
                        R_TRY(this->Flush());
                    }

                    R_SUCCEED();
                }

                virtual Result DoSetSize(s64 size) override {
                    /* Verify we can set the size. */
                    R_TRY(this->DrySetSize(size, m_open_mode));

                    /* Try to set the file size. */
                    R_RETURN(SetFileSizeImpl(m_handle, size));
                }

                virtual Result DoOperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                    AMS_UNUSED(offset, size, src, src_size);
                    switch (op_id) {
                        case fs::OperationId::Invalidate:
                            R_SUCCEED();
                        case fs::OperationId::QueryRange:
                            R_UNLESS(dst != nullptr,                         fs::ResultNullptrArgument());
                            R_UNLESS(dst_size == sizeof(fs::QueryRangeInfo), fs::ResultInvalidSize());
                            static_cast<fs::QueryRangeInfo *>(dst)->Clear();
                            R_SUCCEED();
                        default:
                            R_THROW(fs::ResultUnsupportedOperateRangeForTmFileSystemFile());
                    }
                }
            public:
                 virtual sf::cmif::DomainObjectId GetDomainObjectId() const override {
                     AMS_ABORT("GetDomainObjectId() should never be called on a LocalFile");
                 }
        };

        bool IsDirectory(const WIN32_FIND_DATAW &fd) {
            return fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
        }

        class LocalDirectory : public ::ams::fs::fsa::IDirectory, public ::ams::fs::impl::Newable {
            private:
                std::unique_ptr<wchar_t[], ::ams::fs::impl::Deleter> m_path;
                HANDLE m_dir_handle;
                HANDLE m_search_handle;
                fs::OpenDirectoryMode m_open_mode;
            public:
                LocalDirectory(HANDLE d, fs::OpenDirectoryMode m, std::unique_ptr<wchar_t[], ::ams::fs::impl::Deleter> &&p) : m_path(std::move(p)), m_dir_handle(d), m_search_handle(INVALID_HANDLE_VALUE) {
                    m_open_mode = static_cast<fs::OpenDirectoryMode>(util::ToUnderlying(m) & ~util::ToUnderlying(fs::OpenDirectoryMode_NotRequireFileSize));
                }

                virtual ~LocalDirectory() {
                    if (m_search_handle != INVALID_HANDLE_VALUE) {
                        ::FindClose(m_search_handle);
                    }
                    ::CloseHandle(m_dir_handle);
                }
            public:
                virtual Result DoRead(s64 *out_count, fs::DirectoryEntry *out_entries, s64 max_entries) override {
                    auto read_count = 0;
                    while (read_count < max_entries) {
                        /* Read the next file. */
                        WIN32_FIND_DATAW fd;
                        std::memset(fd.cFileName, 0, sizeof(fd.cFileName));
                        if (m_search_handle == INVALID_HANDLE_VALUE) {
                            /* Create our search handle. */
                            if (m_search_handle = ::FindFirstFileW(m_path.get(), std::addressof(fd)); m_search_handle == INVALID_HANDLE_VALUE) {
                                /* Check that we failed because there are no files. */
                                R_UNLESS(::GetLastError() == ERROR_FILE_NOT_FOUND, ConvertLastErrorToResult());
                                break;
                            }
                        } else if (!::FindNextFileW(m_search_handle, std::addressof(fd))) {
                            /* Check that we failed because we ran out of files. */
                            R_UNLESS(::GetLastError() == ERROR_NO_MORE_FILES, ConvertLastErrorToResult());
                            break;
                        }

                        /* If we shouldn't create an entry, continue. */
                        if (!this->IsReadTarget(fd)) {
                            continue;
                        }

                        /* Create the entry. */
                        auto &entry = out_entries[read_count++];

                        std::memset(entry.name, 0, sizeof(entry.name));
                        const auto wide_res = ::WideCharToMultiByte(CP_UTF8, 0, fd.cFileName, -1, entry.name, sizeof(entry.name), nullptr, nullptr);
                        R_UNLESS(wide_res != 0, fs::ResultInvalidPath());

                        entry.type      = IsDirectory(fd) ? fs::DirectoryEntryType_Directory : fs::DirectoryEntryType_File;
                        entry.file_size = static_cast<s64>(fd.nFileSizeLow) | static_cast<s64>(static_cast<u64>(fd.nFileSizeHigh) << BITSIZEOF(fd.nFileSizeLow));
                    }

                    /* Set the output read count. */
                    *out_count = read_count;
                    R_SUCCEED();
                }

                virtual Result DoGetEntryCount(s64 *out) override {
                    /* Open a new search handle. */
                    WIN32_FIND_DATAW fd;
                    auto handle = ::FindFirstFileW(m_path.get(), std::addressof(fd));
                    R_UNLESS(handle != INVALID_HANDLE_VALUE, ConvertLastErrorToResult());
                    ON_SCOPE_EXIT { ::FindClose(handle); };

                    /* Iterate to get the total entry count. */
                    auto entry_count = 0;
                    while (::FindNextFileW(handle, std::addressof(fd))) {
                        if (this->IsReadTarget(fd)) {
                            ++entry_count;
                        }
                    }

                    /* Check that we stopped iterating because we ran out of files. */
                    R_UNLESS(::GetLastError() == ERROR_NO_MORE_FILES, ConvertLastErrorToResult());

                    /* Set the output. */
                    *out = entry_count;
                    R_SUCCEED();
                }
            private:
                bool IsReadTarget(const WIN32_FIND_DATAW &fd) const {
                    /* If the file is "..", don't return it. */
                    if (::wcsncmp(fd.cFileName, L"..", 3) == 0 || ::wcsncmp(fd.cFileName, L".", 2) == 0) {
                        return false;
                    }

                    /* Return whether our open mode supports the target. */
                    if (IsDirectory(fd)) {
                        return m_open_mode != fs::OpenDirectoryMode_File;
                    } else {
                        return m_open_mode != fs::OpenDirectoryMode_Directory;
                    }
                }
            public:
                 virtual sf::cmif::DomainObjectId GetDomainObjectId() const override {
                     AMS_ABORT("GetDomainObjectId() should never be called on a LocalDirectory");
                 }
        };

        #else
        constexpr int MaxFilePathLength      = PATH_MAX - 1;
        constexpr int MaxDirectoryPathLength = PATH_MAX - 1;

        #if defined (ATMOSPHERE_OS_LINUX)
        struct linux_dirent64 {
           ino64_t d_ino;
           off64_t d_off;
           unsigned short d_reclen;
           unsigned char  d_type;
           char d_name[];
        };

        using NativeDirectoryEntryType = struct linux_dirent64;
        #else
        using NativeDirectoryEntryType = struct dirent;
        #endif

        bool AreLongPathsEnabledImpl() {
            /* TODO: How does this work on linux/macos? */
            return true;
        }

        enum ErrnoSource {
            ErrnoSource_OpenFile,          // 0
            ErrnoSource_CreateFile,        // 1
            ErrnoSource_Unlink,            // 2
            ErrnoSource_Pread,             // 3
            ErrnoSource_Pwrite,            // 4
            ErrnoSource_Ftruncate,         // 5
                                           //
            ErrnoSource_OpenDirectory,     // 6
            ErrnoSource_Mkdir,             // 7
            ErrnoSource_Rmdir,             // 8
            ErrnoSource_GetDents,          // 9
                                           //
            ErrnoSource_RenameDirectory,   // 10
            ErrnoSource_RenameFile,        // 11
                                           //
            ErrnoSource_Stat,              // 12
            ErrnoSource_Statvfs,           // 13
        };

        Result ConvertErrnoToResult(ErrnoSource source) {
            switch (errno) {
                case ENOENT:
                    R_THROW(fs::ResultPathNotFound());
                case EEXIST:
                    switch (source) {
                        case ErrnoSource_Rmdir:
                            R_THROW(fs::ResultDirectoryNotEmpty());
                        default:
                            R_THROW(fs::ResultPathAlreadyExists());
                    }
                case ENOTDIR:
                    switch (source) {
                        case ErrnoSource_Rmdir:
                            R_THROW(fs::ResultPathNotFound());
                        default:
                            R_THROW(fs::ResultPathNotFound());
                    }
                case EISDIR:
                    switch (source) {
                        case ErrnoSource_CreateFile:
                            R_THROW(fs::ResultPathAlreadyExists());
                        case ErrnoSource_OpenFile:
                        case ErrnoSource_Unlink:
                            R_THROW(fs::ResultPathNotFound());
                        default:
                            R_THROW(fs::ResultUnexpectedInLocalFileSystemE());
                    }
                case ENOTEMPTY:
                    R_THROW(fs::ResultDirectoryNotEmpty());
                case EACCES:
                case EINTR:
                    R_THROW(fs::ResultTargetLocked());
                default:
                    //printf("Returning default errno -> result, errno=%d, source=%d\n", errno, static_cast<int>(source));
                    R_THROW(fs::ResultUnexpectedInLocalFileSystemE());
            }
        }

        Result WaitDeletionCompletion(const char *native_path) {
            /* TODO: Does linux need to wait for delete to complete? */
            AMS_UNUSED(native_path);
            R_SUCCEED();
        }


        Result GetEntryTypeImpl(fs::DirectoryEntryType *out, const char *native_path) {
            struct stat st;
            R_UNLESS(::stat(native_path, std::addressof(st)) == 0, ConvertErrnoToResult(ErrnoSource_Stat));

            *out = (S_ISDIR(st.st_mode)) ? fs::DirectoryEntryType_Directory : fs::DirectoryEntryType_File;
            R_SUCCEED();
        }

        auto RetryForEIntr(auto f) {
            decltype(f()) res;
            do {
                res = f();
            } while (res < 0 && errno == EINTR);
            return res;
        };

        void CloseFileDescriptor(int handle) {
            const int res = RetryForEIntr([&] () ALWAYS_INLINE_LAMBDA {
                return ::close(handle);
            });
            AMS_ASSERT(res == 0);
            AMS_UNUSED(res);
        }

        Result SetFileSizeImpl(int handle, s64 size) {
            const auto res = RetryForEIntr([&] () ALWAYS_INLINE_LAMBDA { return ::ftruncate(handle, size); });
            R_UNLESS(res == 0, ConvertErrnoToResult(ErrnoSource_Ftruncate));
            R_SUCCEED();
        }

        class LocalFile : public ::ams::fs::fsa::IFile, public ::ams::fs::impl::Newable {
            private:
                const int m_handle;
                const fs::OpenMode m_open_mode;
            public:
                LocalFile(int h, fs::OpenMode m) : m_handle(h), m_open_mode(m) { /* ... */ }

                virtual ~LocalFile() {
                    CloseFileDescriptor(m_handle);
                }
            public:
                virtual Result DoRead(size_t *out, s64 offset, void *buffer, size_t size, const fs::ReadOption &option) override {
                    /* Check that read is possible. */
                    size_t dry_read_size;
                    R_TRY(this->DryRead(std::addressof(dry_read_size), offset, size, option, m_open_mode));

                    /* If we have nothing to read, we don't need to do anything. */
                    if (dry_read_size == 0) {
                        *out = 0;
                        R_SUCCEED();
                    }

                    /* Read. */
                    const auto read_size = RetryForEIntr([&] () ALWAYS_INLINE_LAMBDA -> ssize_t { return ::pread(m_handle, buffer, size, offset); });
                    R_UNLESS(read_size >= 0, ConvertErrnoToResult(ErrnoSource_Pread));

                    /* Set output. */
                    *out = static_cast<size_t>(read_size);
                    R_SUCCEED();
                }

                virtual Result DoGetSize(s64 *out) override {
                    /* Get the file size. */
                    const auto size = RetryForEIntr([&] () ALWAYS_INLINE_LAMBDA -> s64 { return ::lseek(m_handle, 0, SEEK_END); });
                    R_UNLESS(size >= 0, fs::ResultUnexpectedInLocalFileSystemD());

                    /* Set the output. */
                    *out = size;
                    R_SUCCEED();
                }

                virtual Result DoFlush() override {
                    /* If we're not writable, we have nothing to flush. */
                    R_SUCCEED_IF((m_open_mode & fs::OpenMode_Write) == 0);

                    /* Flush our buffer. */
                    const auto res = RetryForEIntr([&] () ALWAYS_INLINE_LAMBDA { return ::fsync(m_handle); });
                    R_UNLESS(res == 0, fs::ResultUnexpectedInLocalFileSystemC());
                    R_SUCCEED();
                }

                virtual Result DoWrite(s64 offset, const void *buffer, size_t size, const fs::WriteOption &option) override {
                    /* Verify that we can write. */
                    bool needs_append;
                    R_TRY(this->DryWrite(std::addressof(needs_append), offset, size, option, m_open_mode));

                    /* If we need to, perform the write. */
                    if (size != 0) {
                        /* Read. */
                        const auto size_written = RetryForEIntr([&] () ALWAYS_INLINE_LAMBDA -> ssize_t { return ::pwrite(m_handle, buffer, size, offset); });
                        R_UNLESS(size_written >= 0, ConvertErrnoToResult(ErrnoSource_Pwrite));

                        /* Check that a correct amount of data was written. */
                        R_UNLESS(static_cast<size_t>(size_written) >= size, fs::ResultNotEnoughFreeSpace());

                        /* Sanity check that we wrote the right amount. */
                        AMS_ASSERT(static_cast<size_t>(size_written) == size);
                    }

                    /* If we need to, flush. */
                    if (option.HasFlushFlag()) {
                        R_TRY(this->Flush());
                    }

                    R_SUCCEED();
                }

                virtual Result DoSetSize(s64 size) override {
                    /* Verify we can set the size. */
                    R_TRY(this->DrySetSize(size, m_open_mode));

                    /* Try to set the file size. */
                    R_RETURN(SetFileSizeImpl(m_handle, size));
                }

                virtual Result DoOperateRange(void *dst, size_t dst_size, fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                    AMS_UNUSED(offset, size, src, src_size);
                    switch (op_id) {
                        case fs::OperationId::Invalidate:
                            R_SUCCEED();
                        case fs::OperationId::QueryRange:
                            R_UNLESS(dst != nullptr,                         fs::ResultNullptrArgument());
                            R_UNLESS(dst_size == sizeof(fs::QueryRangeInfo), fs::ResultInvalidSize());
                            static_cast<fs::QueryRangeInfo *>(dst)->Clear();
                            R_SUCCEED();
                        default:
                            R_THROW(fs::ResultUnsupportedOperateRangeForTmFileSystemFile());
                    }
                }
            public:
                 virtual sf::cmif::DomainObjectId GetDomainObjectId() const override {
                     AMS_ABORT("GetDomainObjectId() should never be called on a LocalFile");
                 }
        };

        class LocalDirectory : public ::ams::fs::fsa::IDirectory, public ::ams::fs::impl::Newable {
            private:
                std::unique_ptr<char[], ::ams::fs::impl::Deleter> m_path;
                int m_dir_handle;
                fs::OpenDirectoryMode m_open_mode;
                bool m_not_require_file_size;

                std::unique_ptr<fs::DirectoryEntry[], ::ams::fs::impl::Deleter> m_temp_entries;
                int m_temp_entries_count;
                int m_temp_entries_ofs;

                #if defined(ATMOSPHERE_OS_MACOS)
                uintptr_t m_basep = 0;
                #endif
            public:
                LocalDirectory(int d, fs::OpenDirectoryMode m, std::unique_ptr<char[], ::ams::fs::impl::Deleter> &&p) : m_path(std::move(p)), m_dir_handle(d), m_temp_entries(nullptr), m_temp_entries_count(0), m_temp_entries_ofs(0) {
                    m_open_mode             = static_cast<fs::OpenDirectoryMode>(util::ToUnderlying(m) & ~util::ToUnderlying(fs::OpenDirectoryMode_NotRequireFileSize));
                    m_not_require_file_size = m & fs::OpenDirectoryMode_NotRequireFileSize;
                }

                virtual ~LocalDirectory() {
                    CloseFileDescriptor(m_dir_handle);
                }
            public:
                virtual Result DoRead(s64 *out_count, fs::DirectoryEntry *out_entries, s64 max_entries) override {
                    auto read_count = 0;

                    /* Copy out any pending entries from a previous call. */
                    while (m_temp_entries_ofs < m_temp_entries_count && read_count < max_entries) {
                        out_entries[read_count++] = m_temp_entries[m_temp_entries_ofs++];
                    }

                    /* If we're done with our temporary entries, release them. */
                    if (m_temp_entries_ofs == m_temp_entries_count) {
                        m_temp_entries.reset();
                        m_temp_entries_ofs   = 0;
                        m_temp_entries_count = 0;
                    }

                    if (read_count < max_entries) {
                        /* Declare buffer to hold temporary path. */
                        char path_buf[PATH_MAX];
                        auto base_path_len = std::strlen(m_path.get());
                        std::memcpy(path_buf, m_path.get(), base_path_len);
                        if (path_buf[base_path_len - 1] != '/') {
                            path_buf[base_path_len++] = '/';
                        }

                        #if defined(ATMOSPHERE_OS_LINUX)
                        char buf[1_KB];
                        #else
                        char buf[2_KB];
                        #endif
                        NativeDirectoryEntryType *ent = nullptr;
                        while (read_count < max_entries) {
                            /* Read next entries. */
                            #if defined (ATMOSPHERE_OS_LINUX)
                            const auto nread = ::syscall(SYS_getdents64, m_dir_handle, buf, sizeof(buf));
                            #elif defined(ATMOSPHERE_OS_MACOS)
                            const auto nread = ::__getdirentries64(m_dir_handle, buf, sizeof(buf), std::addressof(m_basep));
                            #else
                            #error "Unknown OS to read from directory FD"
                            #endif
                            R_UNLESS(nread >= 0, ConvertErrnoToResult(ErrnoSource_GetDents));

                            /* If we read nothing, we've hit the end of the directory. */
                            if (nread == 0) {
                                break;
                            }

                            /* Determine the number of entries we read. */
                            auto cur_read_entries = 0;
                            for (auto pos = 0; pos < nread; pos += ent->d_reclen) {
                                /* Get the native entry. */
                                ent = reinterpret_cast<NativeDirectoryEntryType *>(buf + pos);

                                /* If the entry isn't a read target, ignore it. */
                                if (IsReadTarget(ent)) {
                                    ++cur_read_entries;
                                }
                            }

                            /* If we'll end up reading more than we can fit, allocate a temporary buffer. */
                            if (read_count + cur_read_entries > max_entries) {
                                /* Allocate temporary entries. */
                                m_temp_entries_count = (read_count + cur_read_entries) - max_entries;
                                m_temp_entries_ofs   = 0;

                                /* TODO: Non-fatal? */
                                m_temp_entries = fs::impl::MakeUnique<fs::DirectoryEntry[]>(m_temp_entries_count);
                                AMS_ABORT_UNLESS(m_temp_entries != nullptr);
                            }

                            /* Iterate received entries. */
                            for (auto pos = 0; pos < nread; pos += ent->d_reclen) {
                                /* Get the native entry. */
                                ent = reinterpret_cast<NativeDirectoryEntryType *>(buf + pos);

                                /* If the entry isn't a read target, ignore it. */
                                if (!IsReadTarget(ent)) {
                                    continue;
                                }

                                /* Decide on the output entry. */
                                fs::DirectoryEntry *out_entry;
                                if (read_count < max_entries) {
                                    out_entry = std::addressof(out_entries[read_count++]);
                                } else {
                                    out_entry = std::addressof(m_temp_entries[m_temp_entries_ofs++]);
                                }

                                /* Setup the output entry. */
                                {
                                    std::memset(out_entry->name, 0, sizeof(out_entry->name));

                                    const auto name_len = std::strlen(ent->d_name);
                                    AMS_ABORT_UNLESS(name_len <= fs::EntryNameLengthMax);

                                    std::memcpy(out_entry->name, ent->d_name, name_len + 1);

                                    out_entry->type = (ent->d_type == DT_DIR) ? fs::DirectoryEntryType_Directory : fs::DirectoryEntryType_File;

                                    /* If we have to, get the filesize. This is (unfortunately) expensive on linux. */
                                    if (out_entry->type == fs::DirectoryEntryType_File && !m_not_require_file_size) {
                                        /* Set up the temporary file path. */
                                        AMS_ABORT_UNLESS(base_path_len + name_len + 1 <= PATH_MAX);
                                        std::memcpy(path_buf + base_path_len, ent->d_name, name_len + 1);

                                        /* Get the file stats. */
                                        struct stat st;
                                        R_UNLESS(::stat(path_buf, std::addressof(st)) == 0, ConvertErrnoToResult(ErrnoSource_Stat));

                                        out_entry->file_size = static_cast<s64>(st.st_size);
                                    }
                                }
                            }

                            /* Ensure our temporary entries are correct. */
                            if (m_temp_entries != nullptr) {
                                AMS_ASSERT(read_count == max_entries);
                                AMS_ASSERT(m_temp_entries_ofs == m_temp_entries_count);
                                m_temp_entries_ofs = 0;
                            }
                        }
                    }

                    /* Set the output read count. */
                    *out_count = read_count;
                    R_SUCCEED();
                }

                virtual Result DoGetEntryCount(s64 *out) override {
                    /* Open the directory anew. */
                    const auto handle = RetryForEIntr([&] () ALWAYS_INLINE_LAMBDA { return ::open(m_path.get(), O_RDONLY | O_DIRECTORY); });
                    R_UNLESS(handle >= 0, ConvertErrnoToResult(ErrnoSource_OpenDirectory));
                    ON_SCOPE_EXIT { CloseFileDescriptor(handle); };

                    /* Iterate to get the total entry count. */
                    auto entry_count = 0;
                    {
                        #if defined(ATMOSPHERE_OS_LINUX)
                        char buf[1_KB];
                        #else
                        char buf[2_KB];
                        uintptr_t basep = 0;
                        #endif

                        NativeDirectoryEntryType *ent = nullptr;
                        while (true) {
                            /* Read next entries. */
                            #if defined (ATMOSPHERE_OS_LINUX)
                            const auto nread = ::syscall(SYS_getdents64, handle, buf, sizeof(buf));
                            #elif defined(ATMOSPHERE_OS_MACOS)
                            const auto nread = ::__getdirentries64(handle, buf, sizeof(buf), std::addressof(basep));
                            #else
                            #error "Unknown OS to read from directory FD"
                            #endif

                            R_UNLESS(nread >= 0, ConvertErrnoToResult(ErrnoSource_GetDents));

                            /* If we read nothing, we've hit the end of the directory. */
                            if (nread == 0) {
                                break;
                            }

                            /* Iterate received entries. */
                            for (auto pos = 0; pos < nread; pos += ent->d_reclen) {
                                /* Get the entry. */
                                ent = reinterpret_cast<NativeDirectoryEntryType *>(buf + pos);

                                /* If the entry is a read target, increment our count. */
                                if (IsReadTarget(ent)) {
                                    ++entry_count;
                                }
                            }
                        }
                    }

                    *out = entry_count;
                    R_SUCCEED();
                }
            private:
                bool IsReadTarget(const NativeDirectoryEntryType *ent) const {
                    /* If the file is "..", don't return it. */
                    if (std::strcmp(ent->d_name, ".") == 0 || std::strcmp(ent->d_name, "..") == 0) {
                        return false;
                    }

                    /* Return whether our open mode supports the target. */
                    if (ent->d_type == DT_DIR) {
                        return m_open_mode != fs::OpenDirectoryMode_File;
                    } else {
                        return m_open_mode != fs::OpenDirectoryMode_Directory;
                    }
                }
            public:
                 virtual sf::cmif::DomainObjectId GetDomainObjectId() const override {
                     AMS_ABORT("GetDomainObjectId() should never be called on a LocalDirectory");
                 }
        };

        #endif

        bool AreLongPathsEnabled() {
            AMS_FUNCTION_LOCAL_STATIC(bool, s_enabled, AreLongPathsEnabledImpl());
            return s_enabled;
        }

    }

    Result LocalFileSystem::Initialize(const fs::Path &root_path, fssystem::PathCaseSensitiveMode case_sensitive_mode) {
        /* Initialize our root path. */
        R_TRY(m_root_path.Initialize(root_path));

        /* If we're not empty, we'll need to convert to a native path. */
        if (m_root_path.IsEmpty()) {
            /* Reset our native path, since we're acting without a root. */
            m_native_path_buffer.reset(nullptr);
            m_native_path_length = 0;
        } else {
            /* Convert to native path. */
            NativePathBuffer native_path = nullptr;
            int native_len = 0;
            #if defined(ATMOSPHERE_OS_WINDOWS)
            {
                /* Get path length. */
                native_len = ::MultiByteToWideChar(CP_UTF8, 0, m_root_path.GetString(), -1, nullptr, 0);

                /* Allocate our native path buffer. */
                native_path = fs::impl::MakeUnique<NativeCharacterType[]>(native_len + 1);
                R_UNLESS(native_path != nullptr, fs::ResultAllocationMemoryFailedMakeUnique());

                /* Convert path. */
                const auto res = ::MultiByteToWideChar(CP_UTF8, 0, m_root_path.GetString(), -1, native_path.get(), native_len);
                R_UNLESS(res != 0,                                            fs::ResultTooLongPath());
                R_UNLESS(res <= static_cast<int>(fs::EntryNameLengthMax + 1), fs::ResultTooLongPath());

                /* Fix up directory separators. */
                for (NativeCharacterType *p = native_path.get(); *p != 0; ++p) {
                    if (*p == '/') {
                        *p = '\\';
                    }
                }
            }
            #else
            {
                /* Get path size. */
                native_len = std::strlen(m_root_path.GetString());

                /* Tentatively assume other operating systems do the sane thing and use utf-8 strings. */
                native_path = fs::impl::MakeUnique<NativeCharacterType[]>(native_len + 1);
                R_UNLESS(native_path != nullptr, fs::ResultAllocationMemoryFailedMakeUnique());

                /* Copy in path. */
                std::memcpy(native_path.get(), m_root_path.GetString(), native_len + 1);
            }
            #endif

            /* Temporarily set case sensitive mode to insensitive, and verify we can get the root directory. */
            m_case_sensitive_mode = fssystem::PathCaseSensitiveMode_CaseInsensitive;
            {
                constexpr fs::Path RequiredRootPath = fs::MakeConstantPath("/");

                fs::DirectoryEntryType type;
                R_TRY(this->GetEntryType(std::addressof(type), RequiredRootPath));

                R_UNLESS(type == fs::DirectoryEntryType_Directory, fs::ResultPathNotFound());
            }

            /* Set our native path members. */
            m_native_path_buffer = std::move(native_path);
            m_native_path_length = native_len;
        }

        /* Set our case sensitive mode. */
        m_case_sensitive_mode = case_sensitive_mode;
        R_SUCCEED();
    }

    Result LocalFileSystem::GetCaseSensitivePath(int *out_size, char *dst, size_t dst_size, const char *path, const char *work_path) {
        AMS_UNUSED(out_size, dst, dst_size, path, work_path);
        AMS_ABORT("TODO");
    }

    Result LocalFileSystem::CheckPathCaseSensitively(const NativeCharacterType *path, const NativeCharacterType *root_path, NativeCharacterType *cs_buf, size_t cs_size, bool check_case_sensitivity) {
        AMS_UNUSED(path, root_path, cs_buf, cs_size, check_case_sensitivity);
        AMS_ABORT("TODO");
    }

    Result LocalFileSystem::ResolveFullPath(NativePathBuffer *out, const fs::Path &path, int max_len, int min_len, bool check_case_sensitivity) {
        /* Create the full path. */
        fs::Path full_path;
        R_TRY(full_path.Combine(m_root_path, path));

        /* Check that the path is valid. */
        fs::PathFlags flags;
        flags.AllowWindowsPath();
        flags.AllowRelativePath();
        flags.AllowEmptyPath();
        R_TRY(fs::PathFormatter::CheckPathFormat(full_path.GetString(), flags));

        /* Check the path's character count. */
        #if defined(ATMOSPHERE_OS_WINDOWS)
        AreLongPathsEnabled();
        // TODO: R_TRY(fs::CheckCharacterCountForWindows(full_path.GetString(), MaxBasePathLength, AreLongPathsEnabled() ? 0 : max_len));
        AMS_UNUSED(max_len);
        #else
        AreLongPathsEnabled();
        /* TODO: Check character count for linux/macos? */
        AMS_UNUSED(max_len);
        #endif


        /* Convert to native path. */
        NativePathBuffer native_path = nullptr;
        #if defined(ATMOSPHERE_OS_WINDOWS)
        {
            /* Get path length. */
            const int native_len = ::MultiByteToWideChar(CP_UTF8, 0, full_path.GetString(), -1, nullptr, 0);

            /* Allocate our native path buffer. */
            native_path = fs::impl::MakeUnique<NativeCharacterType[]>(native_len + min_len + 1);
            R_UNLESS(native_path != nullptr, fs::ResultAllocationMemoryFailedMakeUnique());

            /* Convert path. */
            const auto res = ::MultiByteToWideChar(CP_UTF8, 0, full_path.GetString(), -1, native_path.get(), native_len);
            R_UNLESS(res != 0,          fs::ResultTooLongPath());
            R_UNLESS(res <= native_len, fs::ResultTooLongPath());

            /* Fix up directory separators. */
            s32 len = 0;
            for (NativeCharacterType *p = native_path.get(); *p != 0; ++p) {
                if (*p == '/') {
                    *p = '\\';
                }
                ++len;
            }

            /* Fix up trailing : */
            if (native_path[len - 1] == ':') {
                native_path[len]     = '\\';
                native_path[len + 1] = 0;
            }

            /* If case sensitivity is required, allocate case sensitive buffer. */
            if (m_case_sensitive_mode == PathCaseSensitiveMode_CaseSensitive && native_path[0] != 0) {
                /* Allocate case sensitive buffer. */
                auto case_sensitive_buffer_size = sizeof(NativeCharacterType) * (m_native_path_length + native_len + 1 + fs::EntryNameLengthMax);
                NativePathBuffer case_sensitive_path_buffer = fs::impl::MakeUnique<NativeCharacterType[]>(case_sensitive_buffer_size / sizeof(NativeCharacterType));
                R_UNLESS(case_sensitive_path_buffer != nullptr, fs::ResultAllocationMemoryFailedMakeUnique());

                /* Get root path. */
                const NativeCharacterType *root_path = m_native_path_buffer.get() != nullptr ? m_native_path_buffer.get() : L"";

                /* Perform case sensitive path checking. */
                R_TRY(this->CheckPathCaseSensitively(native_path.get(), root_path, case_sensitive_path_buffer.get(), case_sensitive_buffer_size, check_case_sensitivity));
            }

            /* Set default path, if empty. */
            if (native_path[0] == 0) {
                native_path[0] = '.';
                native_path[1] = '\\';
                native_path[2] = 0;
            }
        }
        #else
        {
            /* Get path size. */
            const int native_len = std::strlen(full_path.GetString());

            /* Tentatively assume other operating systems do the sane thing and use utf-8 strings. */
            native_path = fs::impl::MakeUnique<NativeCharacterType[]>(native_len + min_len + 1);
            R_UNLESS(native_path != nullptr, fs::ResultAllocationMemoryFailedMakeUnique());

            /* Copy in path. */
            std::memcpy(native_path.get(), full_path.GetString(), native_len + 1);

            /* TODO: Is case sensitivity adjustment needed here? */
            AMS_UNUSED(check_case_sensitivity);
        }
        #endif

        /* Set the output path. */
        *out = std::move(native_path);
        R_SUCCEED();
    }

    Result LocalFileSystem::DoGetDiskFreeSpace(s64 *out_free, s64 *out_total, s64 *out_total_free, const fs::Path &path) {
        /* Resolve the path. */
        NativePathBuffer native_path;
        R_TRY(this->ResolveFullPath(std::addressof(native_path), path, MaxFilePathLength, 0, false));

        /* Get the disk free space. */
        #if defined(ATMOSPHERE_OS_WINDOWS)
        {
            ULARGE_INTEGER free, total, total_free;
            R_UNLESS(::GetDiskFreeSpaceExW(native_path.get(), std::addressof(free), std::addressof(total), std::addressof(total_free)), ConvertLastErrorToResult());

            *out_free       = static_cast<s64>(free.QuadPart);
            *out_total      = static_cast<s64>(total.QuadPart);
            *out_total_free = static_cast<s64>(total_free.QuadPart);
        }
        #else
        {
            struct statvfs st;
            const auto res = RetryForEIntr([&] () ALWAYS_INLINE_LAMBDA { return ::statvfs(native_path.get(), std::addressof(st)); });
            R_UNLESS(res >= 0, ConvertErrnoToResult(ErrnoSource_Statvfs));

            *out_free       = static_cast<s64>(st.f_bavail) * static_cast<s64>(st.f_frsize);
            *out_total      = static_cast<s64>(st.f_blocks) * static_cast<s64>(st.f_frsize);
            *out_total_free = static_cast<s64>(st.f_bfree)  * static_cast<s64>(st.f_frsize);
        }
        #endif

        R_SUCCEED();
    }

    Result LocalFileSystem::DeleteDirectoryRecursivelyInternal(const NativeCharacterType *path, bool delete_top) {
        #if defined(ATMOSPHERE_OS_WINDOWS)
        {
            /* Get the path length. */
            const auto path_len = ::wcslen(path);

            /* Allocate a new path buffer. */
            NativePathBuffer cur_path_buf = fs::impl::MakeUnique<NativeCharacterType[]>(path_len + MAX_PATH);
            R_UNLESS(cur_path_buf.get() != nullptr, fs::ResultAllocationMemoryFailedMakeUnique());

            /* Copy the path into the temporary buffer. */
            ::wcscpy(cur_path_buf.get(), path);
            ::wcscat(cur_path_buf.get(), L"\\*");

            /* Iterate the directory, deleting all contents. */
            {
                /* Begin finding. */
                WIN32_FIND_DATAW fd;
                const auto handle = ::FindFirstFileW(cur_path_buf.get(), std::addressof(fd));
                R_UNLESS(handle != INVALID_HANDLE_VALUE, ConvertLastErrorToResult());
                ON_SCOPE_EXIT { ::FindClose(handle); };

                /* Clear the path from <path>\\* to path\\ */
                wchar_t * const dst = cur_path_buf.get() + path_len + 1;
                *dst = 0;

                /* Loop files. */
                while (::FindNextFileW(handle, std::addressof(fd))) {
                    /* Skip . and .. */
                    if (::wcsncmp(fd.cFileName, L"..", 3) == 0 || ::wcsncmp(fd.cFileName, L".", 2) == 0) {
                        continue;
                    }

                    /* Copy the current filename to our working path. */
                    ::wcscpy(dst, fd.cFileName);

                    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        /* If a directory, delete it recursively. */
                        R_TRY(this->DeleteDirectoryRecursivelyInternal(cur_path_buf.get(), true));
                    } else {
                        /* If a file, just delete it. */
                        auto delete_file = [&]() -> Result {
                            R_UNLESS(::DeleteFileW(cur_path_buf.get()), ConvertLastErrorToResult());
                            R_SUCCEED();
                        };

                        R_TRY(fssystem::RetryToAvoidTargetLocked(delete_file));
                        R_TRY(WaitDeletionCompletion(cur_path_buf.get()));
                    }
                }

                /* Check that we stopped iterating because we ran out of files. */
                R_UNLESS(::GetLastError() == ERROR_NO_MORE_FILES, ConvertLastErrorToResult());
            }

            /* If we should, delete the top level directory. */
            if (delete_top) {
                auto delete_impl = [&] () -> Result {
                    R_UNLESS(::RemoveDirectoryW(path), ConvertLastErrorToResult());
                    R_SUCCEED();
                };

                /* Perform the delete. */
                R_TRY(fssystem::RetryToAvoidTargetLocked(delete_impl));

                /* Wait for the deletion to complete. */
                R_TRY(WaitDeletionCompletion(path));
            }
        }
        #else
        {
            /* Get the path length. */
            const auto path_len = std::strlen(path);

            /* Allocate a temporary buffer. */
            NativePathBuffer cur_path_buf = fs::impl::MakeUnique<NativeCharacterType[]>(path_len + PATH_MAX);
            R_UNLESS(cur_path_buf.get() != nullptr, fs::ResultAllocationMemoryFailedMakeUnique());

            /* Copy the path into the temporary buffer. */
            std::memcpy(cur_path_buf.get(), path, path_len);
            auto ofs = path_len;
            if (cur_path_buf.get()[ofs - 1] != '/') {
                cur_path_buf.get()[ofs++] = '/';
            }

            /* Iterate the directory, deleting all contents. */
            {
                /* Open the directory. */
                const auto handle = RetryForEIntr([&] () ALWAYS_INLINE_LAMBDA { return ::open(path, O_RDONLY | O_DIRECTORY); });
                R_UNLESS(handle >= 0, ConvertErrnoToResult(ErrnoSource_OpenDirectory));
                ON_SCOPE_EXIT { CloseFileDescriptor(handle); };

                #if defined(ATMOSPHERE_OS_LINUX)
                char buf[1_KB];
                #else
                char buf[2_KB];
                uintptr_t basep = 0;
                #endif

                NativeDirectoryEntryType *ent = nullptr;
                static_assert(sizeof(*ent) <= sizeof(buf));
                while (true) {
                    /* Read next entries. */
                    #if defined (ATMOSPHERE_OS_LINUX)
                    const auto nread = ::syscall(SYS_getdents64, handle, buf, sizeof(buf));
                    #elif defined(ATMOSPHERE_OS_MACOS)
                    const auto nread = ::__getdirentries64(handle, buf, sizeof(buf), std::addressof(basep));
                    #else
                    #error "Unknown OS to read from directory FD"
                    #endif
                    R_UNLESS(nread >= 0, ConvertErrnoToResult(ErrnoSource_GetDents));

                    /* If we read nothing, we've hit the end of the directory. */
                    if (nread == 0) {
                        break;
                    }

                    /* Iterate received entries. */
                    for (auto pos = 0; pos < nread; pos += ent->d_reclen) {
                        /* Get the entry. */
                        ent = reinterpret_cast<NativeDirectoryEntryType *>(buf + pos);

                        /* Skip . and .. */
                        if (std::strcmp(ent->d_name, ".") == 0 || std::strcmp(ent->d_name, "..") == 0) {
                            continue;
                        }

                        /* Get the entry name length. */
                        const int e_len = std::strlen(ent->d_name);
                        std::memcpy(cur_path_buf.get() + ofs, ent->d_name, e_len + 1);

                        /* Get the dir type. */
                        const auto d_type = ent->d_type;

                        if (d_type == DT_DIR) {
                            /* If a directory, recursively delete it. */
                            R_TRY(this->DeleteDirectoryRecursivelyInternal(cur_path_buf.get(), true));
                        } else {
                            /* If a file, just delete it. */
                            auto delete_file = [&]() -> Result {
                                const auto res = ::unlink(cur_path_buf.get());
                                R_UNLESS(res >= 0, ConvertErrnoToResult(ErrnoSource_Unlink));
                                R_SUCCEED();
                            };

                            R_TRY(fssystem::RetryToAvoidTargetLocked(delete_file));
                            R_TRY(WaitDeletionCompletion(cur_path_buf.get()));
                        }
                    }
                }
            }

            /* If we should, delete the top level directory. */
            if (delete_top) {
                auto delete_impl = [&] () -> Result {
                    R_UNLESS(::rmdir(path) == 0, ConvertErrnoToResult(ErrnoSource_Rmdir));
                    R_SUCCEED();
                };

                /* Perform the delete. */
                R_TRY(fssystem::RetryToAvoidTargetLocked(delete_impl));

                /* Wait for the deletion to complete. */
                R_TRY(WaitDeletionCompletion(path));
            }
        }
        #endif

        R_SUCCEED();
    }

    Result LocalFileSystem::DoCreateFile(const fs::Path &path, s64 size, int flags) {
        AMS_UNUSED(flags);

        /* Resolve the path. */
        NativePathBuffer native_path;
        R_TRY(this->ResolveFullPath(std::addressof(native_path), path, MaxFilePathLength, 0, false));

        /* Create the file. */
        #if defined(ATMOSPHERE_OS_WINDOWS)
        {
            /* Get handle to created file. */
            const auto handle = ::CreateFileW(native_path.get(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (handle == INVALID_HANDLE_VALUE) {
                /* If we failed because of target locked, it may be the case that the path already exists as a directory. */
                R_TRY_CATCH(ConvertLastErrorToResult()) {
                    R_CATCH(fs::ResultTargetLocked) {
                        /* Get the file attributes. */
                        const auto attr = ::GetFileAttributesW(native_path.get());

                        /* Check they're valid. */
                        R_UNLESS(attr != INVALID_FILE_ATTRIBUTES, R_CURRENT_RESULT);

                        /* Check that they specify a directory. */
                        R_UNLESS((attr & FILE_ATTRIBUTE_DIRECTORY) != 0, R_CURRENT_RESULT);

                        /* The path is an existing directory. */
                        R_THROW(fs::ResultPathAlreadyExists());
                    }
                } R_END_TRY_CATCH;
            }
            ON_RESULT_FAILURE { ::DeleteFileW(native_path.get()); };
            ON_SCOPE_EXIT { ::CloseHandle(handle); };

            /* Set the file as sparse. */
            {
                DWORD dummy;
                ::DeviceIoControl(handle, FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, std::addressof(dummy), nullptr);
            }

            /* Set the file size. */
            if (size > 0) {
                R_TRY(SetFileSizeImpl(handle, size));
            }
        }
        #else
        {
            /* Create the file. */
            const auto handle = RetryForEIntr([&] () ALWAYS_INLINE_LAMBDA -> int {
                return ::open(native_path.get(), O_WRONLY | O_CREAT | O_EXCL, 0666);
            });
            R_UNLESS(handle >= 0, ConvertErrnoToResult(ErrnoSource_CreateFile));
            ON_RESULT_FAILURE { ::unlink(native_path.get()); };
            ON_SCOPE_EXIT { CloseFileDescriptor(handle); };

            /* Set the file as sparse. */
            /* TODO: How do you do this on macos/linux? */

            /* Set the file size. */
            if (size > 0) {
                R_TRY(SetFileSizeImpl(handle, size));
            }
        }
        #endif

        R_SUCCEED();
    }

    Result LocalFileSystem::DoDeleteFile(const fs::Path &path) {
        /* Resolve the path. */
        NativePathBuffer native_path;
        R_TRY(this->ResolveFullPath(std::addressof(native_path), path, MaxFilePathLength, 0, true));

        /* Delete the file, retrying on target locked. */
        auto delete_impl = [&] () -> Result {
            #if defined(ATMOSPHERE_OS_WINDOWS)
            {
                /* Try to delete the file directly. */
                R_SUCCEED_IF(::DeleteFileW(native_path.get()));

                /* Convert the last error to a result. */
                const auto last_error_result = ConvertLastErrorToResult();

                /* Check if access denied; it may indicate we tried to open a directory. */
                R_UNLESS(::GetLastError() == ERROR_ACCESS_DENIED, last_error_result);

                /* Check if we tried to open a directory. */
                fs::DirectoryEntryType type{};
                R_TRY(GetEntryTypeImpl(std::addressof(type), native_path.get()));

                /* If the type is anything other than directory, perform generic result conversion. */
                R_UNLESS(type == fs::DirectoryEntryType_Directory, last_error_result);

                /* Return path not found, for trying to open a file as a directory. */
                R_THROW(fs::ResultPathNotFound());
            }
            #else
            {
                /* If on macOS, we need to check if the path is a directory before trying to unlink it. */
                /* This is because unlink succeeds on directories when executing as superuser. */
                #if defined(ATMOSPHERE_OS_MACOS)
                {
                    /* Check if we tried to open a directory. */
                    fs::DirectoryEntryType type;
                    R_TRY(GetEntryTypeImpl(std::addressof(type), native_path.get()));
                    R_UNLESS(type == fs::DirectoryEntryType_File, fs::ResultPathNotFound());
                }
                #endif

                /* Delete the file. */
                const auto res = ::unlink(native_path.get());
                R_UNLESS(res >= 0, ConvertErrnoToResult(ErrnoSource_Unlink));
            }
            #endif

            R_SUCCEED();
        };

        /* Perform the delete. */
        R_TRY(fssystem::RetryToAvoidTargetLocked(delete_impl));

        /* Wait for the deletion to complete. */
        R_RETURN(WaitDeletionCompletion(native_path.get()));
    }

    Result LocalFileSystem::DoCreateDirectory(const fs::Path &path) {
        /* Check for path validity. */
        R_UNLESS(path != "/", fs::ResultPathNotFound());
        R_UNLESS(path != ".", fs::ResultPathNotFound());

        /* Resolve the path. */
        NativePathBuffer native_path;
        R_TRY(this->ResolveFullPath(std::addressof(native_path), path, MaxDirectoryPathLength, 0, false));

        /* Create the directory. */
        #if defined(ATMOSPHERE_OS_WINDOWS)
        R_UNLESS(::CreateDirectoryW(native_path.get(), nullptr), ConvertLastErrorToResult());
        #else
        R_UNLESS(::mkdir(native_path.get(), 0777) == 0, ConvertErrnoToResult(ErrnoSource_Mkdir));
        #endif

        R_SUCCEED();
    }

    Result LocalFileSystem::DoDeleteDirectory(const fs::Path &path) {
        /* Guard against deletion of raw drive. */
        #if defined(ATMOSPHERE_OS_WINDOWS)
        R_UNLESS(!fs::IsWindowsDriveRootPath(path), fs::ResultDirectoryNotDeletable());
        #else
        /* TODO: Linux/macOS? */
        #endif

        /* Resolve the path. */
        NativePathBuffer native_path;
        R_TRY(this->ResolveFullPath(std::addressof(native_path), path, MaxFilePathLength, 0, true));

        /* Delete the directory, retrying on target locked. */
        auto delete_impl = [&] () -> Result {
            #if defined(ATMOSPHERE_OS_WINDOWS)
            R_UNLESS(::RemoveDirectoryW(native_path.get()), ConvertLastErrorToResult());
            #else
            R_UNLESS(::rmdir(native_path.get()) == 0, ConvertErrnoToResult(ErrnoSource_Rmdir));
            #endif

            R_SUCCEED();
        };

        /* Perform the delete. */
        R_TRY(fssystem::RetryToAvoidTargetLocked(delete_impl));

        /* Wait for the deletion to complete. */
        R_RETURN(WaitDeletionCompletion(native_path.get()));
    }

    Result LocalFileSystem::DoDeleteDirectoryRecursively(const fs::Path &path) {
        /* Guard against deletion of raw drive. */
        #if defined(ATMOSPHERE_OS_WINDOWS)
        R_UNLESS(!fs::IsWindowsDriveRootPath(path), fs::ResultDirectoryNotDeletable());
        #else
        /* TODO: Linux/macOS? */
        #endif

        /* Resolve the path. */
        NativePathBuffer native_path;
        R_TRY(this->ResolveFullPath(std::addressof(native_path), path, MaxFilePathLength, 0, true));

        /* Delete the directory. */
        R_RETURN(this->DeleteDirectoryRecursivelyInternal(native_path.get(), true));
    }

    Result LocalFileSystem::DoRenameFile(const fs::Path &old_path, const fs::Path &new_path) {
        /* Resolve the old path. */
        NativePathBuffer native_old_path;
        R_TRY(this->ResolveFullPath(std::addressof(native_old_path), old_path, MaxFilePathLength, 0, true));

        /* Resolve the new path. */
        NativePathBuffer native_new_path;
        R_TRY(this->ResolveFullPath(std::addressof(native_new_path), new_path, MaxFilePathLength, 0, false));

        /* Check that the old path is a file. */
        fs::DirectoryEntryType type{};
        R_TRY(GetEntryTypeImpl(std::addressof(type), native_old_path.get()));
        R_UNLESS(type == fs::DirectoryEntryType_File, fs::ResultPathNotFound());

        /* Perform the rename. */
        auto rename_impl = [&]() -> Result {
            #if defined(ATMOSPHERE_OS_WINDOWS)
            if (!::MoveFileW(native_old_path.get(), native_new_path.get())) {
                R_TRY_CATCH(ConvertLastErrorToResult()) {
                    R_CATCH(fs::ResultTargetLocked) {
                        /* If we're performing a self rename, succeed. */
                        R_SUCCEED_IF(::wcscmp(native_old_path.get(), native_new_path.get()) == 0);

                        /* Otherwise, check if the new path already exists. */
                        const auto attr = ::GetFileAttributesW(native_new_path.get());
                        R_UNLESS(attr == INVALID_FILE_ATTRIBUTES, fs::ResultPathAlreadyExists());

                        /* Return the original result. */
                        R_THROW(R_CURRENT_RESULT);
                    }
                } R_END_TRY_CATCH;
            }
            #else
            {
                /* ::rename() will destroy an existing file at new path...so check for that case ahead of time. */
                {
                    struct stat st;
                    R_UNLESS(::stat(native_new_path.get(), std::addressof(st)) < 0, fs::ResultPathAlreadyExists());
                }

                /* Rename the file. */
                R_UNLESS(::rename(native_old_path.get(), native_new_path.get()) == 0, ConvertErrnoToResult(ErrnoSource_RenameFile));
            }
            #endif
            R_SUCCEED();
        };

        R_RETURN(fssystem::RetryToAvoidTargetLocked(rename_impl));
    }

    Result LocalFileSystem::DoRenameDirectory(const fs::Path &old_path, const fs::Path &new_path) {
        /* Resolve the old path. */
        NativePathBuffer native_old_path;
        R_TRY(this->ResolveFullPath(std::addressof(native_old_path), old_path, MaxDirectoryPathLength, 0, true));

        /* Resolve the new path. */
        NativePathBuffer native_new_path;
        R_TRY(this->ResolveFullPath(std::addressof(native_new_path), new_path, MaxDirectoryPathLength, 0, false));

        /* Check that the old path is a file. */
        fs::DirectoryEntryType type{};
        R_TRY(GetEntryTypeImpl(std::addressof(type), native_old_path.get()));
        R_UNLESS(type == fs::DirectoryEntryType_Directory, fs::ResultPathNotFound());

        /* Perform the rename. */
        auto rename_impl = [&]() -> Result {
            #if defined(ATMOSPHERE_OS_WINDOWS)
            if (!::MoveFileW(native_old_path.get(), native_new_path.get())) {
                R_TRY_CATCH(ConvertLastErrorToResult()) {
                    R_CATCH(fs::ResultTargetLocked) {
                        /* If we're performing a self rename, succeed. */
                        R_SUCCEED_IF(::wcscmp(native_old_path.get(), native_new_path.get()) == 0);

                        /* Otherwise, check if the new path already exists. */
                        const auto attr = ::GetFileAttributesW(native_new_path.get());
                        R_UNLESS(attr == INVALID_FILE_ATTRIBUTES, fs::ResultPathAlreadyExists());

                        /* Return the original result. */
                        R_THROW(R_CURRENT_RESULT);
                    }
                } R_END_TRY_CATCH;
            }
            #else
            {
                /* ::rename() will overwrite an existing empty directory at the target, so check for that ahead of time. */
                {
                    struct stat st;
                    R_UNLESS(::stat(native_new_path.get(), std::addressof(st)) < 0, fs::ResultPathAlreadyExists());
                }

                /* Rename the directory. */
                R_UNLESS(::rename(native_old_path.get(), native_new_path.get()) == 0, ConvertErrnoToResult(ErrnoSource_RenameDirectory));
            }
            #endif
            R_SUCCEED();
        };

        R_RETURN(fssystem::RetryToAvoidTargetLocked(rename_impl));
    }

    Result LocalFileSystem::DoGetEntryType(fs::DirectoryEntryType *out, const fs::Path &path) {
        /* Resolve the path. */
        NativePathBuffer native_path;
        R_TRY(this->ResolveFullPath(std::addressof(native_path), path, MaxFilePathLength, 0, true));

        /* Get the entry type. */
        R_RETURN(GetEntryTypeImpl(out, native_path.get()));
    }

    Result LocalFileSystem::DoOpenFile(std::unique_ptr<fs::fsa::IFile> *out_file, const fs::Path &path, fs::OpenMode mode) {
        /* Resolve the path. */
        NativePathBuffer native_path;
        R_TRY(this->ResolveFullPath(std::addressof(native_path), path, MaxFilePathLength, 0, true));

        /* Open the file, retrying on target locked. */
        auto open_impl = [&] () -> Result {
            #if defined(ATMOSPHERE_OS_WINDOWS)
            /* Open a windows file handle. */
            const DWORD desired_access = ((mode & fs::OpenMode_Read) ? GENERIC_READ : 0) | ((mode & fs::OpenMode_Write) ? GENERIC_WRITE : 0);
            const auto file_handle = ::CreateFileW(native_path.get(), desired_access, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);
            if (file_handle == INVALID_HANDLE_VALUE) {
                /* Convert the last error to a result. */
                const auto last_error_result = ConvertLastErrorToResult();

                /* Check if access denied; it may indicate we tried to open a directory. */
                R_UNLESS(::GetLastError() == ERROR_ACCESS_DENIED, last_error_result);

                /* Check if we tried to open a directory. */
                fs::DirectoryEntryType type{};
                R_TRY(GetEntryTypeImpl(std::addressof(type), native_path.get()));

                /* If the type isn't file, return path not found. */
                R_UNLESS(type == fs::DirectoryEntryType_File, fs::ResultPathNotFound());

                /* Return the error we encountered earlier. */
                R_THROW(last_error_result);
            }
            ON_RESULT_FAILURE { ::CloseHandle(file_handle); };
            #else
            const bool is_read  = (mode & fs::OpenMode_Read);
            const bool is_write = (mode & fs::OpenMode_Write);
            int file_handle = RetryForEIntr([&] () ALWAYS_INLINE_LAMBDA {
                return ::open(native_path.get(), (is_read && is_write) ? (O_RDWR) : (is_write ? (O_WRONLY) : (is_read ? (O_RDONLY) : (0))));
            });
            R_UNLESS(file_handle >= 0, ConvertErrnoToResult(ErrnoSource_OpenFile));
            ON_RESULT_FAILURE { CloseFileDescriptor(file_handle); };
            #endif

            /* Create a new local file. */
            auto file = std::make_unique<LocalFile>(file_handle, mode);
            R_UNLESS(file != nullptr, fs::ResultAllocationMemoryFailedInLocalFileSystemA());

            /* Set the output file. */
            *out_file = std::move(file);
            R_SUCCEED();
        };

        R_RETURN(fssystem::RetryToAvoidTargetLocked(open_impl));
    }

    Result LocalFileSystem::DoOpenDirectory(std::unique_ptr<fs::fsa::IDirectory> *out_dir, const fs::Path &path, fs::OpenDirectoryMode mode) {
        /* Resolve the path. */
        NativePathBuffer native_path;
        R_TRY(this->ResolveFullPath(std::addressof(native_path), path, MaxFilePathLength, 3, true));

        /* Open the directory, retrying on target locked. */
        auto open_impl = [&] () -> Result {
            #if defined(ATMOSPHERE_OS_WINDOWS)
            /* Open a handle file handle. */
            const auto dir_handle = ::CreateFileW(native_path.get(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
            R_UNLESS(dir_handle != INVALID_HANDLE_VALUE, ConvertLastErrorToResult());
            ON_RESULT_FAILURE { ::CloseHandle(dir_handle); };

            /* Check that we tried to open a directory. */
            fs::DirectoryEntryType type{};
            R_TRY(GetEntryTypeImpl(std::addressof(type), native_path.get()));

            /* If the type isn't directory, return path not found. */
            R_UNLESS(type == fs::DirectoryEntryType_Directory, fs::ResultPathNotFound());

            /* Fix up the path for us to perform a windows search. */
            const auto native_len = ::wcslen(native_path.get());
            const bool has_sep = native_len > 0 && native_path[native_len - 1] == '\\';
            if (has_sep) {
                native_path[native_len + 0] = '*';
                native_path[native_len + 1] = 0;
            } else {
                native_path[native_len + 0] = '\\';
                native_path[native_len + 1] = '*';
                native_path[native_len + 2] = 0;
            }
            #else
            /* Open the directory. */
            const auto dir_handle = RetryForEIntr([&] () ALWAYS_INLINE_LAMBDA { return ::open(native_path.get(), O_RDONLY | O_DIRECTORY); });
            R_UNLESS(dir_handle >= 0, ConvertErrnoToResult(ErrnoSource_OpenDirectory));
            ON_RESULT_FAILURE { CloseFileDescriptor(dir_handle); };
            #endif

            /* Create a new local directory. */
            auto dir = std::make_unique<LocalDirectory>(dir_handle, mode, std::move(native_path));
            R_UNLESS(dir != nullptr, fs::ResultAllocationMemoryFailedInLocalFileSystemB());

            /* Set the output directory. */
            *out_dir = std::move(dir);
            R_SUCCEED();
        };

        R_RETURN(fssystem::RetryToAvoidTargetLocked(open_impl));
    }

    Result LocalFileSystem::DoCommit() {
        R_SUCCEED();
    }

    Result LocalFileSystem::DoGetFreeSpaceSize(s64 *out, const fs::Path &path) {
        s64 dummy;
        R_RETURN(this->DoGetDiskFreeSpace(out, std::addressof(dummy), std::addressof(dummy), path));
    }

    Result LocalFileSystem::DoGetTotalSpaceSize(s64 *out, const fs::Path &path) {
        s64 dummy;
        R_RETURN(this->DoGetDiskFreeSpace(std::addressof(dummy), out, std::addressof(dummy), path));
    }

    Result LocalFileSystem::DoCleanDirectoryRecursively(const fs::Path &path) {
        /* Resolve the path. */
        NativePathBuffer native_path;
        R_TRY(this->ResolveFullPath(std::addressof(native_path), path, MaxFilePathLength, 0, true));

        /* Delete the directory. */
        R_RETURN(this->DeleteDirectoryRecursivelyInternal(native_path.get(), false));
    }

    Result LocalFileSystem::DoGetFileTimeStampRaw(fs::FileTimeStampRaw *out, const fs::Path &path) {
        /* Resolve the path. */
        NativePathBuffer native_path;
        R_TRY(this->ResolveFullPath(std::addressof(native_path), path, MaxFilePathLength, 0, true));

        /* Get the file timestamp. */
        #if defined(ATMOSPHERE_OS_WINDOWS)
        {
            /* Get the file attributes. */
            WIN32_FILE_ATTRIBUTE_DATA attr;
            R_UNLESS(::GetFileAttributesExW(native_path.get(), GetFileExInfoStandard, std::addressof(attr)), ConvertLastErrorToResult());

            /* Check that the file isn't a directory. */
            R_UNLESS((attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0, fs::ResultPathNotFound());

            /* Decode the FILETIME values. */
            const s64 create = static_cast<s64>(static_cast<u64>(attr.ftCreationTime.dwLowDateTime  ) | (static_cast<u64>(attr.ftCreationTime.dwHighDateTime  ) << BITSIZEOF(attr.ftCreationTime.dwLowDateTime  )));
            const s64 access = static_cast<s64>(static_cast<u64>(attr.ftLastAccessTime.dwLowDateTime) | (static_cast<u64>(attr.ftLastAccessTime.dwHighDateTime) << BITSIZEOF(attr.ftLastAccessTime.dwLowDateTime)));
            const s64 modify = static_cast<s64>(static_cast<u64>(attr.ftLastWriteTime.dwLowDateTime ) | (static_cast<u64>(attr.ftLastWriteTime.dwHighDateTime ) << BITSIZEOF(attr.ftLastWriteTime.dwLowDateTime )));

            /* Set the output timestamps. */
            if (m_use_posix_time) {
                out->create = ConvertWindowsTimeToPosixTime(create);
                out->access = ConvertWindowsTimeToPosixTime(access);
                out->modify = ConvertWindowsTimeToPosixTime(modify);
            } else {
                out->create = create;
                out->access = access;
                out->modify = modify;
            }

            /* We're not using local timestamps. */
            out->is_local_time = false;
        }
        #else
        {
            /* Get the file stats. */
            struct stat st;
            R_UNLESS(::stat(native_path.get(), std::addressof(st)) == 0, ConvertErrnoToResult(ErrnoSource_Stat));

            /* Check that the path isn't a directory. */
            R_UNLESS(!(S_ISDIR(st.st_mode)), fs::ResultPathNotFound());

            /* Set the output timestamps. */
            #if defined(ATMOSPHERE_OS_LINUX)
            if (m_use_posix_time) {
                out->create = st.st_ctim.tv_sec;
                out->access = st.st_atim.tv_sec;
                out->modify = st.st_mtim.tv_sec;
            } else {
                out->create = ConvertPosixTimeToWindowsTime(st.st_ctim.tv_sec, st.st_ctim.tv_nsec);
                out->access = ConvertPosixTimeToWindowsTime(st.st_atim.tv_sec, st.st_atim.tv_nsec);
                out->modify = ConvertPosixTimeToWindowsTime(st.st_mtim.tv_sec, st.st_mtim.tv_nsec);
            }
            #else
            if (m_use_posix_time) {
                out->create = st.st_ctimespec.tv_sec;
                out->access = st.st_atimespec.tv_sec;
                out->modify = st.st_mtimespec.tv_sec;
            } else {
                out->create = ConvertPosixTimeToWindowsTime(st.st_ctimespec.tv_sec, st.st_ctimespec.tv_nsec);
                out->access = ConvertPosixTimeToWindowsTime(st.st_atimespec.tv_sec, st.st_atimespec.tv_nsec);
                out->modify = ConvertPosixTimeToWindowsTime(st.st_mtimespec.tv_sec, st.st_mtimespec.tv_nsec);
            }
            #endif

            /* We're not using local timestamps. */
            out->is_local_time = false;
        }
        #endif

        R_SUCCEED();
    }

    Result LocalFileSystem::DoQueryEntry(char *dst, size_t dst_size, const char *src, size_t src_size, fs::fsa::QueryId query, const fs::Path &path) {
        AMS_UNUSED(dst, dst_size, src, src_size, query, path);
        R_THROW(fs::ResultUnsupportedOperation());
    }

    Result LocalFileSystem::DoCommitProvisionally(s64 counter) {
        AMS_UNUSED(counter);
        R_SUCCEED();
    }

    Result LocalFileSystem::DoRollback() {
        R_SUCCEED();
    }

}
#endif