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

namespace Thunder {

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
                , Manufacturer()
            {
                Add(_T("vid"), &VID);
                Add(_T("pid"), &PID);
                Add(_T("name"), &Name);
                Add(_T("manufacturer"), &Manufacturer);
            }
            ~BLEConfig()
            {
            }

        public:
            Core::JSON::HexUInt16 VID;
            Core::JSON::HexUInt16 PID;
            Core::JSON::String Name;
            Core::JSON::String Manufacturer;
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

        class SetupConfig : public Core::JSON::Container {
        private:
            SetupConfig(const SetupConfig&) = delete;
            SetupConfig& operator=(const SetupConfig&) = delete;

        public:
            SetupConfig()
                : Core::JSON::Container()
                , Type()
                , Configuration()
            {
                Add(_T("type"), &Type);
                Add(_T("setup"), &Configuration);
            }
            ~SetupConfig() = default;

        public:
            
            Core::JSON::EnumType<SimpleSerial::Payload::Peripheral> Type;
            Core::JSON::String Configuration;
        };

        class Message : public SimpleSerial::Protocol::Message {
        public:
            Message() = delete;
            Message(const Message&) = delete;
            Message& operator=(const Message&) = delete;

            Message(const SimpleSerial::Protocol::OperationType operation, const SimpleSerial::Protocol::DeviceAddressType address)
            {
                Clear();

                Operation(operation);
                Sequence(SimpleSerial::GetSequence());
                Address(address);
            }
        };

        class KeyMessage : public Message {
        public:
            KeyMessage() = delete;
            KeyMessage(const KeyMessage&) = delete;
            KeyMessage& operator=(const KeyMessage&) = delete;

            KeyMessage(const SimpleSerial::Protocol::DeviceAddressType address, const uint16_t keyCode, const bool pressed)
                : Message(SimpleSerial::Protocol::OperationType::KEY, address)
            {
                SimpleSerial::Payload::KeyEvent payload;

                payload.pressed = (pressed == true) ? SimpleSerial::Payload::Action::PRESSED : SimpleSerial::Payload::Action::RELEASED;
                payload.code = keyCode;

                Payload(sizeof(payload), reinterpret_cast<uint8_t*>(&payload));
            }
        };

        class StateMessage : public Message {
        public:
            StateMessage() = delete;
            StateMessage(const StateMessage&) = delete;
            StateMessage& operator=(const StateMessage&) = delete;

            StateMessage(const SimpleSerial::Protocol::DeviceAddressType address)
                : Message(SimpleSerial::Protocol::OperationType::STATE, address)
                , _offset(~0)
            {
                PayloadLength(0);
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
                return (IsValidPayload() == true) ? reinterpret_cast<const SimpleSerial::Payload::Device*>(Payload() + _offset) : nullptr;
            }

        private:
            uint8_t _offset;
        };

        class SettingsMessage : public Message {
        public:
            SettingsMessage() = delete;
            SettingsMessage(const SettingsMessage&) = delete;
            SettingsMessage& operator=(const SettingsMessage&) = delete;

            SettingsMessage(const SimpleSerial::Protocol::DeviceAddressType address, const IRConfig& config)
                : Message(SimpleSerial::Protocol::OperationType::SETTINGS, address)
            {
                SimpleSerial::Payload::IRSettings payload;

                memset(&payload, 0, sizeof(payload));

                if (config.CarrierHz.IsSet()) {
                    payload.carrier_hz = config.CarrierHz.Value();
                }

                Payload(sizeof(payload), reinterpret_cast<uint8_t*>(&payload));
            }

            SettingsMessage(const SimpleSerial::Protocol::DeviceAddressType address, const BLEConfig& config)
                : Message(SimpleSerial::Protocol::OperationType::SETTINGS, address)
            {
                SimpleSerial::Payload::BLESettings payload;
                uint8_t copyLength(0);

                memset(&payload, 0, sizeof(payload));

                if (config.VID.IsSet()) {
                    payload.vid = config.VID.Value();
                }

                if (config.PID.IsSet()) {
                    payload.pid = config.PID.Value();
                }

                if (config.Name.IsSet()) {
                    copyLength = std::min(static_cast<uint8_t>(config.Name.Value().size()), static_cast<uint8_t>(sizeof(payload.name)));
                    memcpy(&payload.name, config.Name.Value().c_str(), copyLength);
                }

                if (config.Manufacturer.IsSet()) {
                    copyLength = std::min(static_cast<uint8_t>(config.Manufacturer.Value().size()), static_cast<uint8_t>(sizeof(payload.manufacturer)));
                    memcpy(&payload.manufacturer, config.Manufacturer.Value().c_str(), copyLength);
                }

                Payload(sizeof(payload), reinterpret_cast<uint8_t*>(&payload));
            }
        };

        class ResetMessage : public Message {
        public:
            ResetMessage() = delete;
            ResetMessage(const ResetMessage&) = delete;
            ResetMessage& operator=(const ResetMessage&) = delete;

            ResetMessage(const SimpleSerial::Protocol::DeviceAddressType address)
                : Message(SimpleSerial::Protocol::OperationType::RESET, address)
            {
                PayloadLength(0);
            }
        };

    public:
        struct ICallback {
            virtual ~ICallback() = default;
            // @brief Signals that the endpoint is started
            virtual void Started() = 0;
        };

        SerialCommunicator()
            : _adminLock()
            , _channel(*this)
            , _callback(nullptr)
        {
        }
        SerialCommunicator(const SerialCommunicator&) = delete;
        SerialCommunicator& operator=(const SerialCommunicator&) = delete;

        virtual ~SerialCommunicator() = default;

        class EXTERNAL DeviceIterator : public Core::IteratorType<const std::list<SimpleSerial::Payload::Device>, const SimpleSerial::Payload::Device&, std::list<SimpleSerial::Payload::Device>::const_iterator> {
        private:
            using BaseClass = Core::IteratorType<const std::list<SimpleSerial::Payload::Device>, const SimpleSerial::Payload::Device&, std::list<SimpleSerial::Payload::Device>::const_iterator>;

        public:
            DeviceIterator()
                : BaseClass()
                , _container()
            {
                Container(_container);
            }
            DeviceIterator(std::list<SimpleSerial::Payload::Device>&& rhs)
                : BaseClass()
                , _container(rhs)
            {
                Container(_container);
            }
            DeviceIterator(const DeviceIterator& rhs)
                : BaseClass()
                , _container(rhs._container)
            {
                Container(_container);
            }
            DeviceIterator(DeviceIterator&& rhs)
                : BaseClass()
                , _container(rhs._container)
            {
                Container(_container);
            }
            ~DeviceIterator() override = default;

        private:
            std::list<SimpleSerial::Payload::Device> _container;
        };

        uint32_t Initialize(const std::string& config);
        void Deinitialize();

        DeviceIterator Devices() const;

        uint32_t KeyEvent(const SimpleSerial::Protocol::DeviceAddressType address, const bool pressed, const uint16_t code) const;

        uint32_t Reset(const SimpleSerial::Protocol::DeviceAddressType address) const;
        uint32_t Setup(const SimpleSerial::Protocol::DeviceAddressType address, const string& config) const;

        void Callback(ICallback* callback);

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
        mutable Core::CriticalSection _adminLock;
        mutable Channel _channel;
        ICallback* _callback;
    }; // class SerialCommunicator
} // namespace plugin
} // namespace Thunder
