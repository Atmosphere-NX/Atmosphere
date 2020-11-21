/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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

#include "xusb.h"
#include "xusb_control.h"
#include "xusb_endpoint.h"

#include<array>

extern "C" {
#include "../lib/log.h"
#include "../utils.h"
#include "../btn.h"
}

namespace ams {

    namespace xusb {

        namespace test {

            USB_DECLARE_STRING_DESCRIPTOR(langid, u"\x0409"); // English (US)
            USB_DECLARE_STRING_DESCRIPTOR(manu, u"Test Manufacturer");
            USB_DECLARE_STRING_DESCRIPTOR(product, u"Test Product");
            USB_DECLARE_STRING_DESCRIPTOR(serial, u"Test Serial Number");
            USB_DECLARE_STRING_DESCRIPTOR(configuration, u"Test Configuration");
            USB_DECLARE_STRING_DESCRIPTOR(interface, u"Test Interface");

            static constexpr usb::StringDescriptorIndexer<SD_langid,
                                                          SD_manu,
                                                          SD_product,
                                                          SD_serial,
                                                          SD_configuration,
                                                          SD_interface> sd_indexer;
        
            static xusb::TRB in_ring[5];
            static xusb::TRB out_ring[5];
            static char buffer[513] __attribute__((section(".dram"))) = {0};
        
            class XusbTestGadget : public Gadget {
                public:
                static constexpr usb::DeviceDescriptor device_descriptor = {
                    {
                        .bLength = sizeof(usb::DeviceDescriptor),
                        .bDescriptorType = usb::DescriptorType::DEVICE,
                    },
                    {
                        .bcdUSB = 0x210,
                        .bDeviceClass = 0,
                        .bDeviceSubClass = 0,
                        .bDeviceProtocol = 0,
                        .bMaxPacketSize0 = 64,
                        .idVendor = 0x1209,
                        .idProduct = 0x8b00,
                        .bcdDevice = 0x100,
                        .iManufacturer = sd_indexer.manu,
                        .iProduct = sd_indexer.product,
                        .iSerialNumber = sd_indexer.serial,
                        .bNumConfigurations = 1,
                    }
                };


                static constexpr usb::DeviceQualifierDescriptor device_qualifier_descriptor = {
                    {
                        .bLength = sizeof(usb::DeviceQualifierDescriptor),
                        .bDescriptorType = usb::DescriptorType::DEVICE_QUALIFIER,
                    },
                    {
                        .bcdUSB = 0x210,
                        .bDeviceClass = 0,
                        .bDeviceSubClass = 0,
                        .bDeviceProtocol = 0,
                        .bMaxPacketSize0 = 64,
                        .bNumConfigurations = 1,
                        .bReserved = 0,
                    }
                };
            
                static const struct WholeConfigurationDescriptor {
                    usb::ConfigurationDescriptor configuration_descriptor = {
                        {
                            .bLength = sizeof(usb::ConfigurationDescriptor),
                            .bDescriptorType = usb::DescriptorType::CONFIGURATION,
                        },
                        {
                            .wTotalLength = sizeof(WholeConfigurationDescriptor),
                            .bNumInterfaces = 1,
                            .bConfigurationValue = 1,
                            .iConfiguration = 4,
                            .bmAttributes = 0x80,
                            .bMaxPower = 0,
                        }
                    };
                    usb::InterfaceDescriptor interface_descriptor {
                        {
                            .bLength = sizeof(usb::InterfaceDescriptor),
                            .bDescriptorType = usb::DescriptorType::INTERFACE,
                        },
                        {
                            .bInterfaceNumber = 0,
                            .bAlternateSetting = 0,
                            .bNumEndpoints = 2,
                            .bInterfaceClass = 0xff,
                            .bInterfaceSubClass = 0xff,
                            .bInterfaceProtocol = 0xff,
                            .iInterface = 5,
                        }
                    };
                    usb::EndpointDescriptor endpoint_in_descriptor {
                        {
                            .bLength = sizeof(usb::EndpointDescriptor),
                            .bDescriptorType = usb::DescriptorType::ENDPOINT,
                        },
                        {
                            .bEndpointAddress = 0x81,
                            .bmAttributes = 0x2,
                            .wMaxPacketSize = 512,
                            .bInterval = 1,
                        }
                    };
                    usb::EndpointDescriptor endpoint_out_descriptor {
                        {
                            .bLength = sizeof(usb::EndpointDescriptor),
                            .bDescriptorType = usb::DescriptorType::ENDPOINT,
                        },
                        {
                            .bEndpointAddress = 0x01,
                            .bmAttributes = 0x2,
                            .wMaxPacketSize = 512,
                            .bInterval = 1,
                        }
                    };
                } __attribute__((packed)) whole_configuration_descriptor;

                virtual Result GetDeviceDescriptor(uint8_t index, const usb::DeviceDescriptor **descriptor, uint16_t *length) override {
                    R_UNLESS(index == 0, xusb::ResultInvalidDescriptorIndex());

                    *descriptor = &device_descriptor;
                    *length = sizeof(device_descriptor);
        
                    return ResultSuccess();
                }
    
