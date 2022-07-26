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

#include <algorithm>
#include <cstring>
#include <stdint.h>

#ifndef ASSERT
#define ASSERT(x)
#endif

namespace WPEFramework {
namespace SimpleSerial {
    namespace Protocol {
        // Super Simple Serial Protocol
        //
        // Request from Host
        // | OperationType | SequenceType | DeviceAddressType | LengthType | DataType   | CRC8   |
        //
        // Response from target
        // | OperationType | SequenceType | ResultType        | LengthType | DataType   | CRC8   |
        //
        // Request from Host
        // |0x01|0x00|0x12|0x00|CRC8|
        // Response from target
        // |0x01|0x00|0x01|0x00|CRC8|
        //
        // Request from Host
        // |0x01|0x01|0x13|0x00|CRC8|
        // Response from target
        // |0x01|0x01|0x01|0x00|CRC8|

        enum class OperationType : uint8_t {
            RESET = 0x01, // reset/(dis)enable controller/peripheral
            ALLOCATE,
            FREE,
            KEY, // Do a key action press/release + keycode
            SETTINGS, // Send/Retrieve (VID/PID/NAME)
            STATE, // Get the state of all devices
            EVENT = 0x80 //
        };

        typedef uint8_t SequenceType;
        typedef uint8_t DeviceAddressType;
        typedef uint8_t LengthType;
        typedef uint8_t* DataType;
        typedef uint8_t CRC8Type;

        constexpr uint8_t InvalidAddress = DeviceAddressType(~0);

        enum class ResultType : uint8_t {
            OK = 0x00,
            NOT_CONNECTED,
            NOT_AVAILABLE,
            TRANSMIT_FAILED,
            CRC_INVALID,
            OPERATION_INVALID,
        };

        constexpr uint8_t MaxDataSize = 0xFF;
        constexpr uint8_t MaxPayloadSize = MaxDataSize - (sizeof(SequenceType) + sizeof(OperationType) + sizeof(DeviceAddressType) + sizeof(LengthType) + sizeof(CRC8Type));
        constexpr uint8_t HeaderSize = sizeof(OperationType) + sizeof(SequenceType) + sizeof(DeviceAddressType) + sizeof(LengthType);

        static uint8_t CRC8(const uint8_t length, const uint8_t data[])
        {
            uint8_t crc(~0);

            uint8_t i, j;

            for (i = 0; i < length; i++) {
                crc ^= data[i];
                for (j = 0; j < 8; j++) {
                    if ((crc & 0x80) != 0)
                        crc = static_cast<uint8_t>(((crc << 1) ^ 0x31));
                    else
                        crc <<= 1;
                }
            }

            return crc;
        }

        struct Message {
            uint8_t _buffer[MaxDataSize];
            uint8_t _size;
            mutable uint8_t _offset;

            void Clear()
            {
                std::memset(_buffer, 0, sizeof(_buffer));
                _size = 0;
                _offset = 0;
            }

            uint16_t Deserialize(const uint16_t length, const uint8_t data[])
            {
                uint16_t result(0);

                if (length > 0) {
                    if (_size < HeaderSize) {
                        result = std::min(length, uint16_t(HeaderSize - _size));

                        std::memcpy(&_buffer[_size], data, result);

                        _size += result;
                    }

                    if ((_size >= HeaderSize) && (result < length)) {
                        uint8_t copyLength = std::min(uint8_t(length - result), uint8_t(_size - HeaderSize + PayloadLength() + sizeof(CRC8Type)));

                        std::memcpy(&_buffer[_size], &(data[result]), copyLength);

                        _size += copyLength;
                        result += copyLength;
                    }
                }

                return result;
            }
            uint16_t Serialize(uint16_t length, uint8_t data[]) const
            {
                uint16_t copyLength = std::min(length, uint16_t(_size - _offset));

                ASSERT(IsComplete() == true);

                if (copyLength > 0) {
                    std::memcpy(data, &_buffer[_offset], copyLength);
                    _offset += copyLength;
                }

                return copyLength;
            }

            bool IsComplete() const
            {
                return ((_size > HeaderSize) && (_size >= (HeaderSize + PayloadLength() + sizeof(CRC8Type))));
            }
            bool IsValid() const
            {
                return (IsComplete() && CRC8((HeaderSize + PayloadLength()), _buffer) == _buffer[(HeaderSize + PayloadLength())]);
            }

