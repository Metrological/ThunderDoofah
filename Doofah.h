/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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

#pragma once

#include "Module.h"

#include <interfaces/json/JsonData_RemoteControl.h>

#include "SimpleSerial.h"

#include "SerialCommunicator.h"

namespace WPEFramework {
namespace Plugin {
    class Doofah : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {

    public:
        Doofah(const Doofah&) = delete;
        Doofah& operator=(const Doofah&) = delete;

        Doofah()
            : _skipURL(0)
            , _connectionId(0)
            , _service(nullptr)
            , _communicator()
        {
            RegisterAll();
        }

        virtual ~Doofah() override
        {
            UnregisterAll();
        }

        BEGIN_INTERFACE_MAP(Doofah)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Port(_T("/dev/ttyUSB0"))
                , Device(~0)
                , Peripheral(SimpleSerial::Payload::Peripheral::ROOT)
            {
                Add(_T("port"), &Port);
                Add(_T("device"), &Device);
                Add(_T("peripheral"), &Peripheral);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String Port;
            Core::JSON::DecUInt8 Device;
            Core::JSON::EnumType<SimpleSerial::Payload::Peripheral> Peripheral;
        };

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

        //   IWeb methods
        // -------------------------------------------------------------------------------------------------------
        void Inbound(Web::Request& request) override;
        Core::ProxyType<Web::Response> Process(const Web::Request& request) override;

        // JSON RPC
        // -------------------------------------------------------------------------------------------------------
        void RegisterAll();
        void UnregisterAll();

        uint32_t endpoint_press(const JsonData::RemoteControl::KeyobjInfo& params);
        uint32_t endpoint_release(const JsonData::RemoteControl::KeyobjInfo& params);

        void event_keypressed(const string& id, const bool& pressed);
    private:
        uint8_t _skipURL;
        uint32_t _connectionId;
        PluginHost::IShell* _service;
        WPEFramework::Doofah::SerialCommunicator _communicator;
    };
} // namespace Plugin
} // namespace WPEFramework
