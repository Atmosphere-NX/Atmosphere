/*
 * Copyright (c) Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License
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
#include <vapours/svc/svc_types_common.hpp>
#include <vapours/svc/svc_select_thread_local_region.hpp>

namespace ams::svc::ipc {

    #pragma GCC push_options
    #pragma GCC optimize ("-O3")

    ALWAYS_INLINE u32 *GetMessageBuffer() {
        return GetThreadLocalRegion()->message_buffer;
    }

    constexpr inline size_t MessageBufferSize = sizeof(::ams::svc::ThreadLocalRegion::message_buffer);

    class MessageBuffer {
        public:
            class MessageHeader {
                private:
                    /* Define fields for the first header word. */
                    using Tag           = util::BitPack32::Field<0,                  BITSIZEOF(u16), u16>;
                    using PointerCount  = util::BitPack32::Field<Tag::Next,                       4, s32>;
                    using SendCount     = util::BitPack32::Field<PointerCount::Next,              4, s32>;
                    using ReceiveCount  = util::BitPack32::Field<SendCount::Next,                 4, s32>;
                    using ExchangeCount = util::BitPack32::Field<ReceiveCount::Next,              4, s32>;
                    static_assert(ExchangeCount::Next == BITSIZEOF(u32));

                    /* Define fields for the second header word. */
                    using RawCount          = util::BitPack32::Field<0,                       10, s32>;
                    using ReceiveListCount  = util::BitPack32::Field<RawCount::Next,           4, s32>;
                    using Reserved0         = util::BitPack32::Field<ReceiveListCount::Next,   6, u32>;
                    using ReceiveListOffset = util::BitPack32::Field<Reserved0::Next,         11, s32>;
                    using HasSpecialHeader  = util::BitPack32::Field<ReceiveListOffset::Next,  1, bool>;

                    static constexpr inline u64 NullTag = 0;
                    static_assert(HasSpecialHeader::Next == BITSIZEOF(u32));
                public:
                    enum ReceiveListCountType {
                        ReceiveListCountType_None            = 0,
                        ReceiveListCountType_ToMessageBuffer = 1,
                        ReceiveListCountType_ToSingleBuffer  = 2,

                        ReceiveListCountType_CountOffset = 2,
                        ReceiveListCountType_CountMax    = 13,
                    };
                private:
                    util::BitPack32 m_header[2];
                public:
                    constexpr ALWAYS_INLINE MessageHeader() : m_header{util::BitPack32{0}, util::BitPack32{0}} {
                        m_header[0].Set<Tag>(NullTag);
                    }

                    constexpr ALWAYS_INLINE MessageHeader(u16 tag, bool special, s32 ptr, s32 send, s32 recv, s32 exch, s32 raw, s32 recv_list) : m_header{util::BitPack32{0}, util::BitPack32{0}} {
                        m_header[0].Set<Tag>(tag);
                        m_header[0].Set<PointerCount>(ptr);
                        m_header[0].Set<SendCount>(send);
                        m_header[0].Set<ReceiveCount>(recv);
                        m_header[0].Set<ExchangeCount>(exch);

                        m_header[1].Set<RawCount>(raw);
                        m_header[1].Set<ReceiveListCount>(recv_list);
                        m_header[1].Set<HasSpecialHeader>(special);
                    }

                    ALWAYS_INLINE explicit MessageHeader(const MessageBuffer &buf) : m_header{util::BitPack32{0}, util::BitPack32{0}} {
                        buf.Get(0, m_header, util::size(m_header));
                    }

                    ALWAYS_INLINE explicit MessageHeader(const u32 *msg) : m_header{util::BitPack32{msg[0]}, util::BitPack32{msg[1]}} { /* ... */ }

                    constexpr ALWAYS_INLINE u16 GetTag() const {
                        return m_header[0].Get<Tag>();
                    }

                    constexpr ALWAYS_INLINE s32 GetPointerCount() const {
                        return m_header[0].Get<PointerCount>();
                    }

                    constexpr ALWAYS_INLINE s32 GetSendCount() const {
                        return m_header[0].Get<SendCount>();
                    }

                    constexpr ALWAYS_INLINE s32 GetReceiveCount() const {
                        return m_header[0].Get<ReceiveCount>();
                    }

                    constexpr ALWAYS_INLINE s32 GetExchangeCount() const {
                        return m_header[0].Get<ExchangeCount>();
                    }

                    constexpr ALWAYS_INLINE s32 GetMapAliasCount() const {
                        return this->GetSendCount() + this->GetReceiveCount() + this->GetExchangeCount();
                    }

                    constexpr ALWAYS_INLINE s32 GetRawCount() const {
                        return m_header[1].Get<RawCount>();
                    }

                    constexpr ALWAYS_INLINE s32 GetReceiveListCount() const {
                        return m_header[1].Get<ReceiveListCount>();
                    }

                    constexpr ALWAYS_INLINE s32 GetReceiveListOffset() const {
                        return m_header[1].Get<ReceiveListOffset>();
                    }

                    constexpr ALWAYS_INLINE bool GetHasSpecialHeader() const {
                        return m_header[1].Get<HasSpecialHeader>();
                    }

                    constexpr ALWAYS_INLINE void SetReceiveListCount(s32 recv_list) {
                        m_header[1].Set<ReceiveListCount>(recv_list);
                    }

                    constexpr ALWAYS_INLINE const util::BitPack32 *GetData() const {
                        return m_header;
                    }

                    static constexpr ALWAYS_INLINE size_t GetDataSize() {
                        return sizeof(m_header);
                    }
            };

            class SpecialHeader {
                private:
                    /* Define fields for the header word. */
                    using HasProcessId    = util::BitPack32::Field<0,                     1, bool>;
                    using CopyHandleCount = util::BitPack32::Field<HasProcessId::Next,    4, s32>;
                    using MoveHandleCount = util::BitPack32::Field<CopyHandleCount::Next, 4, s32>;
                private:
                    util::BitPack32 m_header;
                    bool m_has_header;
                public:
                    constexpr ALWAYS_INLINE explicit SpecialHeader(bool pid, s32 copy, s32 move) : m_header{0}, m_has_header(true) {
                        m_header.Set<HasProcessId>(pid);
                        m_header.Set<CopyHandleCount>(copy);
                        m_header.Set<MoveHandleCount>(move);
                    }

                    consteval explicit SpecialHeader(bool pid, s32 copy, s32 move, bool _has_header) : m_header{0}, m_has_header(_has_header) {
                        m_header.Set<HasProcessId>(pid);
                        m_header.Set<CopyHandleCount>(copy);
                        m_header.Set<MoveHandleCount>(move);

                        AMS_ASSUME(m_has_header == (this->GetHasProcessId() || this->GetCopyHandleCount() > 0 || this->GetMoveHandleCount() > 0));
                    }

                    ALWAYS_INLINE explicit SpecialHeader(const MessageBuffer &buf, const MessageHeader &hdr) : m_header{0}, m_has_header(hdr.GetHasSpecialHeader()) {
                        if (m_has_header) {
                            buf.Get(MessageHeader::GetDataSize() / sizeof(util::BitPack32), std::addressof(m_header), sizeof(m_header) / sizeof(util::BitPack32));
                        }
                    }

                    constexpr ALWAYS_INLINE bool GetHasProcessId() const {
                        return m_header.Get<HasProcessId>();
                    }

                    constexpr ALWAYS_INLINE s32 GetCopyHandleCount() const {
                        return m_header.Get<CopyHandleCount>();
                    }

                    constexpr ALWAYS_INLINE s32 GetMoveHandleCount() const {
                        return m_header.Get<MoveHandleCount>();
                    }

                    constexpr ALWAYS_INLINE const util::BitPack32 *GetHeader() const {
                        return std::addressof(m_header);
                    }

                    constexpr ALWAYS_INLINE size_t GetHeaderSize() const {
                        if (m_has_header) {
                            return sizeof(m_header);
                        } else {
                            return 0;
                        }
                    }

                    constexpr ALWAYS_INLINE size_t GetDataSize() const {
                        if (m_has_header) {
                            return (this->GetHasProcessId() ? sizeof(u64) : 0)   +
                                   (this->GetCopyHandleCount() * sizeof(Handle)) +
                                   (this->GetMoveHandleCount() * sizeof(Handle));
                        } else {
                            return 0;
                        }
                    }
            };

            class MapAliasDescriptor {
                public:
                    enum Attribute {
                        Attribute_Ipc          = 0,
                        Attribute_NonSecureIpc = 1,
                        Attribute_NonDeviceIpc = 3,
                    };
                private:
                    /* Define fields for the first two words. */
                    using SizeLow     = util::BitPack32::Field<0, BITSIZEOF(u32), u32>;
                    using AddressLow  = util::BitPack32::Field<0, BITSIZEOF(u32), u32>;

                    /* Define fields for the packed descriptor word. */
                    using Attributes  = util::BitPack32::Field<0,                  2, Attribute>;
                    using AddressHigh = util::BitPack32::Field<Attributes::Next,   3, u32>;
                    using Reserved    = util::BitPack32::Field<AddressHigh::Next, 19, u32>;
                    using SizeHigh    = util::BitPack32::Field<Reserved::Next,     4, u32>;
                    using AddressMid  = util::BitPack32::Field<SizeHigh::Next,     4, u32>;

                    constexpr ALWAYS_INLINE u32 GetAddressMid(u64 address) {
                        return static_cast<u32>(address >> AddressLow::Count) & ((1u << AddressMid::Count) - 1);
                    }

                    constexpr ALWAYS_INLINE u32 GetAddressHigh(u64 address) {
                        return static_cast<u32>(address >> (AddressLow::Count + AddressMid::Count));
                    }
                private:
                    util::BitPack32 m_data[3];
                public:
                    constexpr ALWAYS_INLINE MapAliasDescriptor() : m_data{util::BitPack32{0}, util::BitPack32{0}, util::BitPack32{0}} { /* ... */ }

                    ALWAYS_INLINE MapAliasDescriptor(const void *buffer, size_t _size, Attribute attr = Attribute_Ipc) : m_data{util::BitPack32{0}, util::BitPack32{0}, util::BitPack32{0}} {
                        const u64 address = reinterpret_cast<u64>(buffer);
                        const u64 size    = static_cast<u64>(_size);
                        m_data[0] = { static_cast<u32>(size) };
                        m_data[1] = { static_cast<u32>(address) };

                        m_data[2].Set<Attributes>(attr);
                        m_data[2].Set<AddressMid>(GetAddressMid(address));
                        m_data[2].Set<SizeHigh>(static_cast<u32>(size >> SizeLow::Count));
                        m_data[2].Set<AddressHigh>(GetAddressHigh(address));
                    }

                    ALWAYS_INLINE MapAliasDescriptor(const MessageBuffer &buf, s32 index) : m_data{util::BitPack32{0}, util::BitPack32{0}, util::BitPack32{0}} {
                        buf.Get(index, m_data, util::size(m_data));
                    }

                    constexpr ALWAYS_INLINE uintptr_t GetAddress() const {
                        const u64 address = (static_cast<u64>((m_data[2].Get<AddressHigh>() << AddressMid::Count) | m_data[2].Get<AddressMid>()) << AddressLow::Count) | m_data[1].Get<AddressLow>();
                        return address;
                    }

                    constexpr ALWAYS_INLINE uintptr_t GetSize() const {
                        const u64 size = (static_cast<u64>(m_data[2].Get<SizeHigh>()) << SizeLow::Count) | m_data[0].Get<SizeLow>();
                        return size;
                    }

                    constexpr ALWAYS_INLINE Attribute GetAttribute() const {
                        return m_data[2].Get<Attributes>();
                    }

                    constexpr ALWAYS_INLINE const util::BitPack32 *GetData() const {
                        return m_data;
                    }

                    static constexpr ALWAYS_INLINE size_t GetDataSize() {
                        return sizeof(m_data);
                    }
            };

            class PointerDescriptor {
                private:
                    /* Define fields for the packed descriptor word. */
                    using Index       = util::BitPack32::Field<0,                  4, s32>;
                    using Reserved0   = util::BitPack32::Field<Index::Next,        2, u32>;
                    using AddressHigh = util::BitPack32::Field<Reserved0::Next,    3, u32>;
                    using Reserved1   = util::BitPack32::Field<AddressHigh::Next,  3, u32>;
                    using AddressMid  = util::BitPack32::Field<Reserved1::Next,    4, u32>;
                    using Size        = util::BitPack32::Field<AddressMid::Next,  16, u32>;

                    /* Define fields for the second word. */
                    using AddressLow  = util::BitPack32::Field<0, BITSIZEOF(u32), u32>;

                    constexpr ALWAYS_INLINE u32 GetAddressMid(u64 address) {
                        return static_cast<u32>(address >> AddressLow::Count) & ((1u << AddressMid::Count) - 1);
                    }

                    constexpr ALWAYS_INLINE u32 GetAddressHigh(u64 address) {
                        return static_cast<u32>(address >> (AddressLow::Count + AddressMid::Count));
                    }
                private:
                    util::BitPack32 m_data[2];
                public:
                    constexpr ALWAYS_INLINE PointerDescriptor() : m_data{util::BitPack32{0}, util::BitPack32{0}} { /* ... */ }

                    ALWAYS_INLINE PointerDescriptor(const void *buffer, size_t size, s32 index) : m_data{util::BitPack32{0}, util::BitPack32{0}} {
                        const u64 address = reinterpret_cast<u64>(buffer);

                        m_data[0].Set<Index>(index);
                        m_data[0].Set<AddressHigh>(GetAddressHigh(address));
                        m_data[0].Set<AddressMid>(GetAddressMid(address));
                        m_data[0].Set<Size>(size);

                        m_data[1] = { static_cast<u32>(address) };
                    }

                    ALWAYS_INLINE PointerDescriptor(const MessageBuffer &buf, s32 index) : m_data{util::BitPack32{0}, util::BitPack32{0}} {
                        buf.Get(index, m_data, util::size(m_data));
                    }

                    constexpr ALWAYS_INLINE s32 GetIndex() const {
                        return m_data[0].Get<Index>();
                    }

                    constexpr ALWAYS_INLINE uintptr_t GetAddress() const {
                        const u64 address = (static_cast<u64>((m_data[0].Get<AddressHigh>() << AddressMid::Count) | m_data[0].Get<AddressMid>()) << AddressLow::Count) | m_data[1].Get<AddressLow>();
                        return address;
                    }

                    constexpr ALWAYS_INLINE size_t GetSize() const {
                        return m_data[0].Get<Size>();
                    }

                    constexpr ALWAYS_INLINE const util::BitPack32 *GetData() const {
                        return m_data;
                    }

                    static constexpr ALWAYS_INLINE size_t GetDataSize() {
                        return sizeof(m_data);
                    }
            };

            class ReceiveListEntry {
                private:
                    /* Define fields for the first word. */
                    using AddressLow  = util::BitPack32::Field<0, BITSIZEOF(u32), u32>;

                    /* Define fields for the packed descriptor word. */
                    using AddressHigh = util::BitPack32::Field<0,                  7, u32>;
                    using Reserved    = util::BitPack32::Field<AddressHigh::Next,  9, u32>;
                    using Size        = util::BitPack32::Field<Reserved::Next,    16, u32>;

                    constexpr ALWAYS_INLINE u32 GetAddressHigh(u64 address) {
                        return static_cast<u32>(address >> (AddressLow::Count));
                    }
                private:
                    util::BitPack32 m_data[2];
                public:
                    constexpr ALWAYS_INLINE ReceiveListEntry() : m_data{util::BitPack32{0}, util::BitPack32{0}} { /* ... */ }

                    ALWAYS_INLINE ReceiveListEntry(const void *buffer, size_t size) : m_data{util::BitPack32{0}, util::BitPack32{0}} {
                        const u64 address = reinterpret_cast<u64>(buffer);

                        m_data[0] = { static_cast<u32>(address) };

                        m_data[1].Set<AddressHigh>(GetAddressHigh(address));
                        m_data[1].Set<Size>(size);
                    }

                    ALWAYS_INLINE ReceiveListEntry(u32 a, u32 b) : m_data{util::BitPack32{a}, util::BitPack32{b}} { /* ... */ }

                    constexpr ALWAYS_INLINE uintptr_t GetAddress() const {
                        const u64 address = (static_cast<u64>(m_data[1].Get<AddressHigh>()) << AddressLow::Count) | m_data[0].Get<AddressLow>();
                        return address;
                    }

                    constexpr ALWAYS_INLINE size_t GetSize() const {
                        return m_data[1].Get<Size>();
                    }

                    constexpr ALWAYS_INLINE const util::BitPack32 *GetData() const {
                        return m_data;
                    }

                    static constexpr ALWAYS_INLINE size_t GetDataSize() {
                        return sizeof(m_data);
                    }
            };
        private:
            u32 *m_buffer;
            size_t m_size;
        public:
            constexpr ALWAYS_INLINE MessageBuffer(u32 *b, size_t sz) : m_buffer(b), m_size(sz) { /* ... */ }
            constexpr explicit ALWAYS_INLINE MessageBuffer(u32 *b) : m_buffer(b), m_size(sizeof(::ams::svc::ThreadLocalRegion::message_buffer)) { /* ... */ }

            constexpr ALWAYS_INLINE void *GetBufferForDebug() const {
                return m_buffer;
            }

            constexpr ALWAYS_INLINE size_t GetBufferSize() const {
                return m_size;
            }

            ALWAYS_INLINE void Get(s32 index, util::BitPack32 *dst, size_t count) const {
                /* Ensure that this doesn't get re-ordered. */
                __asm__ __volatile__("" ::: "memory");

                /* Get the words. */
                static_assert(sizeof(*dst) == sizeof(*m_buffer));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
                __builtin_memcpy(dst, m_buffer + index, count * sizeof(*dst));
#pragma GCC diagnostic pop
            }

            ALWAYS_INLINE s32 Set(s32 index, const util::BitPack32 *src, size_t count) const {
                /* Ensure that this doesn't get re-ordered. */
                __asm__ __volatile__("" ::: "memory");

                /* Set the words. */
                __builtin_memcpy(m_buffer + index, src, count * sizeof(*src));

                /* Ensure that this doesn't get re-ordered. */
                __asm__ __volatile__("" ::: "memory");

                return index + count;
            }

            template<typename T>
            ALWAYS_INLINE const T &GetRaw(s32 index) const {
                return *reinterpret_cast<const T *>(m_buffer + index);
            }

            template<typename T>
            ALWAYS_INLINE s32 SetRaw(s32 index, const T &val) const {
                *reinterpret_cast<const T *>(m_buffer + index) = val;
                return index + (util::AlignUp(sizeof(val), sizeof(*m_buffer)) / sizeof(*m_buffer));
            }

            ALWAYS_INLINE void GetRawArray(s32 index, void *dst, size_t len) const {
                __builtin_memcpy(dst, m_buffer + index, len);
            }

            ALWAYS_INLINE void SetRawArray(s32 index, const void *src, size_t len) const {
                __builtin_memcpy(m_buffer + index, src, len);
            }

            ALWAYS_INLINE void SetNull() const {
                this->Set(MessageHeader());
            }

            ALWAYS_INLINE s32 Set(const MessageHeader &hdr) const {
                __builtin_memcpy(m_buffer, hdr.GetData(), hdr.GetDataSize());
                return hdr.GetDataSize() / sizeof(*m_buffer);
            }

            ALWAYS_INLINE s32 Set(const SpecialHeader &spc) const {
                const s32 index = MessageHeader::GetDataSize() / sizeof(*m_buffer);
                __builtin_memcpy(m_buffer + index, spc.GetHeader(), spc.GetHeaderSize());
                return index + (spc.GetHeaderSize() / sizeof(*m_buffer));
            }

            ALWAYS_INLINE s32 SetHandle(s32 index, const ::ams::svc::Handle &hnd) const {
                static_assert(util::IsAligned(sizeof(hnd), sizeof(*m_buffer)));
                __builtin_memcpy(m_buffer + index, std::addressof(hnd), sizeof(hnd));
                return index + (sizeof(hnd) / sizeof(*m_buffer));
            }

            ALWAYS_INLINE s32 SetProcessId(s32 index, const u64 pid) const {
                static_assert(util::IsAligned(sizeof(pid), sizeof(*m_buffer)));
                __builtin_memcpy(m_buffer + index, std::addressof(pid), sizeof(pid));
                return index + (sizeof(pid) / sizeof(*m_buffer));
            }

            ALWAYS_INLINE s32 Set(s32 index, const MapAliasDescriptor &desc) const {
                __builtin_memcpy(m_buffer + index, desc.GetData(), desc.GetDataSize());
                return index + (desc.GetDataSize() / sizeof(*m_buffer));
            }

            ALWAYS_INLINE s32 Set(s32 index, const PointerDescriptor &desc) const {
                __builtin_memcpy(m_buffer + index, desc.GetData(), desc.GetDataSize());
                return index + (desc.GetDataSize() / sizeof(*m_buffer));
            }

            ALWAYS_INLINE s32 Set(s32 index, const ReceiveListEntry &desc) const {
                __builtin_memcpy(m_buffer + index, desc.GetData(), desc.GetDataSize());
                return index + (desc.GetDataSize() / sizeof(*m_buffer));
            }

            ALWAYS_INLINE s32 Set(s32 index, const u32 val) const {
                static_assert(util::IsAligned(sizeof(val), sizeof(*m_buffer)));
                __builtin_memcpy(m_buffer + index, std::addressof(val), sizeof(val));
                return index + (sizeof(val) / sizeof(*m_buffer));
            }

            ALWAYS_INLINE Result GetAsyncResult() const {
                MessageHeader hdr(m_buffer);
                MessageHeader null{};
                R_SUCCEED_IF(AMS_UNLIKELY((__builtin_memcmp(hdr.GetData(), null.GetData(), MessageHeader::GetDataSize()) != 0)));
                return m_buffer[MessageHeader::GetDataSize() / sizeof(*m_buffer)];
            }

            ALWAYS_INLINE void SetAsyncResult(Result res) const {
                const s32 index = this->Set(MessageHeader());
                const auto value = res.GetValue();
                static_assert(util::IsAligned(sizeof(value), sizeof(*m_buffer)));
                __builtin_memcpy(m_buffer + index, std::addressof(value), sizeof(value));
            }

            ALWAYS_INLINE u32 Get32(s32 index) const {
                return m_buffer[index];
            }

            ALWAYS_INLINE u64 Get64(s32 index) const {
                u64 value;
                __builtin_memcpy(std::addressof(value), m_buffer + index, sizeof(value));
                return value;
            }

            ALWAYS_INLINE u64 GetProcessId(s32 index) const {
                return this->Get64(index);
            }

            ALWAYS_INLINE ams::svc::Handle GetHandle(s32 index) const {
                static_assert(sizeof(ams::svc::Handle) == sizeof(*m_buffer));
                return ::ams::svc::Handle(m_buffer[index]);
            }

            static constexpr ALWAYS_INLINE s32 GetSpecialDataIndex(const MessageHeader &hdr, const SpecialHeader &spc) {
                AMS_UNUSED(hdr);
                return (MessageHeader::GetDataSize() / sizeof(util::BitPack32)) + (spc.GetHeaderSize() / sizeof(util::BitPack32));
            }

            static constexpr ALWAYS_INLINE s32 GetPointerDescriptorIndex(const MessageHeader &hdr, const SpecialHeader &spc) {
                return GetSpecialDataIndex(hdr, spc) + (spc.GetDataSize() / sizeof(util::BitPack32));
            }

            static constexpr ALWAYS_INLINE s32 GetMapAliasDescriptorIndex(const MessageHeader &hdr, const SpecialHeader &spc) {
                return GetPointerDescriptorIndex(hdr, spc) + (hdr.GetPointerCount() * PointerDescriptor::GetDataSize() / sizeof(util::BitPack32));
            }

            static constexpr ALWAYS_INLINE s32 GetRawDataIndex(const MessageHeader &hdr, const SpecialHeader &spc) {
                return GetMapAliasDescriptorIndex(hdr, spc) + (hdr.GetMapAliasCount() * MapAliasDescriptor::GetDataSize() / sizeof(util::BitPack32));
            }

            static constexpr ALWAYS_INLINE s32 GetReceiveListIndex(const MessageHeader &hdr, const SpecialHeader &spc) {
                if (const s32 recv_list_index = hdr.GetReceiveListOffset()) {
                    return recv_list_index;
                } else {
                    return GetRawDataIndex(hdr, spc) + hdr.GetRawCount();
                }
            }

            static constexpr ALWAYS_INLINE size_t GetMessageBufferSize(const MessageHeader &hdr, const SpecialHeader &spc) {
                /* Get the size of the plain message. */
                size_t msg_size = GetReceiveListIndex(hdr, spc) * sizeof(util::BitPack32);

                /* Add the size of the receive list. */
                const auto count = hdr.GetReceiveListCount();
                switch (count) {
                    case MessageHeader::ReceiveListCountType_None:
                        break;
                    case MessageHeader::ReceiveListCountType_ToMessageBuffer:
                        break;
                    case MessageHeader::ReceiveListCountType_ToSingleBuffer:
                        msg_size += ReceiveListEntry::GetDataSize();
                        break;
                    default:
                        msg_size += (count - MessageHeader::ReceiveListCountType_CountOffset) * ReceiveListEntry::GetDataSize();
                        break;
                }

                return msg_size;
            }
    };

    #pragma GCC pop_options

}
