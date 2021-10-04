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

namespace ams::util {

    namespace {

        struct UuidImpl {
            util::BitPack32 data[4];

            using TimeLow               = util::BitPack32::Field<0,                           BITSIZEOF(u32), u32>;

            using TimeMid               = util::BitPack32::Field<0,                           BITSIZEOF(u16), u16>;
            using TimeHighAndVersion    = util::BitPack32::Field<TimeMid::Next,               BITSIZEOF(u16), u16>;
            using Version               = util::BitPack32::Field<TimeMid::Next + 12,                       4, u16>;

            static_assert(TimeHighAndVersion::Next == Version::Next);

            using ClockSeqHiAndReserved = util::BitPack32::Field<0,                           BITSIZEOF(u8),  u8>;
            using Reserved              = util::BitPack32::Field<6,                           2,              u8>;
            using ClockSeqLow           = util::BitPack32::Field<ClockSeqHiAndReserved::Next, BITSIZEOF(u8),  u8>;
            using NodeLow               = util::BitPack32::Field<ClockSeqLow::Next,           BITSIZEOF(u16), u16>;

            static_assert(ClockSeqHiAndReserved::Next == Reserved::Next);

            using NodeHigh              = util::BitPack32::Field<0,                           BITSIZEOF(u32), u32>;


            inline Uuid Convert() const {
                /* Convert the fields from native endian to big endian. */
                util::BitPack32 converted[4] = {util::BitPack32{0}, util::BitPack32{0}, util::BitPack32{0}, util::BitPack32{0}};

                converted[0].Set<TimeLow>(util::ConvertToBigEndian(this->data[0].Get<TimeLow>()));

                converted[1].Set<TimeMid>(util::ConvertToBigEndian(this->data[1].Get<TimeMid>()));
                converted[1].Set<TimeHighAndVersion>(util::ConvertToBigEndian(this->data[1].Get<TimeHighAndVersion>()));


                converted[2].Set<ClockSeqHiAndReserved>(util::ConvertToBigEndian(this->data[2].Get<ClockSeqHiAndReserved>()));
                converted[2].Set<ClockSeqLow>(util::ConvertToBigEndian(this->data[2].Get<ClockSeqLow>()));

                u64 node_lo = static_cast<u64>(this->data[2].Get<NodeLow>());
                u64 node_hi = static_cast<u64>(this->data[3].Get<NodeHigh>());
                u64 node    = util::ConvertToBigEndian48(static_cast<u64>((node_hi << BITSIZEOF(u16)) | (node_lo)));

                constexpr u64 NodeLoMask = (UINT64_C(1) << BITSIZEOF(u16)) - 1u;
                constexpr u64 NodeHiMask = (UINT64_C(1) << BITSIZEOF(u32)) - 1u;

                converted[2].Set<NodeLow>(static_cast<u16>(node & NodeLoMask));
                converted[3].Set<NodeHigh>(static_cast<u32>((node >> BITSIZEOF(u16)) & NodeHiMask));

                Uuid uuid;
                std::memcpy(uuid.data, converted, sizeof(uuid.data));
                return uuid;
            }
        };
        static_assert(sizeof(UuidImpl) == sizeof(Uuid));

        ALWAYS_INLINE Uuid GenerateUuidVersion4() {
            constexpr u16 Version  = 0x4;
            constexpr u8  Reserved = 0x1;

            /* Generate a random uuid. */
            UuidImpl uuid = {util::BitPack32{0}, util::BitPack32{0}, util::BitPack32{0}, util::BitPack32{0}};
            os::GenerateRandomBytes(uuid.data, sizeof(uuid.data));

            /* Set version and reserved. */
            uuid.data[1].Set<UuidImpl::Version>(Version);
            uuid.data[2].Set<UuidImpl::Reserved>(Reserved);

            /* Return the uuid. */
            return uuid.Convert();
        }

    }

    Uuid GenerateUuid() {
        return GenerateUuidVersion4();
    }

    Uuid GenerateUuidVersion5(const void *sha1_hash) {
        constexpr u16 Version  = 0x5;
        constexpr u8  Reserved = 0x1;

        /* Generate a uuid from a SHA1 hash. */
        UuidImpl uuid = {util::BitPack32{0}, util::BitPack32{0}, util::BitPack32{0}, util::BitPack32{0}};
        std::memcpy(uuid.data, sha1_hash, sizeof(uuid.data));

        /* Set version and reserved. */
        uuid.data[1].Set<UuidImpl::Version>(Version);
        uuid.data[2].Set<UuidImpl::Reserved>(Reserved);

        /* Return the uuid. */
        return uuid.Convert();
    }

}
