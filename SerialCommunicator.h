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

#include "ICommunicator.h"

#include <list>

namespace WPEFramework {

namespace Doofah {
    class SerialCommunicator {
        friend class Core::SingletonType<SerialCommunicator>;

    private:
        static SimpleSerial::Protocol::SequenceType GetSequence()
        {
            static SimpleSerial::Protocol::SequenceType g_sequence = 0;
            return g_sequence++;
        }

        class Config : public Core::JSON::Container {
        private:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config()
                : Core::JSON::Container()
                , Connector(_T("/dev/ttyUSB0"))
                , BaudRate(115200)
                , FlowControl(Core::SerialPort::OFF)
            {
                Add(_T("connector"), &Connector);
                Add(_T("baudrate"), &BaudRate);
                Add(_T("flowcontrol"), &FlowControl);
            }
            ~Config()
            {
            }

        public:
            Core::JSON::String Connector;
            Core::JSON::DecUInt32 BaudRate;
            Core::JSON::EnumType<Core::SerialPort::FlowControl> FlowControl;
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
                Sequence(GetSequence());
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
                Sequence(GetSequence());
                Address(address);
            }

            inline bool IsValidPayload() const
            {
                return (_offset < PayloadLength());
            }

            typedef Core::IteratorType<const std::list<SimpleSerial::Payload::Device>, const SimpleSerial::Payload::Device&, std::list<SimpleSerial::Payload::Device>::const_iterator> DeviceIterator;

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
        public:
            SettingsMessage() = delete;
            SettingsMessage(const SettingsMessage&) = delete;
            SettingsMessage& operator=(const SettingsMessage&) = delete;

            SettingsMessage(const SimpleSerial::Protocol::DeviceAddressType address, const uint8_t length, const uint8_t payload[])
            {
                Clear();

                Operation(SimpleSerial::Protocol::OperationType::SETTINGS);
                Sequence(GetSequence());
                Address(address);
                PayloadLength(length);

                Deserialize(length, payload);
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
                Sequence(GetSequence());
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

        typedef Core::IteratorType<const std::list<SimpleSerial::Protocol::DeviceAddressType>, const SimpleSerial::Protocol::DeviceAddressType&, std::list<SimpleSerial::Protocol::DeviceAddressType>::const_iterator> DeviceIterator;

        uint32_t Configure(const std::string& config);

        DeviceIterator Devices();
        
        uint32_t KeyEvent(const SimpleSerial::Protocol::DeviceAddressType address, const bool pressed, const uint16_t code);
        
        uint32_t Reset(const SimpleSerial::Protocol::DeviceAddressType address);
        uint32_t Setup(const SimpleSerial::Protocol::DeviceAddressType address, const string& config);

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
