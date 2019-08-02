#include "fspusb_drivefile.hpp"

extern HosMutex g_usbdrive_drives_lock;
extern std::vector<DriveData> g_usbdrive_drives;

bool DriveFile::IsOk() {
    DRIVES_SCOPE_GUARD;
    // Before any command, check whether the file's drive is still accessible
    for(u32 i = 0; i < g_usbdrive_drives.size(); i++) {
        if(g_usbdrive_drives[i].usbif.ID == usbid) {
            return true;
        }
    }
    return false;
}

Result DriveFile::ReadImpl(u64 *out, u64 offset, void *buffer, u64 size) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    auto res = f_lseek(&fp, offset);
    if(res == FR_OK) {
        res = f_read(&fp, buffer, size, (UINT*)out);
    }
    return FspUsbResults::MakeFATFSErrorResult(res);
}

Result DriveFile::GetSizeImpl(u64 *out) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    *out = f_size(&fp);
    return 0;
}

Result DriveFile::FlushImpl() {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    return 0;
}

Result DriveFile::WriteImpl(u64 offset, void *buffer, u64 size, u32 option) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    auto res = f_lseek(&fp, offset);
    if(res == FR_OK) {
        u64 out;
        res = f_write(&fp, buffer, size, (UINT*)&out);
    }
    return FspUsbResults::MakeFATFSErrorResult(res);
}

Result DriveFile::SetSizeImpl(u64 size) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    auto res = f_lseek(&fp, size);
    return FspUsbResults::MakeFATFSErrorResult(res);
}

Result DriveFile::OperateRangeImpl(FsOperationId operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) {
    if(!IsOk()) {
        return FspUsbResults::DriveUnavailable;
    }
    return 0;
}