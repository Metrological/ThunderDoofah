/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Module.h"

#include "Doofah.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(SimpleSerial::Payload::Peripheral) { SimpleSerial::Payload::Peripheral::ROOT, _TXT("root") },
    { SimpleSerial::Payload::Peripheral::BLE, _TXT("ble") },
    { SimpleSerial::Payload::Peripheral::IR, _TXT("ir") },
    ENUM_CONVERSION_END(SimpleSerial::Payload::Peripheral);

namespace Plugin {
    static Core::ProxyPoolType<Web::TextBody> _textBodies(2);
    namespace {
        static Metadata<Doofah> metadata(
            // Version
            1, 0, 0,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {});
    }

    static Core::ProxyPoolType<Web::JSONBodyType<Doofah::DeviceList>> jsonResponseFactoryDevicesList(1);

    /* virtual */ const string Doofah::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(_service == nullptr);

        string message;
        Config config;

        config.FromString(service->ConfigLine());
        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());

        _service = service;

        JSONRPCRegister();

        _communicator.Callback(&_sink);

        uint32_t result = _communicator.Initialize(config.Connector.Value());

        if (result == Core::ERROR_NONE) {
            result = _communicator.Reset(0x00); // reset the endpoint

            if (result != Core::ERROR_NONE) {
                TRACE(Trace::Error, ("Reset Failed 0x%04X", result));
                message = "Could not reset the end-point";
            }
        } else {
            TRACE(Trace::Error, ("Initialize Failed 0x%04X", result));
            message = "Could not setup communication channel";
        }

        if (message.empty() == false) {
            Deinitialize(service);
        }

        return message;
    }

    /* virtual */ void Doofah::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        JSONRPCUnregister();

        _communicator.Callback(nullptr);
        _communicator.Deinitialize();

        ASSERT(_service == service);
        _service = nullptr;
    }

    /* virtual */ string Doofah::Information() const
    {
        // No additional info to report.
        return (string());
    }

    bool Doofah::ParseSetupBody(const Web::Request& request, Protocol::DeviceAddressType& address, string& setup)
    {
        bool parsed = false;

        address = Protocol::InvalidAddress;

        const string payload = ((request.HasBody() == true) ? string(*request.Body<const Web::TextBody>()) : "");

        if (payload.empty() == false) {
            JsonData::Doofah::SetupInfo data;
            data.FromString(payload);

            if ((data.Device.IsSet() == true) && (data.Configuration.IsSet() == true)) {
                address = data.Device.Value();
                setup = data.Configuration.Value();
                parsed = true;
            }
        }

        TRACE(Trace::Information, ("%s:%s address=0x%02X payload='%s'", __FUNCTION__, parsed ? "OK" : "NOK", address, payload.c_str()));

        return parsed;
    }

    bool Doofah::ParseKeyCodeBody(const Web::Request& request, Protocol::DeviceAddressType& address, uint32_t& code)
    {
        bool parsed = false;

        address = Protocol::InvalidAddress;

        const string payload = ((request.HasBody() == true) ? string(*request.Body<const Web::TextBody>()) : "");

        if (payload.empty() == false) {
            JsonData::Doofah::KeyInfo data;
            data.FromString(payload);

            if ((data.Code.IsSet() == true) && (data.Device.IsSet() == true)) {
                code = data.Code.Value();
                address = data.Device.Value();
            }
            parsed = true;
        }

        TRACE(Trace::Information, ("%s:%s address=0x%02X payload='%s'", __FUNCTION__, parsed ? "OK" : "NOK", address, payload.c_str()));

        return parsed;
    }

    bool Doofah::ParseDeviceAddressBody(const Web::Request& request, Protocol::DeviceAddressType& address)
    {
        bool parsed = false;

        address = Protocol::InvalidAddress;

        const string payload = ((request.HasBody() == true) ? string(*request.Body<const Web::TextBody>()) : "");

        if (payload.empty() == false) {
            JsonData::Doofah::DeviceInfo data;
            data.FromString(payload);

            if (data.Device.IsSet() == true) {
                address = data.Device.Value();
            }
            parsed = true;
        }

        TRACE(Trace::Information, ("%s:%s address=0x%02X payload='%s'", __FUNCTION__, parsed ? "OK" : "NOK", address, payload.c_str()));

        return parsed;
    }

    Core::ProxyType<Web::Response> Doofah::GetMethod(Core::TextSegmentIterator& index)
    {
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());

        result->ErrorCode = Web::STATUS_NOT_FOUND;
        result->Message = string(_T("Unknown request path specified."));

        if (index.IsValid() == true && index.Next() == true) {
            // GET .../Doofah/ADRRESS : Get config of a specific device of the end-point
            // TODO: return config for a specific peripheral.
            result->ErrorCode = Web::STATUS_NOT_IMPLEMENTED;
            result->Message = string(_T("TODO: return config for a specific device."));
        } else {
            // GET .../Doofah : Get all devices provided by an end-point
            WPEFramework::Doofah::SerialCommunicator::DeviceIterator list = _communicator.Devices();

            Core::ProxyType<Web::JSONBodyType<Doofah::DeviceList>> devices(jsonResponseFactoryDevicesList.Element());

            devices->Set(list);

            result->ErrorCode = Web::STATUS_ACCEPTED;
            result->Message = string((_T("Returned Devices")));
            result->Body(devices);
            result->ContentType = Web::MIMETypes::MIME_JSON;
        }

        return (result);
    }

    Core::ProxyType<Web::Response> Doofah::PutMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());

        uint32_t commResult(Core::ERROR_NONE);

        result->ErrorCode = Web::STATUS_NOT_FOUND;
        result->Message = string(_T("Unknown request path specified."));

        if (index.IsValid() == true && index.Next() == true) {
            bool pressed = false;
            Protocol::DeviceAddressType address(Protocol::InvalidAddress);
    
            // PUT .../Doofah/Press|Release : send a code to the end point
            if (((pressed = (index.Current() == _T("Press"))) == true) || (index.Current() == _T("Release"))) {
                uint32_t code = 0;

                if (ParseKeyCodeBody(request, address, code) == true) {
                    if ((address != Protocol::InvalidAddress) && (code != 0) && (commResult = _communicator.KeyEvent(address, pressed, code) == Core::ERROR_NONE)) {
                        result->ErrorCode = Web::STATUS_ACCEPTED;
                        result->Message = string((_T("key is sent")));
                    } else {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = string(_T("failed to sent key"));
                    }
                } else {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = string(_T("No key code or/and device address in body"));
                }
            } else if (index.Current() == _T("Setup")) {
                string config;
                if (ParseSetupBody(request, address, config) == true) {
                    if ((address != Protocol::InvalidAddress) && (commResult = _communicator.Setup(address, config) == Core::ERROR_NONE)) {
                        result->ErrorCode = Web::STATUS_ACCEPTED;
                        result->Message = string((_T("setup ok")));
                    } else {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = string(_T("failed to setup "));
                    }
                } else {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = string(_T("No config or/and device address in body"));
                }
            } else if (index.Current() == _T("Reset")) {
                if (ParseDeviceAddressBody(request, address) == true) {
                    if ((address != Protocol::InvalidAddress) && (commResult = _communicator.Reset(address) == Core::ERROR_NONE)) {
                        result->ErrorCode = Web::STATUS_ACCEPTED;
                        result->Message = string((_T("reset ok")));
                    } else {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = string(_T("failed to reset"));
                    }
                } else {
                    result->ErrorCode = Web::STATUS_NOT_IMPLEMENTED;
                    result->Message = string(_T("Reset failed"));
                }
            }
        }
        return (result);
    }

    /* virtual */ void Doofah::Inbound(Web::Request& request)
    {
        request.Body(_textBodies.Element());
    }

    /* virtual */ Core::ProxyType<Web::Response> Doofah::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result;
        Core::TextSegmentIterator index(Core::TextFragment(request.Path, _skipURL, static_cast<uint32_t>(request.Path.length()) - _skipURL), false, '/');

        // By default, we are in front of any element, jump onto the first element, which is if, there is something an empty slot.
        index.Next();

        // If there is nothing or only a slashe, we will now jump over it, and otherwise, we have data.
        if (request.Verb == Web::Request::HTTP_PUT) {
            result = PutMethod(index, request);
        } else if (request.Verb == Web::Request::HTTP_GET) {
            result = GetMethod(index);
        } else {
            result = PluginHost::IFactories::Instance().Response();
            result->ErrorCode = Web::STATUS_NOT_IMPLEMENTED;
            result->Message = string(_T("Unknown method used."));
        }
        return result;
    }

} // namespace Plugin
} // namespace WPEFramework
