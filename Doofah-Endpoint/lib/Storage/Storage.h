#pragma once

#include <EEPROM.h>

namespace Doofhah {
constexpr uint8_t FlashBaseAddress = 0x00;

class Storage {
public:
    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;

    static Storage& Instance();

    void Begin();
    void Reset();

    uint16_t Allocate(uint16_t length);
    uint16_t Read(const uint16_t address, const uint16_t length, uint8_t data[]);
    uint16_t Write(const uint16_t address, const uint16_t length, const uint8_t data[]);
    void Clear(const uint16_t address);

public:
    class Persistent {
    public:
        Persistent(const Persistent&) = delete;
        Persistent& operator=(const Persistent&) = delete;

        Persistent(const uint16_t size)
            : _address(Storage::Instance().Allocate(size))
        {
        }

        inline uint16_t Read(const uint16_t length, uint8_t data[]) const
        {
            return Storage::Instance().Read(_address, length, data);
        }

        inline uint16_t Write(const uint16_t length, const uint8_t data[])
        {
            return Storage::Instance().Write(_address, length, data);
        }

        inline void Clear()
        {
            return Storage::Instance().Clear(_address);
        }

    private:
        uint16_t _address;
    }; // class Persistent

private:
    Storage();

#ifdef __DEBUG__
    void DumpFlash();
#else
    void DumpFlash()
    {
    }
#endif

private:
    EEPROMClass _flash;
    uint16_t _address;
}; // class Storage
} // namespace