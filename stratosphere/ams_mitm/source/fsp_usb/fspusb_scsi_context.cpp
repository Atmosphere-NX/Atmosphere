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

#include <switch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstdarg>
#include "fspusb_scsi_context.hpp"

SCSICommandStatus SCSIDevice::read_csw() {
    uint32_t num_bytes;
    Result res = usbHsEpPostBuffer(out_endpoint, usb_bounce_buffer_c, 0x10, &num_bytes);
    if(R_FAILED(res))
    {
        printf("read_csw usb fail (handle: %d) %08x\n", out_endpoint->s.handle, res);
    }
    if(num_bytes != 13)
    {
        printf("read_csw usb short read of length %i\n", num_bytes);
    }

    return SCSICommandStatus(usb_bounce_buffer_c);
}

void SCSIDevice::push_cmd(SCSICommand *c) {
    // Push a 31 byte command.
    uint32_t num_bytes;
    memset(usb_bounce_buffer_a, 0, 0x1000);
    c->to_bytes(usb_bounce_buffer_a);

    Result res = usbHsEpPostBuffer(in_endpoint, usb_bounce_buffer_a, 31, &num_bytes);
    if(R_FAILED(res))
    {
        printf("push_cmd usb fail %08x\n", res);
        return;
    }
    if(num_bytes != 31)
    {
        printf("push_cmd usb fail short write of %i bytes????\n", num_bytes);
        return;
    }
}

SCSICommandStatus SCSIDevice::transfer_cmd(SCSICommand *c, uint8_t *buffer, size_t buffer_size) {
    push_cmd(c);

    uint32_t transfer_length = c->data_transfer_length;
    uint32_t transferred = 0;
    uint32_t total_transferred = 0;
    
    if(buffer_size < transfer_length)
    {
        printf("Buffer too small!!!!\n");
    }

    Result res;

    if(transfer_length > 0)
    {
        if(c->direction == SCSIDirection::In)
        {
            while(total_transferred < transfer_length)
            {
                memset(usb_bounce_buffer_b, 0, 0x1000);
                res = usbHsEpPostBuffer(out_endpoint, usb_bounce_buffer_b, transfer_length - total_transferred, &transferred);

                if(R_FAILED(res))
                {
                    printf("usbHsEpPostBuffer failed %08x\n", res);
                }

                if(transferred == 13)
                {
                    uint32_t signature;
                    memcpy(&signature, usb_bounce_buffer_b, 4);
                    if(signature == CSW_SIGNATURE)
                    {
                        // We weren't expecting a CSW!
                        // But we got one anyway!
                        return SCSICommandStatus(usb_bounce_buffer_b);
                    }
                }

                memcpy(buffer + total_transferred, usb_bounce_buffer_b, transferred);
                total_transferred += transferred;
            }
        }
        else
        {
            while(total_transferred < transfer_length)
            {
                memcpy(usb_bounce_buffer_b, buffer + total_transferred, transfer_length - total_transferred);
                res = usbHsEpPostBuffer(in_endpoint, usb_bounce_buffer_b, transfer_length - total_transferred, &transferred);
                if(R_FAILED(res))
                {
                    printf("usbHsEpPostBuffer failed %08x\n", res);
                }
                total_transferred += transferred;
            }
        }
    }

    SCSICommandStatus w = read_csw();
    if(w.tag != c->tag)
    {
        // ???
    }

    if(w.status != COMMAND_PASSED)
    {
        // ???
    }

    return w;
}

SCSIBlock::SCSIBlock(std::shared_ptr<SCSIDevice> device_) {
    device = device_;
    SCSIInquiryCommand inquiry(36);
    SCSITestUnitReadyCommand test_unit_ready;
    SCSIReadCapacityCommand read_capacity;
    uint8_t inquiry_response[36];
    SCSICommandStatus status = device->transfer_cmd(&inquiry, inquiry_response, 36);
    status = device->transfer_cmd(&test_unit_ready, NULL, 0);
    uint8_t read_capacity_response[8];
    uint32_t size_lba;
    uint32_t lba_bytes;
    status = device->transfer_cmd(&read_capacity, read_capacity_response, 8);
    memcpy(&size_lba, &read_capacity_response[0], 4);
    size_lba = __builtin_bswap32(size_lba);
    memcpy(&lba_bytes, &read_capacity_response[4], 4);
    lba_bytes = __builtin_bswap32(lba_bytes);
    capacity = size_lba * lba_bytes;
    block_size = lba_bytes;
    uint8_t mbr[0x200];
    read_sectors(mbr, 0, 1);
    partition_infos[0] = MBRPartition::from_bytes(&mbr[0x1be]);
    partition_infos[1] = MBRPartition::from_bytes(&mbr[0x1ce]);
    partition_infos[2] = MBRPartition::from_bytes(&mbr[0x1de]);
    partition_infos[3] = MBRPartition::from_bytes(&mbr[0x1ee]);
    for(int i = 0; i < 4; i++)
    {
        MBRPartition p = partition_infos[i];
        if(p.partition_type != 0)
        {
            partitions[i] = SCSIBlockPartition(this, p);
        }
    }
}

int SCSIBlock::read_sectors(uint8_t *buffer, uint32_t sector_offset, uint32_t num_sectors) {
    SCSIRead10Command read_ten(sector_offset, block_size, num_sectors);
    SCSICommandStatus status = device->transfer_cmd(&read_ten, buffer, num_sectors * block_size);
    return num_sectors;
}

int SCSIBlock::write_sectors(const uint8_t *buffer, uint32_t sector_offset, uint32_t num_sectors) {
    SCSIWrite10Command write_ten(sector_offset, block_size, num_sectors);
    SCSICommandStatus status = device->transfer_cmd(&write_ten, (uint8_t*)buffer, num_sectors * block_size);
    return num_sectors;
}

SCSIBlockPartition::SCSIBlockPartition(SCSIBlock *block_, MBRPartition partition_info) {
    block = block_;
    start_lba_offset = partition_info.start_lba;
    lba_size = partition_info.num_sectors;
}

int SCSIBlockPartition::read_sectors(uint8_t *buffer, uint32_t sector_offset, uint32_t num_sectors) {
    // TODO: assert we don't read outside the partition
    return block->read_sectors(buffer, sector_offset + start_lba_offset, num_sectors);
}

int SCSIBlockPartition::write_sectors(const uint8_t *buffer, uint32_t sector_offset, uint32_t num_sectors) {
    // TODO: assert we don't write (!!) outside the partition
    return block->write_sectors(buffer, sector_offset + start_lba_offset, num_sectors);
}