            inline uint8_t Size() const
            {
                return _size;
            }

            inline const uint8_t* Data() const
            {
                return _buffer;
            }

            inline DeviceAddressType Address() const
            {
                ASSERT(_size >= 3);
                return static_cast<DeviceAddressType>(_buffer[2]);
            }
            inline void Address(const DeviceAddressType address)
            {
                if (_size < 3) {
                    _size = 3;
                }
                _buffer[2] = static_cast<uint8_t>(address);
            }

            inline OperationType Operation() const
            {
                ASSERT(_size >= 1);
                return static_cast<OperationType>(_buffer[0]);
            }
            inline void Operation(const OperationType operation)
            {
                if (_size < 1) {
                    _size = 1;
                }
                _buffer[0] = static_cast<uint8_t>(operation);
            }

            inline LengthType PayloadLength() const
            {
                ASSERT(_size >= 4);
                return static_cast<LengthType>(_buffer[3]);
            }
            inline void PayloadLength(const LengthType length)
            {
                if (_size < 4) {
                    _size = 4;
                }
                _buffer[3] = static_cast<uint8_t>(length);
            }

            inline const uint8_t* Payload() const
            {
                return &_buffer[HeaderSize];
            }

            inline uint8_t Payload(const uint8_t length, const uint8_t data[])
            {
                uint8_t result(0);

                if ((length > 0) && (length <= MaxPayloadSize)) {
                    if (_size < (HeaderSize + length)) {
                        _size = (HeaderSize + length);
                    }

                    _buffer[3] = static_cast<uint8_t>(length);

                    memcpy(&_buffer[HeaderSize], data, length);
                }

                return result;
            }

            inline ResultType Result() const
            {
                ASSERT(_size >= 3);
                return static_cast<ResultType>(_buffer[2]);
            }
            inline void Result(const ResultType result)
            {
                if (_size < 3) {
                    _size = 3;
                }
                _buffer[2] = static_cast<uint8_t>(result);
            }

            inline SequenceType Sequence() const
            {
                ASSERT(_size >= 2);
                return static_cast<SequenceType>(_buffer[1]);
            }
            inline void Sequence(const SequenceType sequence)
            {
                if (_size < 2) {
                    _size = 2;
                }
                _buffer[1] = static_cast<uint8_t>(sequence);
            }

            inline uint8_t Finalize()
            {
                CRC8Type crc(0);

                if ((_size >= HeaderSize) && (_size >= (HeaderSize + PayloadLength()))) {
                    crc = CRC8((HeaderSize + PayloadLength()), _buffer);
                    _buffer[HeaderSize + PayloadLength()] = crc;
                    _size = HeaderSize + PayloadLength() + sizeof(CRC8Type);
                }

                // Ready to be send...
                _offset = 0;

                return crc;
            }
        };
    } // namespace Protocol

    namespace Payload {
        enum class Action : uint8_t {
            RELEASED = 0x00,
            PRESSED = 0x01
        };

        enum class PeripheralState : uint8_t {
            UNINITIALIZED = 0x01,
            AVAILABLE = 0x02,
            OCCUPIED = 0x03
        };

        enum class Peripheral : uint8_t {
            ROOT = 0x00,
            IR = 0x20,
            BLE = 0x40
        };

#pragma pack(push, 1)
        typedef struct KeyEvent {
            Action pressed;
            uint16_t code;
        } KeyEvent;

        typedef struct BLESettings {
            uint16_t vid;
            uint16_t pid;
            char* name;
        } BLESettings;

        //
        // TODO:
        // Somehow we need to be able to register keyCodes to ProntoHex codes or
        // add a learn function to the plugin.
        // more info:
        // http://www.hifi-remote.com/wiki/index.php/Working_With_Pronto_Hex
        // http://www.remotecentral.com/features/irdisp2.htm
        //
        // tools for analysing IR and generating ProntoHex.
        // http://www.harctoolbox.org/IrScrutinizer.html (Opensource)
        // https://www.analysir.com/ (Paid)
        //
        typedef struct IRSettings {
            uint16_t carrier_hz;
        } IRSettings;

        typedef struct Device {
            Protocol::DeviceAddressType address;
            PeripheralState state;
            Peripheral peripheral;
        } Device;

        typedef struct State {
            Device* devices;
        } State;
#pragma pack(pop)

    } // namespace Payload
} // namespace SimpleSerial
} // namespace WPEFramework
