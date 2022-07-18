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

    ENUM_CONVERSION_BEGIN(SimpleSerial::Payload::Peripheral)
    { SimpleSerial::Payload::Peripheral::ROOT, _TXT("root") },
    { SimpleSerial::Payload::Peripheral::BLE, _TXT("ble") },
    { SimpleSerial::Payload::Peripheral::IR, _TXT("ir") },
    ENUM_CONVERSION_END(SimpleSerial::Payload::Peripheral);
namespace Plugin {
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
        ASSERT(_service == nullptr);
        ASSERT(_connectionId == 0);
        ASSERT(_player == nullptr);

        string message;
        Config config;
        
        config.FromString(service->ConfigLine());
        _skipURL = static_cast<uint8_t>(service->WebPrefix().length());

        _service = service;
        _service->AddRef();

        _communicator = WPEFramework::Doofah::SerialCommunicator::Instance(config.Port.Value());

        if (message.length() != 0) {
            Deinitialize(service);
        }

        return message;
    }

    /* virtual */ void Doofah::Deinitialize(PluginHost::IShell* service VARIABLE_IS_NOT_USED)
    {
        ASSERT(service == _service);

        _communicator = nullptr;

        _service->Release();
        _service = nullptr;
        _connectionId = 0;
    }

    /* virtual */ string Doofah::Information() const
    {
        // No additional info to report.
        return (string());
    }

    /* virtual */ void Doofah::Inbound(Web::Request& /* request */)
    {
    }

    /* virtual */ Core::ProxyType<Web::Response> Doofah::Process(const Web::Request& request)
    {
        ASSERT(_skipURL <= request.Path.length());

        Core::ProxyType<Web::Response> result(PluginHost::IFactories::Instance().Response());

        // By default, we assume everything works..
        result->ErrorCode = Web::STATUS_OK;
        result->Message = "OK";

        // <GET> - currently, only the GET command is supported, returning system info
        if (request.Verb == Web::Request::HTTP_GET) {
            result->Message = "GET";

        } else {
            result->ErrorCode = Web::STATUS_BAD_REQUEST;
            result->Message = _T("Unsupported request for the [Doofah] service.");
        }

        return result;
    }

} // namespace Plugin
} // namespace WPEFramework