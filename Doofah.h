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

#include "JsonData_Doofah.h"

#include "SimpleSerial.h"

#include "SerialCommunicator.h"

namespace Thunder {
namespace Plugin {
    using namespace Thunder::SimpleSerial;
    using namespace JsonData::Doofah;

    class Doofah : public PluginHost::IPlugin, public PluginHost::IWeb, public PluginHost::JSONRPC {
    public:
        Doofah(const Doofah&) = delete;
        Doofah& operator=(const Doofah&) = delete;

        Doofah()
            : _skipURL(0)
            , _communicator()
            , _sink(*this)
            , _service(nullptr)
        {
        }

        virtual ~Doofah() override
        {
        }

        BEGIN_INTERFACE_MAP(Doofah)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IWeb)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    private:
        bool ParseSetupBody(const Web::Request& request, Protocol::DeviceAddressType& address, string& setup);
        bool ParseKeyCodeBody(const Web::Request& request, Protocol::DeviceAddressType& address, uint32_t& code);
        bool ParseDeviceAddressBody(const Web::Request& request, Protocol::DeviceAddressType& address);

        Core::ProxyType<Web::Response> GetMethod(Core::TextSegmentIterator& index);
        Core::ProxyType<Web::Response> PutMethod(Core::TextSegmentIterator& index, const Web::Request& request);

        class Sink : public Thunder::Doofah::SerialCommunicator::ICallback {
        private:
            Sink(const Sink&) = delete;
            Sink& operator=(const Sink&) = delete;
            Sink() = delete;

        public:
            Sink(Doofah& parent)
                : _parent(parent)
            {
            }

            void Started()
            {
                TRACE(Trace::Information, ("End-point started!"));
                _parent.EventStarted();
            }

        private:
            Doofah& _parent;
        };

    public:
        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Connector()
            {
                Add(_T("connector"), &Connector);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String Connector;
        };

        static void FillDeviceInfo(const Payload::Device& info, DeviceEntry& entry)
        {
            entry.Device = info.address;
            entry.Peripheral = info.peripheral;
        }

        class DeviceList : public Core::JSON::Container {
        public:
            DeviceList(const DeviceList&) = delete;
            DeviceList& operator=(const DeviceList&) = delete;

        public:
            DeviceList()
                : Core::JSON::Container()
                , Devices()
            {
                Add(_T("devices"), &Devices);
            }

            ~DeviceList() override = default;

        public:
            void Set(Thunder::Doofah::SerialCommunicator::DeviceIterator& list)
            {
                list.Reset(0);

                while (list.Next() == true) {
                    DeviceEntry device;

                    Doofah::FillDeviceInfo(list.Current(), device);

                    Devices.Add(device);
                }
            }

            Core::JSON::ArrayType<DeviceEntry> Devices;
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
        void JSONRPCRegister();
        void JSONRPCUnregister();

        uint32_t JSONRPCDevices(Core::JSON::ArrayType<DeviceEntry>& response) const;

        uint32_t JSONRPCSetup(const SetupInfo& params);
        uint32_t JSONRPCReset(const DeviceInfo& params);

        uint32_t JSONRPCKeyPress(const KeyInfo& params);
        uint32_t JSONRPCKeyRelease(const KeyInfo& params);

        void EventKeyPressed(const string& id, const bool& pressed);

        void EventStarted();

    private:
        uint8_t _skipURL;
        Thunder::Doofah::SerialCommunicator _communicator;
        Sink _sink;
        PluginHost::IShell* _service;
    };
} // namespace Plugin
} // namespace Thunder
