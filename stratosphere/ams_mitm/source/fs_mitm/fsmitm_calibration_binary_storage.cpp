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
#include "fsmitm_calibration_binary_storage.hpp"

namespace ams::mitm::fs {

    using namespace ams::fs;

    namespace {

        constinit os::SdkMutex g_cal0_access_mutex;

    }
    Result CalibrationBinaryStorage::Read(s64 offset, void *_buffer, size_t size) {
        /* Acquire exclusive calibration binary access. */
        std::scoped_lock lk(g_cal0_access_mutex);

        /* Get u8 buffer. */
        u8 *buffer = static_cast<u8 *>(_buffer);

        /* Succeed on zero-size. */
        R_SUCCEED_IF(size == 0);

        /* Handle the blank region. */
        if (m_read_blank) {
            if (BlankStartOffset <= offset && offset < BlankEndOffset) {
                const size_t blank_size = std::min(size, static_cast<size_t>(BlankEndOffset - offset));
                mitm::ReadFromBlankCalibrationBinary(offset, buffer, blank_size);
                size   -= blank_size;
                buffer += blank_size;
                offset += blank_size;
            }
        }

        /* Succeed if we're done. */
        R_SUCCEED_IF(size == 0);

        /* Handle any in-between data. */
        if (BlankEndOffset <= offset && offset < FakeSecureStartOffset) {
            const size_t mid_size = std::min(size, static_cast<size_t>(FakeSecureStartOffset - offset));
            R_TRY(Base::Read(offset, buffer, mid_size));
            size   -= mid_size;
            buffer += mid_size;
            offset += mid_size;
        }

        /* Succeed if we're done. */
        R_SUCCEED_IF(size == 0);

        /* Handle the secure region. */
        if (FakeSecureStartOffset <= offset && offset < FakeSecureEndOffset) {
            const size_t fake_size = std::min(size, static_cast<size_t>(FakeSecureEndOffset - offset));
            mitm::ReadFromFakeSecureBackupStorage(offset, buffer, fake_size);
            size   -= fake_size;
            buffer += fake_size;
            offset += fake_size;
        }

        /* Succeed if we're done. */
        R_SUCCEED_IF(size == 0);

        /* Handle any remaining data. */
        return Base::Read(offset, buffer, size);
    }

    Result CalibrationBinaryStorage::Write(s64 offset, const void *_buffer, size_t size) {
        /* Acquire exclusive calibration binary access. */
        std::scoped_lock lk(g_cal0_access_mutex);

        /* Get const u8 buffer. */
        const u8 *buffer = static_cast<const u8 *>(_buffer);

        /* Succeed on zero-size. */
        R_SUCCEED_IF(size == 0);

        /* Only allow writes if we should. */
        R_UNLESS(m_allow_writes, fs::ResultUnsupportedOperation());

        /* Handle the blank region. */
        if (m_read_blank) {
            if (BlankStartOffset <= offset && offset < BlankEndOffset) {
                const size_t blank_size = std::min(size, static_cast<size_t>(BlankEndOffset - offset));
                mitm::WriteToBlankCalibrationBinary(offset, buffer, blank_size);
                size   -= blank_size;
                buffer += blank_size;
                offset += blank_size;
            }
        }

        /* Succeed if we're done. */
        R_SUCCEED_IF(size == 0);

        /* Handle any in-between data. */
        if (BlankEndOffset <= offset && offset < FakeSecureStartOffset) {
            const size_t mid_size = std::min(size, static_cast<size_t>(FakeSecureStartOffset - offset));
            R_TRY(Base::Write(offset, buffer, mid_size));
            size   -= mid_size;
            buffer += mid_size;
            offset += mid_size;
        }

        /* Succeed if we're done. */
        R_SUCCEED_IF(size == 0);

        /* Handle the secure region. */
        if (FakeSecureStartOffset <= offset && offset < FakeSecureEndOffset) {
            const size_t fake_size = std::min(size, static_cast<size_t>(FakeSecureEndOffset - offset));
            mitm::WriteToFakeSecureBackupStorage(offset, buffer, fake_size);
            size   -= fake_size;
            buffer += fake_size;
            offset += fake_size;
        }

        /* Succeed if we're done. */
        R_SUCCEED_IF(size == 0);

        /* Handle any remaining data. */
        return Base::Write(offset, buffer, size);
    }

}
