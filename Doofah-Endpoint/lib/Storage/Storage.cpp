#include "Storage.h"

#include <Log.h>

namespace Doofhah {

Storage::Storage()
    : _flash("doofah")
    , _address(FlashBaseAddress)
{
}

Storage& Storage::Instance()
{
    static Storage instance;
    return instance;
}

uint16_t Storage::Allocate(uint16_t length)
{
    const uint16_t address(_address);

    _address += sizeof(length) + length;

    _flash.writeUShort(address, length);
    _flash.commit();

    TRACE("%d bytes avalaible on address %d", length, address);

    return address;
}

uint16_t Storage::Read(const uint16_t address, const uint16_t length, uint8_t data[])
{
    uint16_t size = _flash.readUShort(address);

    if (size > 0) {
        if (size == length) {
            _flash.readBytes(address + sizeof(uint16_t), data, size);
        } else {
            TRACE("Flash corrupt");
        }
    } else {
        TRACE("No data");
    }

    return size;
}

uint16_t Storage::Write(const uint16_t address, const uint16_t length, const uint8_t data[])
{
    size_t written(0);

    written += _flash.writeUShort(address, length);
    written += _flash.writeBytes((address + written), data, length);

    if (_flash.commit() == true) {
        TRACE("%d bytes written to flash address %d...", written, address);
    } else {
        TRACE("No data written to flash...");
    }

    return written;
}

void Storage::Clear(const uint16_t address)
{
    uint16_t length = _flash.readUShort(address);
    
    uint16_t offset(address);

    offset += _flash.writeUShort(offset, 0);

    for (uint16_t i(0); i < length; i++) {
        offset += _flash.writeByte(offset + i, 0x00);
    }

    if (_flash.commit() == true) {
        TRACE("%d bytes flushed from address %d...", offset, address);
    } else {
        TRACE("No data flushed...");
    }
}

void Storage::Begin()
{
    TRACE();
    _flash.begin(_address);
    TRACE("Starting Storage with max %d bytes", _flash.length());
    DumpFlash();
}

void Storage::Reset()
{
    uint16_t erased(0);

    for (uint16_t i(0); i < _address; i++) {
        erased += _flash.writeByte(i, 0x00);
    }

    if (_flash.commit()) {
        TRACE("Cleared %d bytes flash.", erased);
    }
}

#ifdef __DEBUG__
void Storage::DumpFlash()
{
    uint8_t data[_flash.length()];
    size_t rd = _flash.readBytes(FlashBaseAddress, data, sizeof(data));

    std::string sdata;
    ToHexString(data, rd, sdata);

    TRACE("Flash[%d]: '%s'", rd, sdata.c_str());
}
#endif
}