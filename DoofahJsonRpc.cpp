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

#include <interfaces/json/JsonData_RemoteControl.h>

namespace WPEFramework {
namespace Plugin {

    using namespace JsonData::RemoteControl;

    void Doofah::RegisterAll()
    {
        Register<KeyobjInfo, void>(_T("press"), &Doofah::endpoint_press, this);
        Register<KeyobjInfo, void>(_T("release"), &Doofah::endpoint_release, this);
    }

    void Doofah::UnregisterAll()
    {
        Unregister(_T("release"));
        Unregister(_T("press"));
    }

    uint32_t Doofah::endpoint_press(const KeyobjInfo& params)
    {
        uint32_t result = Core::ERROR_NONE;

        if ((params.Device.IsSet() == true) && (params.Code.IsSet() == true)) {
            // if (_implementation != nullptr) {
            //     result = _implementation->KeyEvent(true, params.Code.Value(), params.Device.Value());
            // } else {
            //     result = Core::ERROR_UNAVAILABLE;
            // }
        } else {
            result = Core::ERROR_BAD_REQUEST;
        }
        return result;
    }

    uint32_t Doofah::endpoint_release(const KeyobjInfo& params)
    {
        uint32_t result = Core::ERROR_NONE;

        if ((params.Device.IsSet() == true) && (params.Code.IsSet() == true)) {
            // if (_implementation != nullptr) {
            //     result = _implementation->KeyEvent(false, params.Code.Value(), params.Device.Value());
            // } else {
            //     result = Core::ERROR_UNAVAILABLE;
            // }
        } else {
            result = Core::ERROR_BAD_REQUEST;
        }

        return result;
    }

    // Event: keypressed - Notifies of a key press/release action
    void Doofah::event_keypressed(const string& id, const bool& pressed)
    {
        KeypressedParamsData params;
        params.Pressed = pressed;

        Notify(_T("keypressed"), params, [&](const string& designator) -> bool {
            const string designator_id = designator.substr(0, designator.find('.'));
            return (id == designator_id);
        });
    }
} // namespace Plugin
} // namespace WPEFramework
