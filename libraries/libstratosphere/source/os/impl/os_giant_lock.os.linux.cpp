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
#include "os_giant_lock.os.linux.hpp"
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>

namespace ams::os::impl {

    #define AMS_OS_IMPL_GIANT_LOCK_FILE_PATH "/tmp/.ams_os_giant_lock"

    void GiantLockLinuxImpl::Lock() {
        m_fd = ::open(AMS_OS_IMPL_GIANT_LOCK_FILE_PATH, O_RDWR | O_CREAT | O_CLOEXEC, S_IRUSR | S_IWUSR);
        AMS_ABORT_UNLESS(m_fd >= 0);

        while (::lockf(m_fd, F_LOCK, 0) < 0) {
            AMS_ABORT_UNLESS(errno == EINTR);
        }
    }

    void GiantLockLinuxImpl::Unlock() {
        AMS_ASSERT(m_fd >= 0);

        while (::lockf(m_fd, F_ULOCK, 0) < 0) {
            AMS_ABORT_UNLESS(errno == EINTR);
        }

        while (::close(m_fd) < 0) {
            AMS_ABORT_UNLESS(errno == EINTR);
        }

        m_fd = os::InvalidNativeHandle;
    }

}
