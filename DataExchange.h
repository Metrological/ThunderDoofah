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

#include "SimpleSerial.h"

#include "Tracing.h"

namespace WPEFramework {
namespace SimpleSerial {
    static Protocol::SequenceType GetSequence()
    {
        static Protocol::SequenceType g_sequence = 0;
        TRACE_GLOBAL(Doofah::DataExchangeFlow, (_T("Provided sequence id: 0x%02X(%d)"), g_sequence, g_sequence));
        return g_sequence++;
    }

    template <typename LINK>
    class DataExchange {
    public:
        DataExchange(const DataExchange<LINK>&) = delete;
        DataExchange<LINK>& operator=(const DataExchange<LINK>&) = delete;

        DataExchange()
            : _adminLock()
            , _channel(*this)
            , _current(nullptr)
            , _exchange(false, true)
            , _buffer()
        {
            _buffer.Clear();
        }

        virtual ~DataExchange() = default;

    private:
        class Handler : public LINK {
        private:
            Handler() = delete;
            Handler(const Handler&) = delete;
            Handler& operator=(const Handler&) = delete;

        public:
            template <typename... Args>
            Handler(DataExchange<LINK>& parent, Args... args)
                : LINK(args...)
                , _parent(parent)
            {
            }
            ~Handler()
            {
            }

        protected:
            virtual void StateChange()
            {
                _parent.StateChange();
            }
            virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
            {
                return (_parent.SendData(dataFrame, maxSendSize));
            }
            virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
            {
                return (_parent.ReceiveData(dataFrame, receivedSize));
            }

        private:
            DataExchange<LINK>& _parent;
        };

    public:
        inline LINK& Link()
        {
            return (_channel);
        }
        inline string RemoteId() const
        {
            return (_channel.RemoteId());
        }
        inline bool IsOpen() const
        {
            return (_channel.IsOpen());
        }
        inline uint32_t Open(uint32_t waitTime)
        {
            return (_channel.Open(waitTime));
        }
        inline uint32_t Close(uint32_t waitTime)
        {
            return (_channel.Close(waitTime));
        }
        inline uint32_t Flush()
        {
            _adminLock.Lock();

            _channel.Flush();
            _buffer.Clear();
            _current = nullptr;

            _adminLock.Unlock();

            _exchange.SetEvent();

            return (Core::ERROR_NONE);
        }
        inline uint32_t Post(Protocol::Message& message, const uint32_t allowedTime)
        {
            message.Finalize();
            return (message.Operation() == Protocol::OperationType::EVENT) ? Submit(message) : Exchange(message, allowedTime);
        }

        virtual void StateChange()
        {
        }
        virtual void Send(const Protocol::Message& message VARIABLE_IS_NOT_USED)
        {
            string data;
            Core::ToHexString(message.Data(), message.Size(), data);

            TRACE(Doofah::DataExchangeFlow, ("Message Operation: 0x%02X", message.Operation()));
            TRACE(Doofah::DataExchangeFlow, ("Message Sequence: 0x%02X", message.Sequence()));
            TRACE(Doofah::DataExchangeFlow, ("Message Address: 0x%02X", message.Address()));
            TRACE(Doofah::DataExchangeFlow, ("Message PayloadLength: 0x%02X", message.PayloadLength()));
            TRACE(Doofah::DataExchangeFlow, ("Message Complete: %s", _current->IsComplete() ? "Yes" : "No"));
            
            TRACE(Doofah::DataExchangeFlow, ("Send message: %s", data.c_str()));
        }
        virtual void Received(const Protocol::Message& message VARIABLE_IS_NOT_USED)
        {
            string data;
            Core::ToHexString(message.Data(), message.Size(), data);
            TRACE(Doofah::DataExchangeFlow, ("Received message: %s", data.c_str()));

            TRACE(Doofah::DataExchangeFlow, ("Message Operation: 0x%02X", message.Operation()));
            TRACE(Doofah::DataExchangeFlow, ("Message Sequence: 0x%02X", message.Sequence()));
            TRACE(Doofah::DataExchangeFlow, ("Message Address: 0x%02X", message.Address()));
            TRACE(Doofah::DataExchangeFlow, ("Message PayloadLength: 0x%02X", message.PayloadLength()));
            TRACE(Doofah::DataExchangeFlow, ("Message Valid: %s", message.IsValid() ? "Yes" : "No"));
        }

    private:
        uint32_t Submit(Protocol::Message& request)
        {
            uint32_t result(Core::ERROR_INPROGRESS);

            _adminLock.Lock();

            if (_current == nullptr) {
                _current = &request;
                _adminLock.Unlock();

                _channel.Trigger();
                result = Core::ERROR_NONE;

            } else {
                _adminLock.Unlock();
            }

            return (result);
        }
        uint32_t Exchange(Protocol::Message& request, const uint32_t allowedTime)
        {
            uint32_t result(Core::ERROR_INPROGRESS);

            _adminLock.Lock();

            if (_current == nullptr) {
                _current = &request;
                _adminLock.Unlock();

                _exchange.ResetEvent();

                _channel.Trigger();

                // Lock event until Completed() sets it.
                result = _exchange.Lock(allowedTime);

                if ((result == Core::ERROR_NONE) && (request.IsValid() == false)) {
                    result = Core::ERROR_INCORRECT_HASH;
                }

                _adminLock.Lock();
                _current = nullptr;
            }

            _adminLock.Unlock();

            return (result);
        }

        uint32_t Completed(const Protocol::Message& message)
        {
            uint32_t result = Core::ERROR_NONE;

            if (_current != nullptr) {
                _current->Deserialize(message.Size(), message.Data());
            }

            _exchange.SetEvent();

            return (result);
        }

        uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
        {
            uint16_t result = 0;

            _adminLock.Lock();

            if (_current != nullptr) {
                result = _current->Serialize(maxSendSize, dataFrame);

                TRACE(Doofah::DataExchangeFlow, ("Send %d bytes to %p", result, dataFrame));

                if (result == 0) {
                    Send(*_current);

                    if (_current->Operation() == Protocol::OperationType::EVENT) {
                        _current = nullptr;
                    }
                }
            }

            _adminLock.Unlock();

            return (result);
        }
        uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t availableData)
        {
            uint16_t consumedData(0);

            _adminLock.Lock();

            while (consumedData < availableData) {
                consumedData += _buffer.Deserialize( availableData - consumedData, &dataFrame[consumedData]);

                if (_buffer.IsComplete() == true) {
                    if ((_current != nullptr) && (_current->Operation() == _buffer.Operation()) && (_current->Sequence() == _buffer.Sequence())) {
                        Completed(_buffer); // this is an message we expected for
                    } else {
                        Received(_buffer);
                    }

                    _buffer.Clear();
                }
            }
            _adminLock.Unlock();

            return (availableData);
        }

    private:
        Core::CriticalSection _adminLock;
        Handler _channel;
        Protocol::Message* _current;
        Core::Event _exchange;
        Protocol::Message _buffer;
    };
} // namespace Plugin
} // namespace WPEFramework
