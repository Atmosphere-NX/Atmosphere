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
#include "../capsrv/server/decodersrv/decodersrv_decoder_control_service.hpp"
#include "../lm/sf/lm_i_log_getter.hpp"
#include "../lm/sf/lm_i_log_service.hpp"
#include "../sf/hipc/sf_i_hipc_manager.hpp"
#include "../sprofile/srv/sprofile_srv_i_service_getter.hpp"

namespace {

    constexpr u32 GenerateInterfaceIdFromName(const char *s) {
        /* Get the interface length. */
        const auto len = ams::util::Strlen(s);

        /* Calculate the sha256. */
        u8 hash[ams::crypto::Sha256Generator::HashSize] = {};
        ams::crypto::GenerateSha256(hash, sizeof(hash), s, len);

        /* Read it out as little endian. */
        u32 id = 0;
        for (size_t i = 0; i < sizeof(id); ++i) {
            id |= static_cast<u32>(hash[i]) << (BITSIZEOF(u8) * i);
        }

        return id;
    }

    static_assert(GenerateInterfaceIdFromName("nn::sf::hipc::detail::IHipcManager") == 0xEC6BE3FF);

    constexpr void ConvertAtmosphereNameToNintendoName(char *dst, const char *src) {
        /* Determine src len. */
        const auto len = ams::util::Strlen(src);
        const auto *s  = src;

        /* Atmosphere names begin with ams::, Nintendo names begin with nn::. */
        AMS_ASSUME(src[0] == 'a');
        AMS_ASSUME(src[1] == 'm');
        AMS_ASSERT(src[2] == 's');
        dst[0] = 'n';
        dst[1] = 'n';
        src += 3;
        dst += 2;

        /* Copy over. */
        while ((src - s) < len) {
            /* Atmosphere uses ::impl:: instead of ::detail::, ::IDeprecated* for deprecated services. */
            if (src[0] == ':' && src[1] == ':' && src[2] == 'i' && src[3] == 'm' && src[4] == 'p' && src[5] == 'l' && src[6] == ':' && src[7] == ':') {
                dst[0] = ':';
                dst[1] = ':';
                dst[2] = 'd';
                dst[3] = 'e';
                dst[4] = 't';
                dst[5] = 'a';
                dst[6] = 'i';
                dst[7] = 'l';

                src += 6; /* ::impl */
                dst += 8; /* ::detail */
            } else if (src[0] == ':' && src[1] == ':' && src[2] == 'I' && src[3] == 'D' && src[4] == 'e' && src[5] == 'p' && src[6] == 'r' && src[7] == 'e' && src[8] == 'c' && src[9] == 'a' && src[10] == 't' && src[11] == 'e' && src[12] == 'd') {
                dst[0] = ':';
                dst[1] = ':';
                dst[2] = 'I';

                src += 13; /* ::IDeprecated */
                dst +=  3; /* ::I */
            } else {
                *(dst++) = *(src++);
            }
        }

        *dst = 0;
    }

    constexpr u32 GenerateInterfaceIdFromAtmosphereName(const char *ams) {
        char nn[0x100] = {};
        ConvertAtmosphereNameToNintendoName(nn, ams);

        return GenerateInterfaceIdFromName(nn);
    }

    static_assert(GenerateInterfaceIdFromAtmosphereName("ams::sf::hipc::impl::IHipcManager") == GenerateInterfaceIdFromName("nn::sf::hipc::detail::IHipcManager"));

}

