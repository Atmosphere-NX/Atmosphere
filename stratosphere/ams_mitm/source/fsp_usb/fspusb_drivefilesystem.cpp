#include "fspusb_drivefilesystem.hpp"

extern HosMutex g_usbdrive_drives_lock;
extern std::vector<DriveData> g_usbdrive_drives;

DriveData *DriveFileSystem::GetDriveAccess() {
    DRIVES_SCOPE_GUARD;
    if(drvidx >= g_usbdrive_drives.size()) {
        return NULL;
    }
    return &g_usbdrive_drives[drvidx];
}

bool DriveFileSystem::IsOk() {
    DRIVES_SCOPE_GUARD;
    for(u32 i = 0; i < g_usbdrive_drives.size(); i++) {
        if(g_usbdrive_drives[i].usbif.ID == usbid) {
            return true;
        }
    }
    return false;
}

Result DriveFileSystem::CreateFileImpl(const FsPath &path, uint64_t size, int flags) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    std::string pth = GetFullPath(path);
    if(pth.empty()) {
        return FspUsbResults::DriveUnavailable;
    }
    FIL fp;
    auto res = f_open(&fp, pth.c_str(), FA_CREATE_NEW | FA_WRITE);
    if(res == FR_OK) {
        f_lseek(&fp, size);
        f_close(&fp);
    }
    return FspUsbResults::MakeFATFSErrorResult(res);
}

Result DriveFileSystem::DeleteFileImpl(const FsPath &path) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    std::string pth = GetFullPath(path);
    if(pth.empty()) {
        return FspUsbResults::DriveUnavailable;
    }
    auto res = f_unlink(pth.c_str());
    return FspUsbResults::MakeFATFSErrorResult(res);
}

Result DriveFileSystem::CreateDirectoryImpl(const FsPath &path) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    std::string pth = GetFullPath(path);
    if(pth.empty()) {
        return FspUsbResults::DriveUnavailable;
    }
    auto res = f_mkdir(pth.c_str());
    return FspUsbResults::MakeFATFSErrorResult(res);
}

Result DriveFileSystem::DeleteDirectoryImpl(const FsPath &path) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    std::string pth = GetFullPath(path);
    if(pth.empty()) {
        return FspUsbResults::DriveUnavailable;
    }
    auto res = f_rmdir(pth.c_str());
    return FspUsbResults::MakeFATFSErrorResult(res);
}

Result DriveFileSystem::DeleteDirectoryRecursivelyImpl(const FsPath &path) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    std::string pth = GetFullPath(path);
    if(pth.empty()) {
        return FspUsbResults::DriveUnavailable;
    }
    // TODO: do this!
    auto res = FR_OK;
    return FspUsbResults::MakeFATFSErrorResult(res);
}

Result DriveFileSystem::RenameFileImpl(const FsPath &old_path, const FsPath &new_path) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    std::string pth = GetFullPath(old_path);
    if(pth.empty()) {
        return FspUsbResults::DriveUnavailable;
    }
    std::string newpth = GetFullPath(new_path);
    if(newpth.empty()) {
        return FspUsbResults::DriveUnavailable;
    }
    auto res = f_rename(pth.c_str(), newpth.c_str());
    return FspUsbResults::MakeFATFSErrorResult(res);
}

Result DriveFileSystem::RenameDirectoryImpl(const FsPath &old_path, const FsPath &new_path) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    std::string pth = GetFullPath(old_path);
    if(pth.empty()) {
        return FspUsbResults::DriveUnavailable;
    }
    std::string newpth = GetFullPath(new_path);
    if(newpth.empty()) {
        return FspUsbResults::DriveUnavailable;
    }
    auto res = f_rename(pth.c_str(), newpth.c_str());
    return FspUsbResults::MakeFATFSErrorResult(res);
}

Result DriveFileSystem::GetEntryTypeImpl(DirectoryEntryType *out, const FsPath &path) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    std::string pth = GetFullPath(path);
    if(pth.empty()) {
        return FspUsbResults::DriveUnavailable;
    }
    FILINFO info;
    auto res = f_stat(pth.c_str(), &info);
    if(res == FR_OK) {
        if(info.fattrib & AM_DIR) {
            *out = DirectoryEntryType_Directory;
        }
        else {
            *out = DirectoryEntryType_File;
        }
    }
    return FspUsbResults::MakeFATFSErrorResult(res);
}

Result DriveFileSystem::OpenFileImpl(std::unique_ptr<IFile> &out_file, const FsPath &path, OpenMode mode) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    std::string pth = GetFullPath(path);
    if(pth.empty()) {
        return FspUsbResults::DriveUnavailable;
    }
    FIL fp;
    auto res = f_open(&fp, pth.c_str(), FA_OPEN_EXISTING | FA_READ | FA_WRITE);
    if(res == FR_OK) {
        out_file = std::make_unique<DriveFile>(fp, GetDriveAccess());
    }
    return FspUsbResults::MakeFATFSErrorResult(res);
}

Result DriveFileSystem::OpenDirectoryImpl(std::unique_ptr<IDirectory> &out_dir, const FsPath &path, DirectoryOpenMode mode) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    std::string pth = GetFullPath(path);
    if(pth.empty()) {
        return FspUsbResults::DriveUnavailable;
    }
    DIR dp;
    auto res = f_opendir(&dp, pth.c_str());
    if(res == FR_OK) {
        out_dir = std::make_unique<DriveDirectory>(dp, GetDriveAccess());
    }
    return FspUsbResults::MakeFATFSErrorResult(res);
}

Result DriveFileSystem::CommitImpl() {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    return 0;
}

Result DriveFileSystem::GetFreeSpaceSizeImpl(uint64_t *out, const FsPath &path) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    FATFS *fs = NULL;
    DWORD clstrs = 0;
    auto res = f_getfree(GetDriveAccess()->mountname, &clstrs, &fs);
    if(res == FR_OK) {
        if(fs) {
            *out = (uint64_t)(clstrs * fs->csize);
        }
    }
    return FspUsbResults::MakeFATFSErrorResult(res);
}

Result DriveFileSystem::GetTotalSpaceSizeImpl(uint64_t *out, const FsPath &path) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    FATFS *fs = NULL;
    DWORD clstrs = 0;
    auto res = f_getfree(GetDriveAccess()->mountname, &clstrs, &fs);
    if(res == FR_OK) {
        if(fs) {
            *out = (uint64_t)((fs->n_fatent - 2) * fs->csize);
        }
    }
    return FspUsbResults::MakeFATFSErrorResult(res);
}

Result DriveFileSystem::CleanDirectoryRecursivelyImpl(const FsPath &path) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    // Handle this!
    return 0;
}

Result DriveFileSystem::GetFileTimeStampRawImpl(FsTimeStampRaw *out, const FsPath &path) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    return 0;
}

Result DriveFileSystem::QueryEntryImpl(char *out, uint64_t out_size, const char *in, uint64_t in_size, int query, const FsPath &path) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    return 0;
}