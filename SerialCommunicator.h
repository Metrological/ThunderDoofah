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

#include "DataExchange.h"
#include "SimpleSerial.h"

#include <list>

namespace WPEFramework {

namespace Doofah {
    class SerialCommunicator {
    private:
        class SerialConfig : public Core::JSON::Container {
        private:
            SerialConfig(const SerialConfig&) = delete;
            SerialConfig& operator=(const SerialConfig&) = delete;

        public:
            SerialConfig()
                : Core::JSON::Container()
                , Port(_T("/dev/ttyUSB0"))
                , BaudRate(115200)
                , FlowControl(Core::SerialPort::OFF)
            {
                Add(_T("port"), &Port);
                Add(_T("baudrate"), &BaudRate);
                Add(_T("flowcontrol"), &FlowControl);
            }
            ~SerialConfig()
            {
            }

        public:
            Core::JSON::String Port;
            Core::JSON::DecUInt32 BaudRate;
            Core::JSON::EnumType<Core::SerialPort::FlowControl> FlowControl;
        };

        class BLEConfig : public Core::JSON::Container {
        private:
            BLEConfig(const BLEConfig&) = delete;
            BLEConfig& operator=(const BLEConfig&) = delete;

        public:
            BLEConfig()
                : Core::JSON::Container()
                , VID(0)
                , PID(0)
                , Name()
            {
                Add(_T("vid"), &VID);
                Add(_T("pid"), &PID);
                Add(_T("name"), &Name);
            }
            ~BLEConfig()
            {
            }

        public:
            Core::JSON::DecUInt16 VID;
            Core::JSON::DecUInt16 PID;
            Core::JSON::String Name;
        };

        class IRConfig : public Core::JSON::Container {
        private:
            IRConfig(const IRConfig&) = delete;
            IRConfig& operator=(const IRConfig&) = delete;

        public:
            IRConfig()
                : Core::JSON::Container()
                , CarrierHz(38000)
            {
                Add(_T("carrier"), &CarrierHz);
            }
            ~IRConfig()
            {
            }

        public:
            Core::JSON::DecUInt16 CarrierHz;
        };

        class PeripheralConfig : public Core::JSON::Container {
        private:
            PeripheralConfig(const PeripheralConfig&) = delete;
            PeripheralConfig& operator=(const PeripheralConfig&) = delete;

        public:
            PeripheralConfig()
                : Core::JSON::Container()
                , BLE()
                , IR()
            {
                Add(_T("ble"), &BLE);
                Add(_T("ir"), &IR);
            }
            ~PeripheralConfig() = default;

        public:
            Core::JSON::String BLE;
            Core::JSON::String IR;
        };

        class ResourceMessage : public SimpleSerial::Protocol::Message {
        public:
            ResourceMessage() = delete;
            ResourceMessage(const ResourceMessage&) = delete;
            ResourceMessage& operator=(const ResourceMessage&) = delete;

            ResourceMessage(const SimpleSerial::Protocol::DeviceAddressType address, const bool aquire)
            {
                Clear();

                Operation(aquire ? SimpleSerial::Protocol::OperationType::ALLOCATE : SimpleSerial::Protocol::OperationType::FREE);
                Sequence(SimpleSerial::GetSequence());
                Address(address);
                PayloadLength(0);
            }
        };

        class KeyMessage : public SimpleSerial::Protocol::Message {
        public:
            KeyMessage() = delete;
            KeyMessage(const KeyMessage&) = delete;
            KeyMessage& operator=(const KeyMessage&) = delete;

            KeyMessage(const SimpleSerial::Protocol::DeviceAddressType address, const uint16_t keyCode, const bool pressed)
            {
                SimpleSerial::Payload::KeyEvent payload;

                payload.pressed = (pressed == true) ? SimpleSerial::Payload::Action::PRESSED : SimpleSerial::Payload::Action::RELEASED;
                payload.code = keyCode;

                Clear();

                Operation(SimpleSerial::Protocol::OperationType::KEY);
                Sequence(SimpleSerial::GetSequence());
                Address(address);
                PayloadLength(sizeof(payload));

                Deserialize(sizeof(payload), reinterpret_cast<uint8_t*>(&payload));
            }
        };

        class StateMessage : public SimpleSerial::Protocol::Message {
        public:
            StateMessage() = delete;
            StateMessage(const StateMessage&) = delete;
            StateMessage& operator=(const StateMessage&) = delete;

            StateMessage(const SimpleSerial::Protocol::DeviceAddressType address)
                : _offset(~0)
            {
                Clear();

                Operation(SimpleSerial::Protocol::OperationType::STATE);
                Sequence(SimpleSerial::GetSequence());
                Address(address);
            }

            inline bool IsValidPayload() const
            {
                return (_offset < PayloadLength());
            }

            inline bool Next()
            {
                if (_offset == static_cast<uint8_t>(~0)) {
                    _offset = 0;
                } else if (_offset < PayloadLength()) {
                    _offset += sizeof(SimpleSerial::Payload::Device);
                }

                return IsValidPayload();
            }

