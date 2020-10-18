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

#pragma once

#include<stdint.h>
#include<stddef.h>
#include<string.h>

namespace usb {

    enum class DescriptorType : uint8_t {
        DEVICE = 1,
        CONFIGURATION = 2,
        STRING = 3,
        INTERFACE = 4,
        ENDPOINT = 5,
        DEVICE_QUALIFIER = 6,
        OTHER_SPEED_CONFIGURATION = 7,
        INTERFACE_POWER = 8,
        OTG = 9,
        DEBUG = 10,
        INTERFACE_ASSOCIATION = 11,
        BOS = 15,
        DEVICE_CAPABILITY = 16,
        SUPERSPEED_USB_ENDPOINT_COMPANION = 48,
        SUPERSPEEDPLUS_ISOCHRONOUS_ENDPOINT_COMPANION = 49,
    };

    enum class EndpointFeatureSelector : uint8_t {
        ENDPOINT_HALT = 0,
    };

    enum class DeviceCapabilityType : uint8_t {
        WIRELESS_USB = 0x1,
        USB_2_0_EXTENSION = 0x2,
        SUPERSPEED_USB = 0x3,
        CONTAINER_ID = 0x4,
        PLATFORM = 0x5,
        POWER_DELIVERY_CAPABILITY = 0x6,
        BATTERY_INFO_CAPABILITY = 0x7,
        PD_CONSUMER_PORT_CAPABILITY = 0x8,
        PD_PROVIDER_PORT_CAPABILITY = 0x9,
        SUPERSPEED_PLUS = 0xa,
        PRECISION_TIME_MEASUREMENT = 0xb,
        WIRELESS_USB_EXT = 0xc,
        BILLBOARD = 0xd,
        AUTHENTICATION = 0xe,
        BILLBOARD_EX = 0xf,
        CONFIGURATION_SUMMARY = 0x10,
    };
    
    enum class EndpointType : uint8_t {
        Control = 0,
        Isochronous = 1,
        Bulk = 2,
        Interrupt = 3,
    };
    
    struct CommonDescriptor {
        uint8_t bLength;
        DescriptorType bDescriptorType;
    } __attribute__((packed));
    
    struct DeviceDescriptor : public CommonDescriptor {
        struct {
            uint16_t bcdUSB;
            uint8_t bDeviceClass;
            uint8_t bDeviceSubClass;
            uint8_t bDeviceProtocol;
            uint8_t bMaxPacketSize0;
            uint16_t idVendor;
            uint16_t idProduct;
            uint16_t bcdDevice;
            uint8_t iManufacturer;
            uint8_t iProduct;
            uint8_t iSerialNumber;
            uint8_t bNumConfigurations;
        } __attribute__((packed));
    } __attribute__((packed));

    struct DeviceQualifierDescriptor : public CommonDescriptor {
        struct {
            uint16_t bcdUSB;
            uint8_t bDeviceClass;
            uint8_t bDeviceSubClass;
            uint8_t bDeviceProtocol;
            uint8_t bMaxPacketSize0;
            uint8_t bNumConfigurations;
            uint8_t bReserved;
        } __attribute__((packed));
    } __attribute__((packed));

    struct ConfigurationDescriptor : public CommonDescriptor {
        struct {
            uint16_t wTotalLength;
            uint8_t bNumInterfaces;
            uint8_t bConfigurationValue;
            uint8_t iConfiguration;
            uint8_t bmAttributes;
            uint8_t bMaxPower;
        } __attribute__((packed));
    } __attribute__((packed));

    static_assert(sizeof(ConfigurationDescriptor) == 9);
    
    struct InterfaceDescriptor : public CommonDescriptor {
        struct {
            uint8_t bInterfaceNumber;
            uint8_t bAlternateSetting;
            uint8_t bNumEndpoints;
            uint8_t bInterfaceClass;
            uint8_t bInterfaceSubClass;
            uint8_t bInterfaceProtocol;
            uint8_t iInterface;
        } __attribute__((packed));
    } __attribute__((packed));

