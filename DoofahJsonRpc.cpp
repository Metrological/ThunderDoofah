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

#include "Module.h"

#include "Doofah.h"

#include "JsonData_Doofah.h"

namespace WPEFramework {
namespace Plugin {
    using namespace JsonData::Doofah;

    // Registration
    void Doofah::JSONRPCRegister()
    {
        Property<Core::JSON::ArrayType<DeviceEntry>>(_T("devices"), &Doofah::JSONRPCDevices, nullptr, this);
        Register<SetupInfo, void>(_T("setup"), &Doofah::JSONRPCSetup, this);
        Register<AddressInfo, void>(_T("reset"), &Doofah::JSONRPCReset, this);
        Register<KeyInfo, void>(_T("press"), &Doofah::JSONRPCKeyPress, this);
        Register<KeyInfo, void>(_T("release"), &Doofah::JSONRPCKeyRelease, this);
    }
    void Doofah::JSONRPCUnregister()
    {
        Unregister(_T("devices"));
        Unregister(_T("setup"));
        Unregister(_T("reset"));
        Unregister(_T("release"));
        Unregister(_T("press"));
    }

    uint32_t Doofah::JSONRPCKeyPress(const KeyInfo& params)
    {
        uint32_t result = Core::ERROR_NONE;

        if ((params.Device.IsSet() == true) && (params.Code.IsSet() == true)) {
            result = _communicator.KeyEvent(params.Device.Value(), true, params.Code.Value());
        } else {
            result = Core::ERROR_BAD_REQUEST;
        }
        return result;
    }

    uint32_t Doofah::JSONRPCKeyRelease(const KeyInfo& params)
    {
        uint32_t result = Core::ERROR_NONE;

        if ((params.Device.IsSet() == true) && (params.Code.IsSet() == true)) {
            result = _communicator.KeyEvent(params.Device.Value(), false, params.Code.Value());
        } else {
            result = Core::ERROR_BAD_REQUEST;
        }

        return result;
    }

    uint32_t Doofah::JSONRPCSetup(const SetupInfo& params)
    {
        uint32_t result = Core::ERROR_NONE;

        if ((params.Device.IsSet() == true) && (params.Configuration.IsSet() == true)) {
            result = _communicator.Setup(params.Device.Value(), params.Configuration.Value());
        } else {
            result = Core::ERROR_BAD_REQUEST;
        }

        return result;
    }

    uint32_t Doofah::JSONRPCReset(const AddressInfo& params)
    {
        uint32_t result = Core::ERROR_NONE;

        if (params.Address.IsSet() == true) {
            result = _communicator.Reset(params.Address.Value());
        } else {
            result = Core::ERROR_BAD_REQUEST;
        }

        return result;
    }

    // Property: devices - Available devices
    // Return codes:
    //  - ERROR_NONE: Success
    uint32_t Doofah::JSONRPCDevices(Core::JSON::ArrayType<DeviceEntry>& response) const
    {
        WPEFramework::Doofah::SerialCommunicator::DeviceIterator list = _communicator.Devices();

        while (list.Next() == true) {
            DeviceEntry info;
            Doofah::FillDeviceInfo(list.Current(), info);
            response.Add(info);
        }

        return Core::ERROR_NONE;
    }

    // Event: keypressed - Notifies of a key press/release action
    void Doofah::EventKeyPressed(const string& id, const bool& pressed)
    {
        KeypressedParamsData params;
        params.Pressed = pressed;

        Notify(_T("keypressed"), params, [&](const string& designator) -> bool {
            const string designator_id = designator.substr(0, designator.find('.'));
            return (id == designator_id);
        });
    }

    // Event: started - Notifies when the endpoint is ready after a (re)start.
    void Doofah::EventStarted()
    {
        string message("{ \"event\": \"started\" }");
        if (_service != nullptr) {
            _service->Notify(message);
        }

        Notify(_T("started"));
    }
} // namespace Plugin
} // namespace WPEFramework
