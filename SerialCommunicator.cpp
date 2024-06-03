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

namespace Thunder {

ENUM_CONVERSION_BEGIN(Core::SerialPort::FlowControl) { Core::SerialPort::OFF, _TXT("off") },
    { Core::SerialPort::HARDWARE, _TXT("hardware") },
    { Core::SerialPort::SOFTWARE, _TXT("software") },
    ENUM_CONVERSION_END(Core::SerialPort::FlowControl);

namespace Doofah {
    void SerialCommunicator::Callback(ICallback* callback)
    {
        _adminLock.Lock();
        ASSERT((callback == nullptr) ^ (_callback == nullptr));
        _callback = callback;
        _adminLock.Unlock();
    }

    uint32_t SerialCommunicator::Initialize(const string& configuration)
    {
        uint32_t result = Core::ERROR_NONE;
        SerialConfig config;
        TRACE(Trace::Information, ("Configure SerialCommunicator: %s", configuration.c_str()));

        config.FromString(configuration);

        if (_channel.Link().Configuration(
                config.Port.Value(),
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

        if (_channel.IsOpen() == true) {
            _channel.Flush();
        }

        TRACE(Trace::Information, ("Configured SerialCommunicator[%s]: %s", _channel.RemoteId().c_str(), _channel.IsOpen() ? "succesful" : "failed"));

        return result;
    }

    void SerialCommunicator::Deinitialize()
    {
        if (_channel.IsOpen() == true) {
            _channel.Flush();
            _channel.Close(1000);
        }
    }

    uint32_t SerialCommunicator::KeyEvent(const SimpleSerial::Protocol::DeviceAddressType address, const bool pressed, const uint16_t code) const
    {
        KeyMessage message(address, code, pressed);

        uint32_t result = _channel.Post(message, 1000);

        if ((result == Core::ERROR_NONE) && (message.Result() != SimpleSerial::Protocol::ResultType::OK)) {
            TRACE(Trace::Error, ("Exchange Failed: %d", static_cast<uint8_t>(message.Result())));
            result = Core::ERROR_GENERAL;
        }

        return result;
    }

    SerialCommunicator::DeviceIterator SerialCommunicator::Devices() const
    {
        std::list<SimpleSerial::Payload::Device> devices;

        StateMessage message(static_cast<SimpleSerial::Protocol::DeviceAddressType>(SimpleSerial::Payload::Peripheral::ROOT));

        uint32_t result = _channel.Post(message, 1000);

        if (result != Core::ERROR_NONE) {
            TRACE(Trace::Error, ("Post Failed: %d", result));
        } else if ((result == Core::ERROR_NONE) && (message.Result() != SimpleSerial::Protocol::ResultType::OK)) {
            TRACE(Trace::Error, ("Exchange Failed: %d", static_cast<uint8_t>(message.Result())));
            result = Core::ERROR_GENERAL;
        } else {
            message.Reset();

            while ((message.Next() == true) && (message.Current() != nullptr)) {
                SimpleSerial::Payload::Device device;

                device.address = message.Current()->address;
                device.peripheral = message.Current()->peripheral;

                devices.push_back(device);
            }

            TRACE(Trace::Information, ("Got %d devices", devices.size()));
        }

        return DeviceIterator(std::move(devices));
    }

    void SerialCommunicator::Received(const SimpleSerial::Protocol::Message& message)
    {
        TRACE(Trace::Information, ("Received message: 0x%02X", message.Operation()));
        SimpleSerial::PrintMessage(message);
    }

    uint32_t SerialCommunicator::Reset(const SimpleSerial::Protocol::DeviceAddressType address) const
    {
        uint32_t result(Core::ERROR_NONE);

        TRACE(Trace::Information, ("Reset device: 0x%02X", address));

        ResetMessage message(address);

        result = _channel.Post(message, 1000);

        if ((result == Core::ERROR_NONE) && (message.Result() != SimpleSerial::Protocol::ResultType::OK)) {
            TRACE(Trace::Error, ("Exchange Failed: %d", static_cast<uint8_t>(message.Result())));
            result = Core::ERROR_GENERAL;
        }

        return result;
    }
    uint32_t SerialCommunicator::Setup(const SimpleSerial::Protocol::DeviceAddressType address, const string& config) const
    {
        uint32_t result(Core::ERROR_INCOMPLETE_CONFIG);

        SetupConfig SetupConfig;
        SetupConfig.FromString(config);

        TRACE(Trace::Information, ("Setup device: 0x%02X", address));

        if ((SetupConfig.Type.IsSet() == true) && (SetupConfig.Type.Value() == SimpleSerial::Payload::Peripheral::ROOT)) {
            result = Core::ERROR_NOT_SUPPORTED;
        } else if ((SetupConfig.Type.IsSet() == true) && (SetupConfig.Type.Value() == SimpleSerial::Payload::Peripheral::BLE)) {
            BLEConfig bleConfig;
            bleConfig.FromString(SetupConfig.Configuration.Value());

            SettingsMessage bleSettings(address, bleConfig);

            result = _channel.Post(bleSettings, 1000);

            if ((result == Core::ERROR_NONE) && (bleSettings.Result() != SimpleSerial::Protocol::ResultType::OK)) {
                TRACE(Trace::Error, ("Exchange BLE settings Failed: %d", static_cast<uint8_t>(bleSettings.Result())));
            }
        } else if ((SetupConfig.Type.IsSet() == true) && (SetupConfig.Type.Value() == SimpleSerial::Payload::Peripheral::IR)) {
            IRConfig irConfig;
            irConfig.FromString(SetupConfig.Configuration.Value());

            SettingsMessage irSettings(address, irConfig);

            result = _channel.Post(irSettings, 1000);

            if ((result == Core::ERROR_NONE) && (irSettings.Result() != SimpleSerial::Protocol::ResultType::OK)) {
                TRACE(Trace::Error, ("Exchange IR settings Failed: %d", static_cast<uint8_t>(irSettings.Result())));
            }
        }

        return result;
    }
} // namespace Doofah
} // namespace Thunder