                virtual Result GetDeviceQualifierDescriptor(uint8_t index, const usb::DeviceQualifierDescriptor **descriptor, uint16_t *length) override {
                    R_UNLESS(index == 0, xusb::ResultInvalidDescriptorIndex());

                    *descriptor = &device_qualifier_descriptor;
                    *length = sizeof(device_qualifier_descriptor);
        
                    return ResultSuccess();
                }
            
                virtual Result GetConfigurationDescriptor(uint8_t index, const usb::ConfigurationDescriptor **descriptor, uint16_t *length) override {
                    R_UNLESS(index == 0, xusb::ResultInvalidDescriptorIndex());

                    *descriptor = &whole_configuration_descriptor.configuration_descriptor;
                    *length = sizeof(whole_configuration_descriptor);
        
                    return ResultSuccess();
                }

                virtual Result GetBOSDescriptor(const usb::BOSDescriptor **descriptor, uint16_t *length) override {
                    return ResultUnknownDescriptorType();
                }

                virtual Result GetStringDescriptor(uint8_t index, uint16_t language, const usb::CommonDescriptor **descriptor, uint16_t *length) override {
                    R_UNLESS(language == 0x0409 || (language == 0 && index == 0), xusb::ResultInvalidDescriptorIndex());

                    R_UNLESS(sd_indexer.GetStringDescriptor(index, language, descriptor, length), xusb::ResultInvalidDescriptorIndex());
                    
                    return ResultSuccess();
                }

                static constexpr auto out = xusb::endpoints[xusb::EpAddr {1, xusb::Dir::Out}];
                static constexpr auto in = xusb::endpoints[xusb::EpAddr {1, xusb::Dir::In}];
            
                virtual Result SetConfiguration(uint16_t configuration) override {
                    R_UNLESS(configuration <= 1, xusb::ResultInvalidConfiguration());

                    xusb::endpoints.ClearNonControlEndpoints();

                    out.Initialize(whole_configuration_descriptor.endpoint_out_descriptor, out_ring, std::size(out_ring));
                    in.Initialize(whole_configuration_descriptor.endpoint_in_descriptor, in_ring, std::size(in_ring));

                    print(SCREEN_LOG_LEVEL_DEBUG, "configured and enabled endpoints!\n");
                
                    return ResultSuccess();
                }

                virtual Result Deconfigure() override {
                    out.Disable();
                    in.Disable();

                    return ResultSuccess();
                }

                TRB *last_out_trb = nullptr;
                TRB *last_in_trb = nullptr;

                Result SubmitOutTRB() {
                    TRBBorrow trb;
                    
                    R_TRY(out.EnqueueTRB(&trb));
                    
                    print(SCREEN_LOG_LEVEL_DEBUG, "submitting out trb %p\n", &*trb);
                    trb->transfer.InitializeNormal((void*) buffer, sizeof(buffer)-1);
                    trb->transfer.interrupt_on_completion = true;
                    trb->transfer.interrupt_on_short_packet = true;
                    trb.Release();

                    last_out_trb = &*trb;

                    out.RingDoorbell();

                    return ResultSuccess();
                }

                Result SubmitInTRB(size_t length) {
                    TRBBorrow trb;
                    
                    R_TRY(in.EnqueueTRB(&trb));
                    
                    print(SCREEN_LOG_LEVEL_DEBUG, "submitting in trb %p\n", &*trb);
                    trb->transfer.InitializeNormal((void*) buffer, length);
                    trb->transfer.interrupt_on_completion = true;
                    trb->transfer.interrupt_on_short_packet = true;
                    trb.Release();

                    last_in_trb = &*trb;

                    in.RingDoorbell();

                    return ResultSuccess();
                }
            
                virtual void PostConfigure() override {
                    SubmitOutTRB();
                }

                virtual void ProcessTransferEvent(TransferEventTRB *event, TRBBorrow transfer) override {
                    print(SCREEN_LOG_LEVEL_DEBUG, "gadget got transfer event for ep %d\n", event->ep_id);

                    if (event->ep_id == out.GetIndex()) {
                        if (&*transfer == last_out_trb) {
                            print(SCREEN_LOG_LEVEL_DEBUG, "  out transfer completed: %d bytes remain\n", event->trb_transfer_length);
                            buffer[transfer->transfer.transfer_length - event->trb_transfer_length] = 0;
                            print(SCREEN_LOG_LEVEL_DEBUG, "  \"%s\"\n", buffer);

                            SubmitInTRB(transfer->transfer.transfer_length - event->trb_transfer_length);
                        } else {
                            print(SCREEN_LOG_LEVEL_DEBUG, "  got completion for unexpected OUT trb (got %p, expected %p)\n", &*transfer, last_out_trb);
                        }
                    } else if (event->ep_id == in.GetIndex()) {
                        if (&*transfer == last_in_trb) {
                            SubmitOutTRB();
                        } else {
                            print(SCREEN_LOG_LEVEL_DEBUG, "  got completion for unexpected IN trb (got %p, expected %p)\n", &*transfer, last_out_trb);
                        }
                    }
                }
            };

            static XusbTestGadget test_gadget;

            Gadget &GetTestGadget() {
                return test_gadget;
            }

        } // namespace test
    
    } // namespace xusb

} // namespace ams
