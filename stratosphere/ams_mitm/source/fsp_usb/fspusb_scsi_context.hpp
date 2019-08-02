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
#include <switch.h>
#include <malloc.h>
#include <memory> 
#include "fspusb_scsi_command.hpp"

class SCSIDevice
{
    public:
        uint8_t *usb_bounce_buffer_a;
        uint8_t *usb_bounce_buffer_b;
        uint8_t *usb_bounce_buffer_c;
        UsbHsClientIfSession *client;
        UsbHsClientEpSession *in_endpoint;
        UsbHsClientEpSession *out_endpoint;

        SCSIDevice(){}
        SCSIDevice(UsbHsClientIfSession *client_, UsbHsClientEpSession *in_endpoint_, UsbHsClientEpSession *out_endpoint_) {
            usb_bounce_buffer_a = (uint8_t*) memalign(0x1000, 0x1000);
            usb_bounce_buffer_b = (uint8_t*) memalign(0x1000, 0x1000);
            usb_bounce_buffer_c = (uint8_t*) memalign(0x1000, 0x1000);
            client = client_;
            in_endpoint = in_endpoint_;
            out_endpoint = out_endpoint_;
        }

        SCSICommandStatus read_csw();
        void push_cmd(SCSICommand *c);
        SCSICommandStatus transfer_cmd(SCSICommand *c, uint8_t *buffer, size_t buffer_size);
};


class MBRPartition
{
    public:
        uint8_t active;
        uint8_t partition_type;
        uint32_t start_lba;
        uint32_t num_sectors;

        static MBRPartition from_bytes(uint8_t *entry)
        {
            MBRPartition p;
            p.active = entry[0];
            // 1 - 3 are chs start, skip them
            p.partition_type = entry[4];
            // 5 - 7 are chs end, skip them
            memcpy(&p.start_lba, &entry[8], 4);
            memcpy(&p.num_sectors, &entry[12], 4);
            return p;
        }
};

class SCSIBlock;

class SCSIBlockPartition
{
    public:
        SCSIBlock *block;
        uint32_t start_lba_offset;
        uint32_t lba_size;
        SCSIBlockPartition() {}
        SCSIBlockPartition(SCSIBlock *block_, MBRPartition partition_info);
        int read_sectors(uint8_t *buffer, uint32_t sector_offset, uint32_t num_sectors);
        int write_sectors(const uint8_t *buffer, uint32_t sector_offset, uint32_t num_sectors);
};

class SCSIBlock
{
    public:
        uint64_t capacity;
        uint32_t block_size;

        SCSIBlockPartition partitions[4];
        MBRPartition partition_infos[4];

        std::shared_ptr<SCSIDevice> device;

        SCSIBlock(){}
        SCSIBlock(std::shared_ptr<SCSIDevice> device_);
        int read_sectors(uint8_t *buffer, uint32_t sector_offset, uint32_t num_sectors);
        int write_sectors(const uint8_t *buffer, uint32_t sector_offset, uint32_t num_sectors);
};