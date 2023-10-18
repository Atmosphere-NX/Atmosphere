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
#include <stratosphere.hpp>

#define AMS_CAPSRV_DECODER_CONTROL_SERVICE_INTERFACE_INFO(C, H) \
    AMS_SF_METHOD_INFO(C, H, 3001, Result, DecodeJpeg, (const ams::sf::OutNonSecureBuffer &out, const ams::sf::InBuffer &in, u32 width, u32 height, const capsrv::ScreenShotDecodeOption &option),                             (out, in, width, height, option)) \
    AMS_SF_METHOD_INFO(C, H, 4001, Result, ShrinkJpeg, (ams::sf::Out<u64> out_size, const ams::sf::OutNonSecureBuffer &out, const ams::sf::InBuffer &in, u32 width, u32 height, const capsrv::ScreenShotDecodeOption &option), (out_size, out, in, width, height, option))

AMS_SF_DEFINE_INTERFACE(ams::capsrv::sf, IDecoderControlService, AMS_CAPSRV_DECODER_CONTROL_SERVICE_INTERFACE_INFO, 0xD168E90B)

namespace ams::capsrv::server {

    class DecoderControlService final {
        public:
            Result DecodeJpeg(const ams::sf::OutNonSecureBuffer &out, const ams::sf::InBuffer &in, u32 width, u32 height, const ScreenShotDecodeOption &option);
            Result ShrinkJpeg(ams::sf::Out<u64> out_size, const ams::sf::OutNonSecureBuffer &out, const ams::sf::InBuffer &in, u32 width, u32 height, const capsrv::ScreenShotDecodeOption &option);
    };
    static_assert(capsrv::sf::IsIDecoderControlService<DecoderControlService>);

}
