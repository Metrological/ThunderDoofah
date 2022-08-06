#include <Arduino.h>
#include "Log.h"

#include <BleKeyboard.h>
#include <EEPROM.h>
#include <NeoPixelBus.h>
#include <NimBLEDevice.h>
#include <OneButton.h>
#include <SimpleSerial.h>

using namespace WPEFramework::SimpleSerial;

BleKeyboard bleKeyboard("Doofah", "Metrological");
Protocol::Message buffer;
std::vector<Payload::Device> devices;

OneButton button = OneButton(
    BUTTON_PIN, // Input pin for the button
    true, // Button is active LOW
    true // Enable internal pull-up resistor
);

// save the millis when a press has started.
unsigned long pressStartTime;

// The time when LongPressStart is called
constexpr uint16_t longPressTimeMs = 1000;

NeoPixelBus<NeoGrbFeature, NeoSk6812Method> led(RGB_LED_COUNT, RGB_LED_PIN);

constexpr uint8_t colorSaturation = 16;

RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor off(0);

unsigned long lastBlink = 0;

void Led(const RgbColor& color)
{
    led.SetPixelColor(0, color);
    led.Show();
}

constexpr uint8_t IRDevices[] = {
    IR_TX_PIN
};

// EEPROM addresses
constexpr uint8_t MessageBaseAddress = 0x00;

#pragma pack(push, 1)
struct Config {
    // The default values if there is no EEPROM setting yet..
    Config()
        : ble()
        , ir()
        , battery()
    {
        memset(&ble, 0, sizeof(ble));
        memset(&ir, 0, sizeof(ir));
        memset(&battery, 0, sizeof(battery));
    }
    Payload::BLESettings ble;
    Payload::IRSettings ir;
    Payload::BatteryLevel battery;
} _config;
#pragma pack(pop)

void Save()
{
    EEPROM.writeBytes(MessageBaseAddress, &_config, sizeof(_config));
    EEPROM.commit();
}
void Load()
{
    if (EEPROM.readByte(MessageBaseAddress) != 0xFF) {
        EEPROM.readBytes(MessageBaseAddress, &_config, sizeof(_config));
    }
}

bool SetBLE(const Payload::BLESettings& newSettings)
{
    bool changed;

    if ((memcmp(&newSettings, &_config.ble, std::min(sizeof(newSettings), sizeof(_config.ble))) != 0)) {
        memcpy(&_config.ble, &newSettings, std::min(sizeof(newSettings), sizeof(_config.ble)));
        changed = true;
    }

    return changed;
}
bool SetIR(const Payload::IRSettings& newSettings)
{
    bool changed;

    if ((memcmp(&newSettings, &_config.ir, std::min(sizeof(newSettings), sizeof(_config.ir))) != 0)) {
        memcpy(&_config.ir, &newSettings, std::min(sizeof(newSettings), sizeof(_config.ir)));
        changed = true;
    }

    return changed;
}
bool SetBattery(const Payload::BatteryLevel& newSettings)
{
    bool changed;

    if ((memcmp(&newSettings, &_config.battery, std::min(sizeof(newSettings), sizeof(_config.battery))) != 0)) {
        memcpy(&_config.battery, &newSettings, std::min(sizeof(newSettings), sizeof(_config.battery)));

        (newSettings.percentage == 0) ? bleKeyboard.setBatteryLevel(100) : bleKeyboard.setBatteryLevel(newSettings.percentage);
        changed = true;
    }

    return changed;
}

void FactoryReset()
{
    Serial.flush();
    Serial.end();

    Config cleared;

    if (SetBLE(cleared.ble) || SetIR(cleared.ir) || SetBattery(cleared.battery)) {
        Save();
    }

    ESP.restart();
}

Protocol::ResultType KeyMessage(const Payload::KeyEvent& event)
{
    uint8_t keycode(event.code & 0xff);

    Log::Instance().Print("Sending key event: 0x%02X (0x%04X)", keycode, event.code);

    if (event.pressed == Payload::Action::PRESSED) {
        bleKeyboard.press(keycode);
    } else {
        bleKeyboard.release(keycode);
    }

    return Protocol::ResultType::OK;
}

void SendMessage(const Protocol::Message& message)
{
    RgbColor color = led.GetPixelColor(0);
    Led(green);

    uint8_t data(0);

    Log::Instance().Print("Sending %d bytes with operation=0x%02X result=0x%02X...", message.Size(), message.Operation(), message.Result());

    while (message.Serialize(sizeof(data), &data) != 0) {
        Serial.write(data);
    }
    Led(color);
}