    static_assert(sizeof(InterfaceDescriptor) == 9);

    struct EndpointDescriptor : public CommonDescriptor {
        struct {
            uint8_t bEndpointAddress;
            uint8_t bmAttributes;
            uint16_t wMaxPacketSize;
            uint8_t bInterval;
        } __attribute__((packed));

        EndpointType GetEndpointType() const {
            return (EndpointType) (bmAttributes & 0b11);
        }
    } __attribute__((packed));

    static_assert(sizeof(EndpointDescriptor) == 7);
    
    namespace detail {
        
        template<size_t N>
        struct FixedString {
            constexpr FixedString(char16_t const* s) {
                for (size_t i = 0; i < N; i++) {
                    buffer[i] = s[i];
                }
            }
            static const size_t length = N;
            char16_t buffer[N + 1]{};
        };
        template<size_t N> FixedString(char16_t const (&)[N]) -> FixedString<N - 1>;
        
    } // namespace detail
    
    template <detail::FixedString string>
    struct StringDescriptor : public CommonDescriptor {
        inline constexpr StringDescriptor() : bString {} {
            this->bLength = (string.length * 2) + 2;
            this->bDescriptorType = DescriptorType::STRING;

            for (size_t i = 0; i < string.length; i++) {
                this->bString[i] = string.buffer[i];
            }
        }

        char16_t bString[string.length];
    } __attribute__((packed));

    namespace detail {
    
        template<size_t I, typename First, typename... T>
        struct StringDescriptorIndexer : public First::template WithIndex<I>, public StringDescriptorIndexer<I + 1, T...> {
            using ThisDescriptor = First::template WithIndex<I>;
            using NextIndexer = StringDescriptorIndexer<I + 1, T...>;

            bool GetStringDescriptor(uint8_t index, uint16_t language, const CommonDescriptor **descriptor, uint16_t *length) const {
                if (index == I) {
                    *descriptor = &ThisDescriptor::_descriptor;
                    *length = sizeof(ThisDescriptor::_descriptor);
                    return true;
                } else {
                    return NextIndexer::GetStringDescriptor(index, language, descriptor, length);
                }
            }
        };

        template<size_t I, typename First>
        struct StringDescriptorIndexer<I, First> : public First::template WithIndex<I> {
            using ThisDescriptor = First::template WithIndex<I>;

            bool GetStringDescriptor(uint8_t index, uint16_t language, const CommonDescriptor **descriptor, uint16_t *length) const {
                if (index == I) {
                    *descriptor = &ThisDescriptor::_descriptor;
                    *length = sizeof(ThisDescriptor::_descriptor);
                    return true;
                } else {
                    return false;
                }
            }
        };

    } // namespace detail

    template<typename... T>
    using StringDescriptorIndexer = detail::StringDescriptorIndexer<0, T...>;

