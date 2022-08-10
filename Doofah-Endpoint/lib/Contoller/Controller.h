#pragma once
#include <SimpleSerial.h>
#include <Storage.h>
#include <vector>

namespace Doofhah {
using namespace WPEFramework::SimpleSerial;

class Controller {
public:
    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;

    struct IDevice {
        virtual ~IDevice() = default;
        virtual const Payload::Peripheral Type() const = 0;
        virtual void Begin() = 0;

        virtual Protocol::ResultType KeyEvent(const Payload::KeyEvent& event) = 0;
        virtual Protocol::ResultType Reset() = 0;
        virtual Protocol::ResultType Setup(const uint8_t length, const uint8_t data[]) = 0;
    };

    typedef std::vector<IDevice*> DeviceList;

    static Controller& Instance();

    void Register(IDevice* device);
    void Unregister(IDevice* device);

    inline const DeviceList Devices() const
    {
        return _deviceRegister;
    }

    Protocol::ResultType KeyEvent(const Protocol::DeviceAddressType address, const Payload::KeyEvent& event);
    Protocol::ResultType Reset(const Protocol::DeviceAddressType address);
    Protocol::ResultType Setup(const Protocol::DeviceAddressType address, const uint8_t length, const uint8_t data[]);

    ~Controller() = default;

    void Reset();
    void StartDevices();
public:
    template <typename DEVICE>
    class Peripheral : public DEVICE {
    public:
        Peripheral(const Peripheral&) = delete;
        Peripheral& operator=(const Peripheral&) = delete;

        template <typename... Args>
        inline Peripheral(Args&&... args)
            : DEVICE(std::forward<Args>(args)...)
        {
            Controller::Instance().Register(this);
        }

        virtual ~Peripheral()
        {
            Controller::Instance().Unregister(this);
        }
    };

private:
    Controller();

private:
    DeviceList _deviceRegister;
}; // class Controller

} // namespace