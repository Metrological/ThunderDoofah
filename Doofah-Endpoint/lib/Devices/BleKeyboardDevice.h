#pragma once

#include "Controller.h"
#include "Log.h"
#include <BleKeyboard.h>
#include <NimBLEDevice.h>

namespace Doofhah {
class BleKeyboardDevice : public Controller::IDevice {
public:
    template <typename... Args>
    inline BleKeyboardDevice(Args&&... args)
        : _device(std::forward<Args>(args)...)
        , _persistent(sizeof(Payload::BLESettings))

    {
    }

    ~BleKeyboardDevice() = default;

    Protocol::ResultType KeyEvent(const Payload::KeyEvent& event)
    {
        uint8_t keycode(event.code & 0xff);

        TRACE("BLE KeyEvent keycode=0x%04X code=0x%04X action=0x%04X", keycode, event.code, event.pressed);

        if (event.pressed == Payload::Action::PRESSED) {
            _device.press(keycode);
        } else {
            _device.release(keycode);
        }

        return Protocol::ResultType::OK;
    }
    
    Protocol::ResultType Reset()
    {
        TRACE();
        _persistent.Clear();
        return Protocol::ResultType::OK;
    }
    
    Protocol::ResultType Setup(const uint8_t length, const uint8_t data[])
    {
        TRACE();
        _persistent.Write(length, data);
        return Protocol::ResultType::OK;
    }

    inline const Payload::Peripheral Type() const
    {
        return Payload::Peripheral::BLE;
    }

    inline void Battery(uint8_t percentage)
    {
        _device.setBatteryLevel(percentage);
        TRACE("BLE  set batery level %d%", percentage);
    }

    inline void Begin()
    {
        Payload::BLESettings settings;
        memset(&settings, 0, sizeof(settings));

        _persistent.Read(sizeof(settings), reinterpret_cast<uint8_t*>(&settings));

        if (strlen(settings.name) > 0) {
            _device.setName(settings.name);
            TRACE("BLE name %s loaded from flash", settings.name);
        }

        if (settings.vid > 0) {
            _device.set_vendor_id(settings.vid);
            TRACE("BLE vendor ID 0x%04X loaded from flash", settings.vid);
        }

        if (settings.pid > 0) {
            _device.set_product_id(settings.pid);
            TRACE("BLE product ID 0x%04X loaded from flash", settings.pid);
        }

        _device.begin();

        NimBLEAddress address = NimBLEDevice::getAddress();

        TRACE("Started BLE [%s]", address.toString().c_str());
    }

private:
    BleKeyboard _device;
    Storage::Persistent _persistent;
};
}