#define AMS_IMPL_CHECK_INTERFACE_ID(AMS_INTF) \
    static_assert(AMS_INTF::InterfaceIdForDebug == GenerateInterfaceIdFromAtmosphereName( #AMS_INTF ), #AMS_INTF)


AMS_IMPL_CHECK_INTERFACE_ID(ams::capsrv::sf::IDecoderControlService);

AMS_IMPL_CHECK_INTERFACE_ID(ams::erpt::sf::IAttachment);
AMS_IMPL_CHECK_INTERFACE_ID(ams::erpt::sf::IContext);
AMS_IMPL_CHECK_INTERFACE_ID(ams::erpt::sf::IManager);
AMS_IMPL_CHECK_INTERFACE_ID(ams::erpt::sf::IReport);
AMS_IMPL_CHECK_INTERFACE_ID(ams::erpt::sf::ISession);

static_assert(::ams::fatal::impl::IPrivateService::InterfaceIdForDebug == GenerateInterfaceIdFromName("nn::fatalsrv::IPrivateService")); // TODO: FIX-TO-MATCH
static_assert(::ams::fatal::impl::IService::InterfaceIdForDebug        == GenerateInterfaceIdFromName("nn::fatalsrv::IService"));        // TODO: FIX-TO-MATCH

AMS_IMPL_CHECK_INTERFACE_ID(ams::fssrv::sf::IDirectory);
AMS_IMPL_CHECK_INTERFACE_ID(ams::fssrv::sf::IFile);
AMS_IMPL_CHECK_INTERFACE_ID(ams::fssrv::sf::IFileSystem);
AMS_IMPL_CHECK_INTERFACE_ID(ams::fssrv::sf::IStorage);
AMS_IMPL_CHECK_INTERFACE_ID(ams::fssrv::sf::IDeviceOperator);
AMS_IMPL_CHECK_INTERFACE_ID(ams::fssrv::sf::IEventNotifier);
AMS_IMPL_CHECK_INTERFACE_ID(ams::fssrv::sf::IFileSystemProxy);
AMS_IMPL_CHECK_INTERFACE_ID(ams::fssrv::sf::IFileSystemProxyForLoader);
AMS_IMPL_CHECK_INTERFACE_ID(ams::fssrv::sf::IProgramRegistry);

static_assert(::ams::gpio::sf::IManager::InterfaceIdForDebug    == GenerateInterfaceIdFromName("nn::gpio::IManager"));    // TODO: FIX-TO-MATCH
static_assert(::ams::gpio::sf::IPadSession::InterfaceIdForDebug == GenerateInterfaceIdFromName("nn::gpio::IPadSession")); // TODO: FIX-TO-MATCH

AMS_IMPL_CHECK_INTERFACE_ID(ams::htc::tenv::IService);
AMS_IMPL_CHECK_INTERFACE_ID(ams::htc::tenv::IServiceManager);

static_assert(::ams::i2c::sf::IManager::InterfaceIdForDebug == GenerateInterfaceIdFromName("nn::i2c::IManager")); // TODO: FIX-TO-MATCH
static_assert(::ams::i2c::sf::ISession::InterfaceIdForDebug == GenerateInterfaceIdFromName("nn::i2c::ISession")); // TODO: FIX-TO-MATCH

AMS_IMPL_CHECK_INTERFACE_ID(ams::ldr::impl::IDebugMonitorInterface);
AMS_IMPL_CHECK_INTERFACE_ID(ams::ldr::impl::IProcessManagerInterface);
AMS_IMPL_CHECK_INTERFACE_ID(ams::ldr::impl::IShellInterface);

AMS_IMPL_CHECK_INTERFACE_ID(ams::lr::IAddOnContentLocationResolver);
AMS_IMPL_CHECK_INTERFACE_ID(ams::lr::ILocationResolver);
AMS_IMPL_CHECK_INTERFACE_ID(ams::lr::ILocationResolverManager);
AMS_IMPL_CHECK_INTERFACE_ID(ams::lr::IRegisteredLocationResolver);

AMS_IMPL_CHECK_INTERFACE_ID(ams::lm::ILogGetter);
AMS_IMPL_CHECK_INTERFACE_ID(ams::lm::ILogger);
AMS_IMPL_CHECK_INTERFACE_ID(ams::lm::ILogService);

AMS_IMPL_CHECK_INTERFACE_ID(ams::ncm::IContentManager);
AMS_IMPL_CHECK_INTERFACE_ID(ams::ncm::IContentMetaDatabase);
AMS_IMPL_CHECK_INTERFACE_ID(ams::ncm::IContentStorage);

AMS_IMPL_CHECK_INTERFACE_ID(ams::ns::impl::IAsyncResult);

//AMS_IMPL_CHECK_INTERFACE_ID(ams::pgl::sf::IEventObserver);
//AMS_IMPL_CHECK_INTERFACE_ID(ams::pgl::sf::IShellInterface);

AMS_IMPL_CHECK_INTERFACE_ID(ams::pm::impl::IBootModeInterface);
AMS_IMPL_CHECK_INTERFACE_ID(ams::pm::impl::IDebugMonitorInterface);
AMS_IMPL_CHECK_INTERFACE_ID(ams::pm::impl::IDeprecatedDebugMonitorInterface);
AMS_IMPL_CHECK_INTERFACE_ID(ams::pm::impl::IInformationInterface);
AMS_IMPL_CHECK_INTERFACE_ID(ams::pm::impl::IShellInterface);
AMS_IMPL_CHECK_INTERFACE_ID(ams::pm::impl::IDeprecatedShellInterface);

AMS_IMPL_CHECK_INTERFACE_ID(ams::psc::sf::IPmModule);
AMS_IMPL_CHECK_INTERFACE_ID(ams::psc::sf::IPmService);

static_assert(::ams::pwm::sf::IChannelSession::InterfaceIdForDebug == GenerateInterfaceIdFromName("nn::pwm::IChannelSession")); // TODO: FIX-TO-MATCH
static_assert(::ams::pwm::sf::IManager::InterfaceIdForDebug        == GenerateInterfaceIdFromName("nn::pwm::IManager"));        // TODO: FIX-TO-MATCH

AMS_IMPL_CHECK_INTERFACE_ID(ams::ro::impl::IDebugMonitorInterface);
AMS_IMPL_CHECK_INTERFACE_ID(ams::ro::impl::IRoInterface);

//AMS_IMPL_CHECK_INTERFACE_ID(ams::sf::hipc::impl::IMitmQueryService);
AMS_IMPL_CHECK_INTERFACE_ID(ams::sf::hipc::impl::IHipcManager);

AMS_IMPL_CHECK_INTERFACE_ID(ams::spl::impl::ICryptoInterface);
AMS_IMPL_CHECK_INTERFACE_ID(ams::spl::impl::IDeprecatedGeneralInterface);
AMS_IMPL_CHECK_INTERFACE_ID(ams::spl::impl::IDeviceUniqueDataInterface);
AMS_IMPL_CHECK_INTERFACE_ID(ams::spl::impl::IEsInterface);
AMS_IMPL_CHECK_INTERFACE_ID(ams::spl::impl::IFsInterface);
AMS_IMPL_CHECK_INTERFACE_ID(ams::spl::impl::IGeneralInterface);
AMS_IMPL_CHECK_INTERFACE_ID(ams::spl::impl::IManuInterface);
AMS_IMPL_CHECK_INTERFACE_ID(ams::spl::impl::IRandomInterface);
AMS_IMPL_CHECK_INTERFACE_ID(ams::spl::impl::ISslInterface);

AMS_IMPL_CHECK_INTERFACE_ID(ams::sprofile::srv::IProfileControllerForDebug);
AMS_IMPL_CHECK_INTERFACE_ID(ams::sprofile::srv::IProfileImporter);
AMS_IMPL_CHECK_INTERFACE_ID(ams::sprofile::srv::IProfileReader);
AMS_IMPL_CHECK_INTERFACE_ID(ams::sprofile::srv::IProfileUpdateObserver);
AMS_IMPL_CHECK_INTERFACE_ID(ams::sprofile::srv::ISprofileServiceForBgAgent);
AMS_IMPL_CHECK_INTERFACE_ID(ams::sprofile::srv::ISprofileServiceForSystemProcess);
AMS_IMPL_CHECK_INTERFACE_ID(ams::sprofile::srv::IServiceGetter);

AMS_IMPL_CHECK_INTERFACE_ID(ams::tma::IDirectoryAccessor);
AMS_IMPL_CHECK_INTERFACE_ID(ams::tma::IFileAccessor);
AMS_IMPL_CHECK_INTERFACE_ID(ams::tma::IFileManager);
AMS_IMPL_CHECK_INTERFACE_ID(ams::tma::IDeprecatedFileManager);
AMS_IMPL_CHECK_INTERFACE_ID(ams::tma::IHtcsManager);
AMS_IMPL_CHECK_INTERFACE_ID(ams::tma::IHtcManager);
AMS_IMPL_CHECK_INTERFACE_ID(ams::tma::ISocket);

AMS_IMPL_CHECK_INTERFACE_ID(ams::usb::ds::IDsEndpoint);
AMS_IMPL_CHECK_INTERFACE_ID(ams::usb::ds::IDsInterface);
AMS_IMPL_CHECK_INTERFACE_ID(ams::usb::ds::IDsService);
AMS_IMPL_CHECK_INTERFACE_ID(ams::usb::ds::IDsRootSession);
