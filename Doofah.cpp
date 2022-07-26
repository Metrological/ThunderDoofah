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

    /* virtual */ const string Doofah::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);

        string message;
        Config config;

        config.FromString(service->ConfigLine());
        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());

        RegisterAll();

        uint32_t result = _communicator.Initialize(config.Connector.Value());

        if (result == Core::ERROR_NONE) {
            if (config.Device.IsSet()) {
                _endpoint = (_communicator.Allocate(config.Device.Value()) == Core::ERROR_NONE) ? config.Device.Value() : uint8_t(~0);
            } else if (config.Peripheral.IsSet()) {
                WPEFramework::Doofah::SerialCommunicator::DeviceIterator devices = _communicator.Devices();

                if (devices.Count() > 0) {
                    while ((devices.Next() == true) && (_endpoint != uint8_t(~0))) {
                        if ((devices.Current().peripheral == config.Peripheral.Value()) && devices.Current().state != WPEFramework::SimpleSerial::Payload::PeripheralState::OCCUPIED) {
                            _endpoint = (_communicator.Allocate(devices.Current().address) == Core::ERROR_NONE) ? devices.Current().address : uint8_t(~0);
                        }
                    }
                } else {
                    message = "Failed to aquire remote devices";
                }
            }

            if (_endpoint == uint8_t(~0)) {
                message = "Failed to aquire endpoint";
            }
        } else {
            message = "Could not setup communication channel";
        }

        if (message.empty() == false) {
            Deinitialize(service);
        }

        return message;
    }

    /* virtual */ void Doofah::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        UnregisterAll();
        if (_endpoint != uint8_t(~0)) {
            _communicator.Release(_endpoint);
        }
        _communicator.Deinitialize();
    }

    /* virtual */ string Doofah::Information() const
    {
        // No additional info to report.
        return (string());
    }

    bool Doofah::ParseKeyCodeBody(const Web::Request& request, uint32_t& code)
    {
        bool parsed = false;
        const string payload = ((request.HasBody() == true) ? string(*request.Body<const Web::TextBody>()) : "");

        if (payload.empty() == false) {
            Doofah::KeyCodeBody data;
            data.FromString(payload);

            if (data.Code.IsSet() == true) {
                code = data.Code.Value();
            }
            parsed = true;
        }

        return parsed;
    }

    bool Doofah::ParseResetBody(const Web::Request& request, Payload::Peripheral& peripheral)
    {
        bool parsed = false;
        const string payload = ((request.HasBody() == true) ? string(*request.Body<const Web::TextBody>()) : "");

        if (payload.empty() == false) {
            Doofah::ResetBody data;
            data.FromString(payload);

            if (data.Peripheral.IsSet() == true) {
                peripheral = data.Peripheral.Value();
            }
            parsed = true;
        }

        return parsed;
    }

    Core::ProxyType<Web::Response> Doofah::PutMethod(Core::TextSegmentIterator& index, const Web::Request& request)
    {
        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());

        uint32_t commResult(Core::ERROR_NONE);

        result->ErrorCode = Web::STATUS_NOT_FOUND;
        result->Message = string(_T("Unknown request path specified."));

        if (index.IsValid() == true && index.Next() == true) {
            bool pressed = false;

            // PUT .../Doofah/Press|Release : send a code to the end point
            if (((pressed = (index.Current() == _T("Press"))) == true) || (index.Current() == _T("Release"))) {
                uint32_t code = 0;

                if (ParseKeyCodeBody(request, code) == true) {
                    if ((code != 0) && (commResult = _communicator.KeyEvent(_endpoint, pressed, code) == Core::ERROR_NONE)) {
                        result->ErrorCode = Web::STATUS_ACCEPTED;
                        result->Message = string((_T("key is sent")));
                    } else {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = string(_T("failed to sent key"));
                    }
                } else {
                    result->ErrorCode = Web::STATUS_BAD_REQUEST;
                    result->Message = string(_T("No key code in body"));
                }
            } else if (index.Current() == _T("Setup")) {
                if ((commResult = _communicator.Setup(_endpoint, (request.HasBody() == true) ? string(*request.Body<const Web::TextBody>()) : "") == Core::ERROR_NONE)) {
                    result->ErrorCode = Web::STATUS_OK;
                    result->Message = string(_T("Setup OK"));
                } else {
                    result->ErrorCode = Web::STATUS_NOT_IMPLEMENTED;
                    result->Message = string(_T("Setup failed"));
                }
            } else if (index.Current() == _T("Reset")) {
                Payload::Peripheral peripheral;

                if ((ParseResetBody(request, peripheral) == true) && (peripheral == Payload::Peripheral::ROOT)) {
                    if ((commResult = _communicator.Reset(0x00) == Core::ERROR_NONE)) {
                        result->ErrorCode = Web::STATUS_ACCEPTED;
                        result->Message = string((_T("reset ok")));
                    } else {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = string(_T("failed to reset"));
                    }
                } else {
                    if ((commResult = _communicator.Reset(_endpoint) == Core::ERROR_NONE)) {
                        result->ErrorCode = Web::STATUS_OK;
                        result->Message = string(_T("Reset OK"));
                    } else {
                        result->ErrorCode = Web::STATUS_BAD_REQUEST;
                        result->Message = string(_T("Reset failed"));
                    }
                }
            } else {
                result->ErrorCode = Web::STATUS_NOT_IMPLEMENTED;
                result->Message = string(_T("Reset failed"));
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
        } else {
            result = PluginHost::IFactories::Instance().Response();
            result->ErrorCode = Web::STATUS_NOT_IMPLEMENTED;
            result->Message = string(_T("Unknown method used."));
        }
        return result;
    }

} // namespace Plugin
} // namespace WPEFramework
