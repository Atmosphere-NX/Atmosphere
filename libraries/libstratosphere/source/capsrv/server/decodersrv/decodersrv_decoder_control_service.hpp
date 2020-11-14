/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

namespace ams::capsrv::server {

    #define AMS_CAPSRV_DECODER_CONTROL_SERVICE_INTERFACE_INFO(C, H)                                                                                                                  \
        AMS_SF_METHOD_INFO(C, H, 3001, Result, DecodeJpeg, (const sf::OutNonSecureBuffer &out, const sf::InBuffer &in, u32 width, u32 height, const ScreenShotDecodeOption &option))

    AMS_SF_DEFINE_INTERFACE(IDecoderControlService, AMS_CAPSRV_DECODER_CONTROL_SERVICE_INTERFACE_INFO)

    class DecoderControlService final {
        public:
            Result DecodeJpeg(const sf::OutNonSecureBuffer &out, const sf::InBuffer &in, u32 width, u32 height, const ScreenShotDecodeOption &option);
    };
    static_assert(IsIDecoderControlService<DecoderControlService>);

}