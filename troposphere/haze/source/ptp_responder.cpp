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
#include <haze.hpp>
#include <haze/ptp_data_builder.hpp>
#include <haze/ptp_data_parser.hpp>
#include <haze/ptp_responder_types.hpp>

namespace haze {

    namespace {

        PtpBuffers *GetBuffers() {
            static constinit PtpBuffers buffers = {};
            return std::addressof(buffers);
        }

    }

    Result PtpResponder::Initialize(EventReactor *reactor, PtpObjectHeap *object_heap) {
        m_object_heap = object_heap;
        m_buffers = GetBuffers();

        /* Configure fs proxy. */
        m_fs.Initialize(reactor, fsdevGetDeviceFileSystem("sdmc"));

        R_RETURN(m_usb_server.Initialize(std::addressof(MtpInterfaceInfo), SwitchMtpIdVendor, SwitchMtpIdProduct, reactor));
    }

    void PtpResponder::Finalize() {
        m_usb_server.Finalize();
        m_fs.Finalize();
    }

    Result PtpResponder::LoopProcess() {
        while (true) {
            /* Try to handle a request. */
            R_TRY_CATCH(this->HandleRequest()) {
                R_CATCH(haze::ResultStopRequested, haze::ResultFocusLost) {
                    /* If we encountered a stop condition, we're done.*/
                    R_THROW(R_CURRENT_RESULT);
                }
                R_CATCH_ALL() {
                    /* On other failures, try to handle another request. */
                    continue;
                }
            } R_END_TRY_CATCH;

            /* Otherwise, handle the next request. */
            /* ... */
        }
    }

    Result PtpResponder::HandleRequest() {
        ON_RESULT_FAILURE {
            /* For general failure modes, the failure is unrecoverable. Close the session. */
            this->ForceCloseSession();
        };

        R_TRY_CATCH(this->HandleRequestImpl()) {
            R_CATCH(haze::ResultUnknownRequestType) {
                R_TRY(this->WriteResponse(PtpResponseCode_GeneralError));
            }
            R_CATCH(haze::ResultSessionNotOpen) {
                R_TRY(this->WriteResponse(PtpResponseCode_SessionNotOpen));
            }
            R_CATCH(haze::ResultOperationNotSupported) {
                R_TRY(this->WriteResponse(PtpResponseCode_OperationNotSupported));
            }
            R_CATCH(haze::ResultInvalidStorageId) {
                R_TRY(this->WriteResponse(PtpResponseCode_InvalidStorageId));
            }
            R_CATCH(haze::ResultInvalidObjectId) {
                R_TRY(this->WriteResponse(PtpResponseCode_InvalidObjectHandle));
            }
            R_CATCH(haze::ResultUnknownPropertyCode) {
                R_TRY(this->WriteResponse(PtpResponseCode_MtpObjectPropNotSupported));
            }
            R_CATCH(haze::ResultInvalidPropertyValue) {
                R_TRY(this->WriteResponse(PtpResponseCode_MtpInvalidObjectPropValue));
            }
            R_CATCH(haze::ResultGroupSpecified) {
                R_TRY(this->WriteResponse(PtpResponseCode_MtpSpecificationByGroupUnsupported));
            }
            R_CATCH(haze::ResultDepthSpecified) {
                R_TRY(this->WriteResponse(PtpResponseCode_MtpSpecificationByDepthUnsupported));
            }
            R_CATCH(haze::ResultInvalidArgument) {
                R_TRY(this->WriteResponse(PtpResponseCode_GeneralError));
            }
            R_CATCH_MODULE(fs) {
                /* Errors from fs are typically recoverable. */
                R_TRY(this->WriteResponse(PtpResponseCode_GeneralError));
            }
        } R_END_TRY_CATCH;

        R_SUCCEED();
    }