void Process(Protocol::Message& message)
{
    bool reboot = false;

    Log::Instance().Print("Processing %d bytes with operation=0x%02X device=0x%02X...", message.Size(), message.Operation(), message.Address());

    if (message.IsValid() == false) {
        message.PayloadLength(0);
        message.Result(Protocol::ResultType::CRC_INVALID);
    } else {
        Protocol::ResultType result = Protocol::ResultType::OPERATION_INVALID;

        switch (message.Operation()) {
        case Protocol::OperationType::KEY:
            if (message.PayloadLength() == sizeof(Payload::KeyEvent)) {
                result = KeyMessage(*(reinterpret_cast<const Payload::KeyEvent*>(message.Payload())));
            }
            message.PayloadLength(0);
            break;

        case Protocol::OperationType::RESET:
            if (message.Address() == 0x00) {
                reboot = true;
                result = Protocol::ResultType::OK;
            } else if (message.Address() == 0x01) {
                Payload::BLESettings clean;
                memset(&clean, 0, sizeof(clean));

                if (SetBLE(clean) == true) {
                    Save();
                    reboot = true;
                }
                result = Protocol::ResultType::OK;
            }
#ifdef IR_ENABLED
            else if (message.Address() == 0x02) {
                Payload::IRSettings clean;
                memset(&clean, 0, sizeof(clean));

                if (SetIR(clean) == true) {
                    Save();
                    reboot = true;
                }
                result = Protocol::ResultType::OK;
            }
#endif
            else {
                result = Protocol::ResultType::NOT_AVAILABLE;
            }
            message.PayloadLength(0);
            break;

        case Protocol::OperationType::SETTINGS:
            if (message.Address() == 0x01) {
                if (message.PayloadLength() == sizeof(Payload::BLESettings)) {
                    Log::Instance().Print("BLE Settings");

                    Payload::BLESettings newSettings;

                    memcpy(&newSettings, message.Payload(), message.PayloadLength());

                    if (SetBLE(newSettings) == true) {
                        Save();
                    }

                    result = Protocol::ResultType::OK;
                } else {
                    result = Protocol::ResultType::PAYLOAD_INVALID;
                }
            }
#ifdef IR_ENABLED
            else if (message.Address() == 0x02) {

                if (message.PayloadLength() == sizeof(Payload::IRSettings)) {
                    Payload::IRSettings newSettings;

                    memcpy(&newSettings, message.Payload(), message.PayloadLength());

                    if (SetIR(newSettings) == true) {
                        Save();
                    }

                    result = Protocol::ResultType::OK;
                } else {
                    result = Protocol::ResultType::PAYLOAD_INVALID;
                }
            }
#endif
            else {
                result = Protocol::ResultType::NOT_AVAILABLE;
            }

            message.PayloadLength(0);

            break;

        case Protocol::OperationType::STATE: {
            uint8_t offset(0);
            uint8_t payload[devices.size() * sizeof(Payload::Device)];

            for (const auto& dev : devices) {
                memcpy(&payload[offset], reinterpret_cast<const uint8_t*>(&dev), sizeof(Payload::Device));
                offset += sizeof(Payload::Device);
            }

            message.Payload(sizeof(payload), payload);
            result = Protocol::ResultType::OK;
            break;
        }

            // case Protocol::OperationType::EVENT:
            //     ASSERT(false); // We should be generating this...
            //     break;

        default:
            message.PayloadLength(0);
            break;
        }

        message.Result(result);
    }

    message.Finalize();

    SendMessage(message);

    if (reboot == true) {
        ESP.restart();
    }

    message.Clear();
}

void SendEvent()
{
    Protocol::Message message;
    message.Clear();
    message.Operation(Protocol::OperationType::EVENT);
    message.PayloadLength(0);

    message.Finalize();

    SendMessage(message);
}

// button callbacks
void SingleClick()
{
    // poke the plugin
    SendEvent();
}
void PressStart()
{
    pressStartTime = millis() - longPressTimeMs;
}
void PressStop()
{
    Led(off);
    lastBlink = 0;
}
void PressUpdate()
{
    unsigned long time = millis() - pressStartTime;

    if (time > (lastBlink + 500)) {
        RgbColor color = led.GetPixelColor(0);
        (color == off) ? Led(red) : Led(off);
        lastBlink = time;
    }

    if (time > 10000) {
        FactoryReset();
    }
}

void setup()
{
    led.Begin();
    Led(red);

    EEPROM.begin(sizeof(_config));
    Load();

    buffer.Clear();

    devices.push_back({ 0x00, Payload::PeripheralState::AVAILABLE, Payload::Peripheral::ROOT });
    devices.push_back({ 0x01, Payload::PeripheralState::AVAILABLE, Payload::Peripheral::BLE });

#ifdef IR_ENABLED
    uint8_t deviceAdressIndex(0);
    while (deviceAdressIndex < sizeof(IRDevices)) {
        devices.push_back({ IRDevices[deviceAdressIndex++], Payload::PeripheralState::AVAILABLE, Payload::Peripheral::IR });
    }
#endif

    button.setPressTicks(longPressTimeMs);
    button.attachClick(SingleClick);
    button.attachLongPressStart(PressStart);
    button.attachLongPressStop(PressStop);
    button.attachDuringLongPress(PressUpdate);

    if (strlen(_config.ble.name) > 0) {
        bleKeyboard.setName(_config.ble.name);
        Log::Instance().Print("BLE name %s loaded from flash", _config.ble.name);
    }

    if (_config.ble.vid > 0) {
        bleKeyboard.set_vendor_id(_config.ble.vid);
        Log::Instance().Print("BLE vendor ID 0x%04X loaded from flash", _config.ble.vid);
    }

    if (_config.ble.pid > 0) {
        bleKeyboard.set_product_id(_config.ble.pid);
        Log::Instance().Print("BLE product ID 0x%04X loaded from flash", _config.ble.pid);
    }

    if (_config.battery.percentage > 0) {
        bleKeyboard.setBatteryLevel(_config.battery.percentage);
        Log::Instance().Print("BLE batery level %d% loaded from flash", _config.battery.percentage);
    }

    bleKeyboard.begin();

    Serial.begin(COM_BAUDRATE);

    NimBLEAddress address = NimBLEDevice::getAddress();
    Log::Instance().Print("Starting BLE work on [%s] endpoint build %s", address.toString().c_str(), __TIMESTAMP__);
    SendEvent();
    Led(off);
}

void loop()
{
    button.tick();
    while (Serial.available() > 0) {
        Led(blue);

        uint8_t byte = Serial.read();
        buffer.Deserialize(sizeof(byte), &byte);

        if (buffer.IsComplete()) {
            Log::Instance().Print("Received a complete message!");
            Process(buffer);
            Led(off);
        }
    }
}
