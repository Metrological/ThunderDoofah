#pragma once

#include <IRremote.hpp>

#include "Controller.h"

#undef IR_SEND_PIN

namespace Doofhah {
class IRKeyboardDevice : public Controller::IDevice {
public:
    template <typename... Args>
    inline IRKeyboardDevice(Args&&... args)
        : _device(std::forward<Args>(args)...)
        , _persistent(sizeof(Payload::IRSettings))
        , frequencyKHz(36)
    {
    }

    virtual ~IRKeyboardDevice() = default;

    inline const Payload::Peripheral Type() const
    {
        return Payload::Peripheral::IR;
    }

    Protocol::ResultType KeyEvent(const Payload::KeyEvent& event)
    {
        return Protocol::ResultType::UNSUPPORTED;
    };

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

    void Begin()
    {
        Payload::IRSettings settings;
        memset(&settings, 0, sizeof(settings));

        _persistent.Read(sizeof(settings), reinterpret_cast<uint8_t*>(&settings));

        if (settings.carrier_hz > 0) {
            frequencyKHz = settings.carrier_hz / 1000;
        }

        TRACE("Started IR [%d]", _device.sendPin);
    }

private:
    IRsend _device;
    Storage::Persistent _persistent;
    uint8_t frequencyKHz;
};
}