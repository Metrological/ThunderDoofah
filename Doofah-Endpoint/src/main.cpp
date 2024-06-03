#include <Arduino.h>

#include <Controller.h>
#include <Log.h>

#include <BleKeyboardDevice.h>
#include <IRKeyboardDevice.h>

#include <NeoPixelBus.h>
#include <OneButton.h>
#include <SimpleSerial.h>

#include <string>

using namespace Thunder::SimpleSerial;
using namespace Doofhah;

static Controller::Peripheral<BleKeyboardDevice> ble("Doofah", "Metrological");
static Controller::Peripheral<IRKeyboardDevice> ir(IR_TX_PIN);

Protocol::Message buffer;

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

void FactoryReset()
{
    Serial.flush();
    Serial.end();

    Storage::Instance().Reset();

    ESP.restart();
}

void PrintMessage(const std::string& prefix, const Protocol::Message& message)
{
    std::string data;
    ToHexString(message.Data(), message.Size(), data);

    GLOBAL_TRACE("%s: message[%s]", prefix.c_str(), data.c_str());
}

void SendMessage(const Protocol::Message& message)
{
    RgbColor color = led.GetPixelColor(0);
    Led(green);

    uint8_t data(0);

    GLOBAL_TRACE("Sending %d bytes with operation=0x%02X result=0x%02X...", message.Size(), message.Operation(), message.Result());

    PrintMessage(__FUNCTION__, message);

    while (message.Serialize(sizeof(data), &data) != 0) {
        Serial.write(data);
    }
    Led(color);
}

void Process(Protocol::Message& message)
{
    bool reboot = false;

    GLOBAL_TRACE("Processing %d bytes with operation=0x%02X device=0x%02X...", message.Size(), message.Operation(), message.Address());

    PrintMessage(__FUNCTION__, message);

    if (message.IsValid() == false) {
        message.PayloadLength(0);
        message.Result(Protocol::ResultType::CRC_INVALID);
    } else {
        Protocol::ResultType result = Protocol::ResultType::OPERATION_INVALID;

        switch (message.Operation()) {
        case Protocol::OperationType::KEY:

            GLOBAL_TRACE("KeyEvent of 0x%02X", message.Address());
            if ((message.Address() > 0x00) && (message.PayloadLength() == sizeof(Payload::KeyEvent))) {
                result = Controller::Instance().KeyEvent(message.Address() - 1, *(reinterpret_cast<const Payload::KeyEvent*>(message.Payload())));
            }
            message.PayloadLength(0);
            break;

        case Protocol::OperationType::RESET:
            GLOBAL_TRACE("Reset settings of 0x%02X", message.Address());
            if (message.Address() == 0x00) {
                reboot = true;
                result = Protocol::ResultType::OK;
            } else {
                result = Controller::Instance().Reset(message.Address() - 1);
            }

            message.PayloadLength(0);
            break;

        case Protocol::OperationType::SETTINGS:
            GLOBAL_TRACE("Set settings of 0x%02X", message.Address());
            if (message.Address() > 0x00) {
                result = Controller::Instance().Setup(message.Address() - 1, message.PayloadLength(), message.Payload());
            }
            message.PayloadLength(0);
            break;

        case Protocol::OperationType::STATE: {
            uint8_t offset(0);
            GLOBAL_TRACE("Report devices");

            const Controller::DeviceList& devices = Controller::Instance().Devices();

            std::vector<Payload::Device> payload;

            payload.push_back({ 0x00, Payload::Peripheral::ROOT });

            for (const auto& dev : devices) {
                Payload::Device device;
                device.address = (offset / sizeof(Payload::Device)) + 1;
                device.peripheral = dev->Type();
                payload.push_back(device);

                GLOBAL_TRACE("Adding device address=0x%02X type=0x%02X", device.address, device.peripheral);

                offset += sizeof(Payload::Device);
            }

            message.Payload((payload.size() * sizeof(Payload::Device)), reinterpret_cast<uint8_t*>(payload.data()));

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

    Storage::Instance().Begin();

    buffer.Clear();

    button.setPressTicks(longPressTimeMs);
    button.attachClick(SingleClick);
    button.attachLongPressStart(PressStart);
    button.attachLongPressStop(PressStop);
    button.attachDuringLongPress(PressUpdate);

    Controller::Instance().StartDevices();

    Serial.begin(COM_BAUDRATE);

    GLOBAL_TRACE("Starting endpoint build %s", __TIMESTAMP__);

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
            GLOBAL_TRACE("Received a complete message!");
            Process(buffer);
            Led(off);
        }
    }
}
