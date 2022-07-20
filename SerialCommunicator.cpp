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

#include "SerialCommunicator.h"

#include "DataExchange.h"
#include "SimpleSerial.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(Core::SerialPort::FlowControl) { Core::SerialPort::OFF, _TXT("off") },
    { Core::SerialPort::HARDWARE, _TXT("hardware") },
    { Core::SerialPort::SOFTWARE, _TXT("software") },
    ENUM_CONVERSION_END(Core::SerialPort::FlowControl);

namespace Doofah {
    uint32_t SerialCommunicator::Configure(const string& configuration)
    {
        uint32_t result = Core::ERROR_NONE;
        Config config;

        config.FromString(configuration);

        if (_channel.Link().Configuration(
                config.Connector.Value(),
                Core::SerialPort::Convert(config.BaudRate.Value()),
                Core::SerialPort::NONE,
                Core::SerialPort::BITS_8,
                Core::SerialPort::BITS_1,
                config.FlowControl.Value())
            == Core::ERROR_NONE) {
            result = _channel.Open(1000);
        } else {
            result = Core::ERROR_INCOMPLETE_CONFIG;
        }

        return result;
    }

    uint32_t SerialCommunicator::KeyEvent(const SimpleSerial::Protocol::DeviceAddressType address, const bool pressed, const uint16_t code)
    {
        KeyMessage message(address, code, pressed);

        uint32_t result = _channel.Post(message, 1000);

        if ((result == Core::ERROR_NONE) && (message.Result() != SimpleSerial::Protocol::ResultType::OK)) {
            TRACE(Trace::Error, ("Exchange Failed: %d\n", static_cast<uint8_t>(message.Result())));
            result = Core::ERROR_GENERAL;
        }

        return result;
    }

    SerialCommunicator::DeviceIterator SerialCommunicator::Devices()
    {
        std::list<SimpleSerial::Payload::Device> devices;

        StateMessage message(static_cast<SimpleSerial::Protocol::DeviceAddressType>(SimpleSerial::Payload::Peripheral::ROOT));

        uint32_t result = _channel.Post(message, 1000);

        if ((result == Core::ERROR_NONE) && (message.Result() != SimpleSerial::Protocol::ResultType::OK)) {
            TRACE(Trace::Error, ("Exchange Failed: %d\n", static_cast<uint8_t>(message.Result())));
            result = Core::ERROR_GENERAL;
        }

        if (result == Core::ERROR_NONE) {
            message.Reset();

            while ((message.Next() == true) && (message.Current() != nullptr)) {
                SimpleSerial::Payload::Device device;

                device.address = message.Current()->address;
                device.state = message.Current()->state;
                device.peripheral = message.Current()->peripheral;

                devices.push_back(device);
            }
        }

        return DeviceIterator(devices);
    }

    void SerialCommunicator::Received(const SimpleSerial::Protocol::Message& message)
    {
    }

    uint32_t SerialCommunicator::Reset(const SimpleSerial::Protocol::DeviceAddressType address)
    {
        uint32_t result(Core::ERROR_NONE);

        ResetMessage message(address);

        result = _channel.Post(message, 1000);

        if ((result == Core::ERROR_NONE) && (message.Result() != SimpleSerial::Protocol::ResultType::OK)) {
            TRACE(Trace::Error, ("Exchange Failed: %d\n", static_cast<uint8_t>(message.Result())));
            result = Core::ERROR_GENERAL;
        }

        return result;
    }
    uint32_t SerialCommunicator::Setup(const SimpleSerial::Protocol::DeviceAddressType address, const string& config)
    {
        uint32_t result(Core::ERROR_NONE);

        // SettingsMessage message(address);

        // result = _channel.Post(message, 1000);

        // if ((result == Core::ERROR_NONE) && (message.Result() != SimpleSerial::Protocol::ResultType::OK)) {
        //     TRACE(Trace::Error, ("Exchange Failed: %d\n", static_cast<uint8_t>(message.Result())));
        //     result = Core::ERROR_GENERAL;
        // }

        return result;
    }

    uint32_t SerialCommunicator::Resource(const SimpleSerial::Protocol::DeviceAddressType address, const bool& aquire)
    {
        uint32_t result(Core::ERROR_NONE);

        ResourceMessage message(address, aquire);

        result = _channel.Post(message, 1000);

        if ((result == Core::ERROR_NONE) && (message.Result() != SimpleSerial::Protocol::ResultType::OK)) {
            TRACE(Trace::Error, ("Exchange Failed: %d\n", static_cast<uint8_t>(message.Result())));
            result = Core::ERROR_GENERAL;
        }

        return result;
    }
} // namespace Doofah
} // namespace WPEFramework