            void Reset()
            {
                _offset = ~0;
            }

            const SimpleSerial::Payload::Device* Current()
            {
                return (IsValidPayload() == true) ? nullptr : reinterpret_cast<const SimpleSerial::Payload::Device*>(Payload() + _offset);
            }

        private:
            uint8_t _offset;
        };

        class SettingsMessage : public SimpleSerial::Protocol::Message {
        private:
            SettingsMessage(const SimpleSerial::Protocol::DeviceAddressType address, const uint8_t length, const uint8_t payload[])
            {
                Clear();

                Operation(SimpleSerial::Protocol::OperationType::SETTINGS);
                Sequence(SimpleSerial::GetSequence());
                Address(address);
                PayloadLength(length);
                Deserialize(length, payload);
            }

        public:
            SettingsMessage() = delete;
            SettingsMessage(const SettingsMessage&) = delete;
            SettingsMessage& operator=(const SettingsMessage&) = delete;

            SettingsMessage(const SimpleSerial::Protocol::DeviceAddressType address, const IRConfig& config)
            {
                SimpleSerial::Payload::IRSettings payload;

                memset(&payload, 0, sizeof(payload));

                if (config.CarrierHz.IsSet()) {
                    payload.carrier_hz = config.CarrierHz.Value();
                }

                SettingsMessage(address, sizeof(payload), reinterpret_cast<uint8_t*>(&payload));
            }

            SettingsMessage(const SimpleSerial::Protocol::DeviceAddressType address, const BLEConfig& config)
            {
                SimpleSerial::Payload::BLESettings payload;

                memset(&payload, 0, sizeof(payload));

                if (config.VID.IsSet()) {
                    payload.vid = config.VID.Value();
                }

                if (config.PID.IsSet()) {
                    payload.pid = config.PID.Value();
                }

                if (config.Name.IsSet()) {
                    uint8_t copyLength = std::min(config.Name.Value().size(), SimpleSerial::Protocol::MaxPayloadSize - (sizeof(payload) - sizeof(payload.name)));
                    memcpy(&payload.vid, config.Name.Value().c_str(), copyLength);
                }

                SettingsMessage(address, sizeof(payload), reinterpret_cast<uint8_t*>(&payload));
            }
        };

        class ResetMessage : public SimpleSerial::Protocol::Message {
        public:
            ResetMessage() = delete;
            ResetMessage(const SettingsMessage&) = delete;
            ResetMessage& operator=(const SettingsMessage&) = delete;

            ResetMessage(const SimpleSerial::Protocol::DeviceAddressType address)
            {
                Clear();

                Operation(SimpleSerial::Protocol::OperationType::RESET);
                Sequence(SimpleSerial::GetSequence());
                Address(address);
                PayloadLength(0);
            }
        };

    public:
        SerialCommunicator()
            : _channel(*this)
        {
        }
        SerialCommunicator(const SerialCommunicator&) = delete;
        SerialCommunicator& operator=(const SerialCommunicator&) = delete;

        virtual ~SerialCommunicator() = default;

        typedef Core::IteratorType<const std::list<SimpleSerial::Payload::Device>, const SimpleSerial::Payload::Device&, std::list<SimpleSerial::Payload::Device>::const_iterator> DeviceIterator;

        uint32_t Initialize(const std::string& config);
        void Deinitialize(); 

        DeviceIterator Devices();

        uint32_t KeyEvent(const SimpleSerial::Protocol::DeviceAddressType address, const bool pressed, const uint16_t code);

        uint32_t Reset(const SimpleSerial::Protocol::DeviceAddressType address);
        uint32_t Setup(const SimpleSerial::Protocol::DeviceAddressType address, const string& config);

        uint32_t Release(const SimpleSerial::Protocol::DeviceAddressType address);
        uint32_t Allocate(const SimpleSerial::Protocol::DeviceAddressType address);

    private:
        class Channel : public SimpleSerial::DataExchange<Core::SerialPort> {
        private:
            typedef SimpleSerial::DataExchange<Core::SerialPort> BaseClass;

        public:
            Channel() = delete;
            Channel(const Channel&) = delete;
            Channel& operator=(const Channel&) = delete;

            Channel(SerialCommunicator& parent)
                : BaseClass()
                , _parent(parent)
            {
            }

            virtual ~Channel() = default;

        private:
            virtual void Received(const SimpleSerial::Protocol::Message& message) override
            {
                _parent.Received(message);
            }

        private:
            SerialCommunicator& _parent;
        };

    public:
        virtual void Received(const SimpleSerial::Protocol::Message& element);

        typedef std::map<string, SimpleSerial::Protocol::DeviceAddressType> DeviceMap;

    private:
        Channel _channel;
    }; // class SerialCommunicator
} // namespace plugin
} // namespace WPEFramework