    Result PtpResponder::HandleRequestImpl() {
        PtpDataParser dp(m_buffers->usb_bulk_read_buffer, std::addressof(m_usb_server));
        R_TRY(dp.Read(std::addressof(m_request_header)));

        switch (m_request_header.type) {
            case PtpUsbBulkContainerType_Command: R_RETURN(this->HandleCommandRequest(dp));
            default:                              R_THROW(haze::ResultUnknownRequestType());
        }
    }

    Result PtpResponder::HandleCommandRequest(PtpDataParser &dp) {
        if (!m_session_open && m_request_header.code != PtpOperationCode_OpenSession && m_request_header.code != PtpOperationCode_GetDeviceInfo)  {
            R_THROW(haze::ResultSessionNotOpen());
        }

        switch (m_request_header.code) {
            case PtpOperationCode_GetDeviceInfo:              R_RETURN(this->GetDeviceInfo(dp));           break;
            case PtpOperationCode_OpenSession:                R_RETURN(this->OpenSession(dp));             break;
            case PtpOperationCode_CloseSession:               R_RETURN(this->CloseSession(dp));            break;
            case PtpOperationCode_GetStorageIds:              R_RETURN(this->GetStorageIds(dp));           break;
            case PtpOperationCode_GetStorageInfo:             R_RETURN(this->GetStorageInfo(dp));          break;
            case PtpOperationCode_GetObjectHandles:           R_RETURN(this->GetObjectHandles(dp));        break;
            case PtpOperationCode_GetObjectInfo:              R_RETURN(this->GetObjectInfo(dp));           break;
            case PtpOperationCode_GetObject:                  R_RETURN(this->GetObject(dp));               break;
            case PtpOperationCode_SendObjectInfo:             R_RETURN(this->SendObjectInfo(dp));          break;
            case PtpOperationCode_SendObject:                 R_RETURN(this->SendObject(dp));              break;
            case PtpOperationCode_DeleteObject:               R_RETURN(this->DeleteObject(dp));            break;
            case PtpOperationCode_MtpGetObjectPropsSupported: R_RETURN(this->GetObjectPropsSupported(dp)); break;
            case PtpOperationCode_MtpGetObjectPropDesc:       R_RETURN(this->GetObjectPropDesc(dp));       break;
            case PtpOperationCode_MtpGetObjectPropValue:      R_RETURN(this->GetObjectPropValue(dp));      break;
            case PtpOperationCode_MtpSetObjectPropValue:      R_RETURN(this->SetObjectPropValue(dp));      break;
            case PtpOperationCode_MtpGetObjPropList:          R_RETURN(this->GetObjectPropList(dp));       break;
            case PtpOperationCode_AndroidGetPartialObject64:  R_RETURN(this->GetPartialObject64(dp));      break;
            case PtpOperationCode_AndroidSendPartialObject:   R_RETURN(this->SendPartialObject(dp));       break;
            case PtpOperationCode_AndroidTruncateObject:      R_RETURN(this->TruncateObject(dp));          break;
            case PtpOperationCode_AndroidBeginEditObject:     R_RETURN(this->BeginEditObject(dp));         break;
            case PtpOperationCode_AndroidEndEditObject:       R_RETURN(this->EndEditObject(dp));           break;
            default:                                          R_THROW(haze::ResultOperationNotSupported());
        }
    }

    void PtpResponder::ForceCloseSession() {
        if (m_session_open) {
            m_session_open = false;
            m_object_database.Finalize();
        }
    }

    Result PtpResponder::WriteResponse(PtpResponseCode code, const void* data, size_t size) {
        PtpDataBuilder db(m_buffers->usb_bulk_write_buffer, std::addressof(m_usb_server));
        R_TRY(db.AddResponseHeader(m_request_header, code, size));
        R_TRY(db.AddBuffer(reinterpret_cast<const u8*>(data), size));
        R_RETURN(db.Commit());
    }

    Result PtpResponder::WriteResponse(PtpResponseCode code) {
        PtpDataBuilder db(m_buffers->usb_bulk_write_buffer, std::addressof(m_usb_server));
        R_TRY(db.AddResponseHeader(m_request_header, code, 0));
        R_RETURN(db.Commit());
    }

}
