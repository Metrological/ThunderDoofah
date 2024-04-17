#include <Controller.h>

#include <Log.h>

namespace Doofhah {
using namespace Thunder::SimpleSerial;

Controller& Controller::Instance()
{
    static Controller instance;
    return instance;
}

void Controller::Register(IDevice* device)
{
    auto index = std::find(_deviceRegister.begin(), _deviceRegister.end(), device);
    ASSERT(index == _deviceRegister.end());

    if (index == _deviceRegister.end()) {
        _deviceRegister.push_back(device);
    }
}
void Controller::Unregister(IDevice* device)
{
    auto index(std::find(_deviceRegister.begin(), _deviceRegister.end(), device));

    if (index != _deviceRegister.end()) {
        _deviceRegister.erase(index);
    }
}

Protocol::ResultType Controller::KeyEvent(const Protocol::DeviceAddressType address, const Payload::KeyEvent& event)
{
    Protocol::ResultType result(Protocol::ResultType::NOT_AVAILABLE);

    if (address <= _deviceRegister.size()) {
        result = _deviceRegister[address]->KeyEvent(event);
    }

    TRACE();

    return result;
}
Protocol::ResultType Controller::Reset(const Protocol::DeviceAddressType address)
{
    Protocol::ResultType result(Protocol::ResultType::NOT_AVAILABLE);

    TRACE("Address=0x%02X", address);

    if (address <= _deviceRegister.size()) {
        result = _deviceRegister[address]->Reset();
    }

    return result;
}
Protocol::ResultType Controller::Setup(const Protocol::DeviceAddressType address, const uint8_t length, const uint8_t data[])
{
    Protocol::ResultType result(Protocol::ResultType::NOT_AVAILABLE);

    TRACE("Address=0x%02X", address);

    if (address < _deviceRegister.size()) {
        result = _deviceRegister[address]->Setup(length, data);
    }

    return result;
    return Protocol::ResultType::OK;
}

void Controller::Reset()
{
    TRACE();
}

void Controller::StartDevices()
{
    //_storage.Begin();

    TRACE("Starting %d devices", _deviceRegister.size());

    for (auto& device : _deviceRegister) {
        device->Begin();
    }
}

Controller::Controller()
    : _deviceRegister()
{}

} // Controller namespace