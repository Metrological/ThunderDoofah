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

#include <core/Enumerate.h>
#include <core/JSON.h>

#include "SimpleSerial.h"

namespace WPEFramework {
namespace JsonData {
    namespace Doofah {
        class KeyInfo : public Core::JSON::Container {
        public:
            KeyInfo()
                : Core::JSON::Container()
            {
                Add(_T("device"), &Device);
                Add(_T("code"), &Code);
            }

            KeyInfo(const KeyInfo&) = delete;
            KeyInfo& operator=(const KeyInfo&) = delete;

        public:
            Core::JSON::HexUInt8 Device; // Device address
            Core::JSON::DecUInt32 Code; // Key code
        }; // class KeyInfo

        class DeviceInfo : public Core::JSON::Container {
        public:
            DeviceInfo()
                : Core::JSON::Container()
            {
                Add(_T("device"), &Device);
            }

            DeviceInfo(const DeviceInfo&) = delete;
            DeviceInfo& operator=(const DeviceInfo&) = delete;

        public:
            Core::JSON::HexUInt8 Device; // Device 
        }; // class DeviceInfo

        class SetupInfo : public Core::JSON::Container {
        public:
            SetupInfo()
                : Core::JSON::Container()
            {
                Add(_T("device"), &Device);
                Add(_T("configuration"), &Configuration);
            }

            SetupInfo(const SetupInfo&) = delete;
            SetupInfo& operator=(const SetupInfo&) = delete;

        public:
            Core::JSON::HexUInt8 Device; // Device address
            Core::JSON::String Configuration; // Configuration string
        }; // class SetupInfo

        class KeypressedParamsData : public Core::JSON::Container {
        public:
            KeypressedParamsData()
                : Core::JSON::Container()
            {
                Add(_T("pressed"), &Pressed);
            }

            KeypressedParamsData(const KeypressedParamsData&) = delete;
            KeypressedParamsData& operator=(const KeypressedParamsData&) = delete;

        public:
            Core::JSON::Boolean Pressed; // Denotes if the key was pressed (true) or released (false)
        }; // class KeypressedParamsData

        class ConnectedParamsData : public Core::JSON::Container {
        public:
            ConnectedParamsData()
                : Core::JSON::Container()
            {
                Add(_T("connected"), &Connected);
            }

            ConnectedParamsData(const ConnectedParamsData&) = delete;
            ConnectedParamsData& operator=(const ConnectedParamsData&) = delete;

        public:
            Core::JSON::Boolean Connected; // Denotes if the key was pressed (true) or released (false)
        }; // class ConnectedParamsData

        class DeviceEntry : public Core::JSON::Container {
        public:
            inline DeviceEntry()
                : Core::JSON::Container()
            {
                Init();
            }
            inline DeviceEntry(const DeviceEntry& copy)
                : Core::JSON::Container()
                , Device(copy.Device)
                , Peripheral(copy.Peripheral)
            {
                Init();
            }
            DeviceEntry& operator=(const DeviceEntry& rhs)
            {
                Device = rhs.Device;
                Peripheral = rhs.Peripheral;
                return (*this);
            };

            ~DeviceEntry() override = default;

        private:
            void Init()
            {
                Add(_T("device"), &Device);
                Add(_T("peripheral"), &Peripheral);
            }

        public:
            Core::JSON::HexUInt16 Device;
            Core::JSON::EnumType<SimpleSerial::Payload::Peripheral> Peripheral;
        }; // class DeviceEntry
    } // namespace Doofah
} // namespace JsonData
} // namespace WPEFramework