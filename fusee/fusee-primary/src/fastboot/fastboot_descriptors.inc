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

namespace ams {

    namespace fastboot {

        namespace descriptors {

            USB_DECLARE_STRING_DESCRIPTOR(langid, u"\x0409"); // English (US)
            USB_DECLARE_STRING_DESCRIPTOR(manu, u"Atmosphère");
            USB_DECLARE_STRING_DESCRIPTOR(product, u"Fusée Fastboot Gadget");
            USB_DECLARE_STRING_DESCRIPTOR(serial, u"Serial Number");
            USB_DECLARE_STRING_DESCRIPTOR(configuration, u"Fastboot");
            USB_DECLARE_STRING_DESCRIPTOR(interface, u"Fastboot");

            static constexpr usb::StringDescriptorIndexer<SD_langid,
                                                          SD_manu,
                                                          SD_product,
                                                          SD_serial,
                                                          SD_configuration,
                                                          SD_interface> sd_indexer;

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
                    .idProduct = 0x8b01,
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
            
            static constexpr struct WholeConfigurationDescriptor {
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
                        .bInterfaceClass = 0xff, // Vendor Specific Class.
                        .bInterfaceSubClass = 0x42, // Fastboot.
                        .bInterfaceProtocol = 0x03, // Fastboot.
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

            static const uint8_t bMS_VendorCode = 37; /* chosen by fair dice roll */

            /* This helps Windows figure out that it should use the ADB driver with our device without having to get a driver package signed. */
            static constexpr struct {
                usb::ms::os20::DescriptorSetHeader descriptor_set_header1 = {
                    .wLength = sizeof(usb::ms::os20::DescriptorSetHeader),
                    .wDescriptorType = usb::ms::os20::DescriptorType::SetHeaderDescriptor,
                    .dwWindowsVersion = usb::ms::NTDDI_WIN8_1,
                    .wTotalLength = sizeof(*this),
                };
                
                /* https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/automatic-installation-of-winusb#how-to-configure-a-winusb-device */
                usb::ms::os20::FeatureCompatibleID feature_compatible_id = usb::ms::os20::FeatureCompatibleID("WINUSB", "");
                
                /* https://android.googlesource.com/platform/development/+/8c78ba643862731c603677284ae88089a959cc52/host/windows/usb/api/adb_api_extra.h#64 */
                usb::ms::os20::FeatureRegistryProperty<usb::ms::os20::PropertyDataType::REG_SZ, u"DeviceInterfaceGUID", u"{F72FE0D4-CBCB-407d-8814-9ED673D0DD6B}"> feature_registry_device_interface_guid;
            } __attribute__((packed)) ms_os20_descriptor;
            
            static constexpr auto bos_container = usb::make_bos_descriptor_container(usb::caps::USB_2_0_EXTENSION {
                    {
                        {
                            .bLength = sizeof(usb::caps::USB_2_0_EXTENSION),
                            .bDescriptorType = usb::DescriptorType::DEVICE_CAPABILITY,
                        },
                        {
                            .bDevCapabilityType = usb::DeviceCapabilityType::USB_2_0_EXTENSION,
                        }
                    },
                    {
                        .bmAttributes = 0,
                    }
                }, usb::caps::SUPERSPEED {
                    {
                        {
                            .bLength = sizeof(usb::caps::SUPERSPEED),
                            .bDescriptorType = usb::DescriptorType::DEVICE_CAPABILITY,
                        },
                        {
                            .bDevCapabilityType = usb::DeviceCapabilityType::SUPERSPEED_USB,
                        }
                    },
                    {
                        .bmAttributes = 0,
                        .wSpeedsSupported = 0b1111,
                        .bFunctionalitySupport = 1,
                        .bU1DevExitLat = 0,
                        .wU2DevExitLat = 0
                    }
                }, usb::caps::make_platform_capability(
                    /* Indicate to Windows hosts that they can retrieve their OS-specific descriptor through a special vendor-specific request. */
                    usb::ms::DescriptorSetInformation {
                        .dwWindowsVersion = usb::ms::NTDDI_WIN8_1,
                        .wMSOSDescriptorSetTotalLength = sizeof(ms_os20_descriptor),
                        .bMS_VendorCode = bMS_VendorCode,
                        .bAltEnumCode = 0,
                    })
                );
            
        } // namespace descriptors
    
    } // namespace fastboot

} // namespace ams
