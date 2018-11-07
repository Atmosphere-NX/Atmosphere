/*
 * Copyright (c) 2018 Atmosphère-NX
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

#include <cstdarg>
#include "tma_conn_result.hpp"
#include "tma_conn_service_ids.hpp"
#include "crc.h"

class TmaPacket {
    public:
        struct Header {
            u32 service_id;
            u32 task_id;
            u16 command;
            u8  is_continuation;
            u8  version;
            u32 body_len;
            u32 reserved[4]; /* This is where N's header ends. */
            u32 body_checksum;
            u32 header_checksum;
        };
        
        static_assert(sizeof(Header) == 0x28, "Packet::Header definition!");
        
        static constexpr u32 MaxBodySize = 0xE000;
        static constexpr u32 MaxPacketSize = MaxBodySize + sizeof(Header);
        
    private:
        std::unique_ptr<u8[]> buffer = std::make_unique<u8[]>(MaxPacketSize);
        u32 offset = 0;
        
        Header *GetHeader() const {
            return reinterpret_cast<Header *>(buffer.get());
        }
        
        u8 *GetBody(u32 ofs) const {
            return reinterpret_cast<u8 *>(buffer.get() + sizeof(Header) + ofs);
        }
    public:
        TmaPacket() {
            memset(buffer.get(), 0, MaxPacketSize);
        }
        
        /* Implicit ~TmaPacket() */
        
        /* These allow reading a packet in. */
        void CopyHeaderFrom(Header *hdr) {
            *GetHeader() = *hdr;
        }
        
        TmaConnResult CopyBodyFrom(void *body, size_t size) {
            if (size >= MaxBodySize) {
                return TmaConnResult::PacketOverflow;
            }
            
            memcpy(GetBody(0), body, size);
            
            return TmaConnResult::Success;
        }
        
        void CopyHeaderTo(void *out) {
            memcpy(out, buffer.get(), sizeof(Header));
        }
        
        void CopyBodyTo(void *out) const {
            memcpy(out, buffer.get() + sizeof(Header), GetBodyLength());
        }
        
        bool IsHeaderValid() {
            Header *hdr = GetHeader();
            return crc32_arm64_le_hw(reinterpret_cast<const u8 *>(hdr), sizeof(*hdr) - sizeof(hdr->header_checksum)) == hdr->header_checksum;
        }
        
        bool IsBodyValid() const {
            const u32 body_len = GetHeader()->body_len;
            if (body_len == 0) {
                return GetHeader()->body_checksum == 0;
            } else {
                return crc32_arm64_le_hw(GetBody(0), body_len) == GetHeader()->body_checksum;
            }
        }
        
        void SetChecksums() {
            Header *hdr = GetHeader();
            if (hdr->body_len) {
                hdr->body_checksum = crc32_arm64_le_hw(GetBody(0), hdr->body_len);
            } else {
                hdr->body_checksum = 0;
            }
            hdr->header_checksum = crc32_arm64_le_hw(reinterpret_cast<const u8 *>(hdr), sizeof(*hdr) - sizeof(hdr->header_checksum));
        }
        
        u32 GetBodyLength() const {
            return GetHeader()->body_len;
        }
        
        u32 GetLength() const {
            return GetBodyLength() + sizeof(Header);
        }
        
        u32 GetBodyAvailableLength() const {
            return MaxPacketSize - this->offset;
        }
        
        void SetServiceId(TmaService srv) {
            GetHeader()->service_id = static_cast<u32>(srv);
        }
        
        TmaService GetServiceId() const {
            return static_cast<TmaService>(GetHeader()->service_id);
        }
        
        void SetTaskId(u32 id) {
            GetHeader()->task_id = id;
        }
        
        u32 GetTaskId() const {
            return GetHeader()->task_id;
        }
        
        void SetCommand(u16 cmd) {
            GetHeader()->command = cmd;
        }
        
        u16 GetCommand() const {
            return GetHeader()->command;
        }
        
        void SetContinuation(bool c) {
            GetHeader()->is_continuation = c ? 1 : 0;
        }
        
        bool GetContinuation() const {
            return GetHeader()->is_continuation == 1;
        }
        
        void SetVersion(u8 v) {
            GetHeader()->version = v;
        }
        
        u8 GetVersion() const {
            return GetHeader()->version;
        }
        
        void ClearOffset() {
            this->offset = 0;
        }
        
        TmaConnResult Write(const void *data, size_t size) {
            if (size > GetBodyAvailableLength()) {
                return TmaConnResult::PacketOverflow;
            }
            
            memcpy(GetBody(this->offset), data, size);
            this->offset += size;
            GetHeader()->body_len = this->offset;

            return TmaConnResult::Success;
        }
        
        TmaConnResult Read(void *data, size_t size) {
            if (size > GetBodyAvailableLength()) {
                return TmaConnResult::PacketOverflow;
            }
            
            memcpy(data, GetBody(this->offset), size);
            this->offset += size;
            
            return TmaConnResult::Success;
        }
        
        template<typename T>
        TmaConnResult Write(const T &t) {
            return Write(&t, sizeof(T));
        }
        
        template<typename T>
        TmaConnResult Read(T &t) {
            return Read(&t, sizeof(T));
        }
        
        TmaConnResult WriteString(const char *s) {
            return Write(s, strlen(s) + 1);
        }
        
        size_t WriteFormat(const char *format, ...) {
            va_list va_arg;
            va_start(va_arg, format);
            const size_t available = GetBodyAvailableLength();
            const int written = vsnprintf(reinterpret_cast<char *>(GetBody(this->offset)), available, format, va_arg);
            
            size_t total_written;
            if (static_cast<size_t>(written) < available) {
                this->offset += written;
                *GetBody(this->offset++) = 0;
                total_written = written + 1;
            } else {
                this->offset += available;
                total_written = available;
            }
            
            GetHeader()->body_len = this->offset;
            return total_written;
        }
        
        TmaConnResult ReadString(char *buf, size_t buf_size, size_t *out_size) {
            TmaConnResult res = TmaConnResult::Success;
            
            size_t available = GetBodyAvailableLength();
            size_t ofs = 0;
            while (ofs < buf_size) {
                if (ofs >= available) {
                    res = TmaConnResult::PacketOverflow;
                    break;
                }
                if (ofs == buf_size) {
                    res = TmaConnResult::BufferOverflow;
                    break;
                }
                
                buf[ofs] = static_cast<char>(*GetBody(this->offset++));
                
                if (buf[ofs++] == '\x00') {
                    break;
                }
            }
            
            /* Finish reading the string if the user buffer is too small. */
            if (res == TmaConnResult::BufferOverflow) {
                u8 cur = *GetBody(this->offset);
                while (cur != 0) {
                    if (ofs >= available) {
                        res = TmaConnResult::PacketOverflow;
                        break;
                    }
                    cur = *GetBody(this->offset++);
                    ofs++;
                }
            }
            
            if (out_size != nullptr) {
                *out_size = ofs;
            }
            
            return res;
        }
};