    /* The wrapper type is here because we can't pass templates as template
     * parameters directly. */
    
#define USB_DECLARE_STRING_DESCRIPTOR(id, english_value) struct SD_##id { \
        template<size_t I>                                              \
        struct WithIndex {                                              \
            static constexpr ::usb::StringDescriptor<english_value> _descriptor = {}; \
            static constexpr size_t id = I;                             \
        };                                                              \
    }

    struct BOSDescriptor : public CommonDescriptor {
        struct {
            uint16_t wTotalLength;
            uint8_t bNumDeviceCaps;
        } __attribute__((packed));
    } __attribute__((packed));

    struct DeviceCapabilityDescriptor : public CommonDescriptor {
        struct {
            DeviceCapabilityType bDevCapabilityType;
        } __attribute__((packed));
    } __attribute__((packed));

    namespace caps {

        struct USB_2_0_EXTENSION : public DeviceCapabilityDescriptor {
            struct {
                uint32_t bmAttributes;
            } __attribute__((packed));
        } __attribute__((packed));

        struct SUPERSPEED : public DeviceCapabilityDescriptor {
            struct {
                uint8_t bmAttributes;
                uint16_t wSpeedsSupported;
                uint8_t bFunctionalitySupport;
                uint8_t bU1DevExitLat;
                uint16_t wU2DevExitLat;
            } __attribute__((packed));
        } __attribute__((packed));

        template<typename PlatformCapability>
        struct PLATFORM : public DeviceCapabilityDescriptor {
            uint8_t bReserved;
            uint8_t PlatformCapabilityUUID[16];
            PlatformCapability platform_cap;

            constexpr PLATFORM(PlatformCapability platform_cap) :
                DeviceCapabilityDescriptor({
                        {
                            .bLength = sizeof(*this),
                            .bDescriptorType = usb::DescriptorType::DEVICE_CAPABILITY,
                        },
                        {
                            .bDevCapabilityType = usb::DeviceCapabilityType::PLATFORM
                        }
                    }),
                bReserved(0),
                platform_cap(platform_cap) {
                static_assert(sizeof(PlatformCapability::UUID[0]) == 1);
                static_assert(sizeof(PlatformCapability::UUID) == sizeof(PlatformCapabilityUUID));
                
                for (size_t i = 0; i < sizeof(PlatformCapability::UUID); i++) {
                    PlatformCapabilityUUID[i] = PlatformCapability::UUID[i];
                }
            }
        } __attribute__((packed));

        template<typename T>
        constexpr PLATFORM<T> make_platform_capability(T cap) {
            return PLATFORM<T>(cap);
        }
        
    } // namespace caps

    namespace ms {
        inline const uint32_t NTDDI_WIN8_1 = 0x06030000;
        inline const uint8_t MS_OS_20_DESCRIPTOR_INDEX = 0x7;
        
        struct DescriptorSetInformation {
            static inline const uint8_t UUID[16] = {0xDF, 0x60, 0xDD, 0xD8, 0x89, 0x45, 0xC7, 0x4C, 0x9C, 0xD2, 0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F};
            
            uint32_t dwWindowsVersion;
            uint16_t wMSOSDescriptorSetTotalLength;
            uint8_t bMS_VendorCode;
            uint8_t bAltEnumCode;
        } __attribute__((packed));

        namespace os20 {
        
            enum class DescriptorType : uint16_t {
                SetHeaderDescriptor = 0,
                SubsetHeaderConfiguration = 1,
                SubsetHeaderFunction = 2,
                FeatureCompatibleID = 3,
                FeatureRegProperty = 4,
                FeatureMinResumeTime = 5,
                FeatureModelID = 6,
                FeatureCCGPDevice = 7,
                FeatureVendorRevision = 8
            };

            struct DescriptorSetHeader {
                uint16_t wLength;
                DescriptorType wDescriptorType;
                uint32_t dwWindowsVersion;
                uint16_t wTotalLength;
            } __attribute__((packed));

            /*
            struct ConfigurationSubsetHeader {
                uint16_t wLength;
                uint16_t wDescriptorType;
                uint8_t bConfigurationValue;
                uint8_t bReserved;
                uint16_t wTotalLength;
            } __attribute__((packed));

            struct FunctionSubsetHeader {
                uint16_t wLength;
                uint16_t wDescriptorType;
                uint8_t bFirstInterface;
                uint8_t bReserved;
                uint16_t wSubsetLength;
            } __attribute__((packed));
            */

            struct FeatureCompatibleID {
                uint16_t wLength = sizeof(*this);
                DescriptorType wDescriptorType = DescriptorType::FeatureCompatibleID;
                uint8_t CompatibleID[8] = {0};
                uint8_t SubCompatibleID[8] = {0};

                constexpr FeatureCompatibleID(const char CompatibleID[], const char SubCompatibleID[]) {
                    for (size_t i = 0; i < sizeof(this->CompatibleID) && CompatibleID[i] != '\0'; i++) {
                        this->CompatibleID[i] = CompatibleID[i];
                    }

                    for (size_t i = 0; i < sizeof(this->SubCompatibleID) && SubCompatibleID[i] != '\0'; i++) {
                        this->SubCompatibleID[i] = SubCompatibleID[i];
                    }
                }
            } __attribute__((packed));

            enum PropertyDataType : uint16_t {
                REG_SZ = 1,
                REG_EXPAND_SZ = 2,
                REG_BINARY = 3,
                REG_DWORD_LITTLE_ENDIAN = 4,
                REG_DWORD_BIG_ENDIAN = 5,
                REG_LINK = 6,
                REG_MULTI_SZ = 7,
            };
            
            template<PropertyDataType property_data_type, detail::FixedString property_name, detail::FixedString property_data>
            struct FeatureRegistryProperty {
                uint16_t wLength = sizeof(*this);
                DescriptorType wDescriptorType = DescriptorType::FeatureRegProperty;
                uint16_t wPropertyDataType = property_data_type;
                uint16_t wPropertyNameLength = (property_name.length + 1) * 2;
                char16_t PropertyName[property_name.length + 1] = {0};
                uint16_t wPropertyDataLength = (property_data.length + 1) * 2;
                char16_t PropertyData[property_data.length + 1] = {0};

                constexpr FeatureRegistryProperty() {
                    for (size_t i = 0; i < property_name.length; i++) {
                        this->PropertyName[i] = property_name.buffer[i];
                    }
                    for (size_t i = 0; i < property_data.length; i++) {
                        this->PropertyData[i] = property_data.buffer[i];
                    }
                }
            } __attribute__((packed));
            
        } // namespace os20
        
    } // namespace ms

    namespace detail {
        template<typename First, typename... T>
        struct PackedTuple {
            First first;
            PackedTuple<T...> remaining;

            constexpr PackedTuple(First first, T... remaining) :
                first(first),
                remaining(remaining...) {
            }
        } __attribute__((packed));

        template<typename First>
        struct PackedTuple<First> {
            First first;

            constexpr PackedTuple(First first) : first(first) {
            }
        } __attribute__((packed));
    } // namespace detail
    
    template<typename... T>
    struct BOSDescriptorContainer {
        BOSDescriptor bos_descriptor;
        detail::PackedTuple<T...> tuple;

        constexpr BOSDescriptorContainer(T... members) : bos_descriptor {
                {
                    .bLength = sizeof(BOSDescriptor),
                    .bDescriptorType = DescriptorType::BOS,
                },
                {
                    .wTotalLength = sizeof(*this),
                    .bNumDeviceCaps = sizeof...(T),
                }
            }, tuple(members...) {
        }
    } __attribute__((packed));

    template<typename... T>
    constexpr BOSDescriptorContainer<T...> make_bos_descriptor_container(T... members) {
        return BOSDescriptorContainer<T...>(members...);
    }
    
    struct SetupPacket {
        enum class StandardRequest : uint8_t {
            GET_STATUS = 0,
            CLEAR_FEATURE = 1,
            // reserved
            SET_FEATURE = 3,
            // reserved
            SET_ADDRESS = 5,
            GET_DESCRIPTOR = 6,
            SET_DESCRIPTOR = 7,
            GET_CONFIGURATION = 8,
            SET_CONFIGURATION = 9,
            GET_INTERFACE = 10,
            SET_INTERFACE = 11,
            SYNCH_FRAME = 12
        };

        enum class RequestDirection {
            HostToDevice = 0,
            DeviceToHost = 1,
        };

        enum class RequestType {
            Standard = 0,
            Class = 1,
            Vendor = 2,
            Reserved = 3,
        };

        enum class RequestRecipient {
            Device = 0,
            Interface = 1,
            Endpoint = 2,
            Other = 3
        };

        static inline uint8_t PackRequestType(RequestDirection dir, RequestType type, RequestRecipient recipient) {
            return ((uint8_t) dir << 7) |
                ((uint8_t) type << 5) |
                ((uint8_t) recipient << 0);
        }
        
        uint8_t bmRequestType;
        uint8_t bRequest;
        uint16_t wValue;
        uint16_t wIndex;
        uint16_t wLength;
    } __attribute__((packed));
    
} // namespace usb
