#include <Arduino.h>
#include <BleKeyboard.h>
#include <NimBLEDevice.h>
#include <SimpleSerial.h>
#include <EEPROM.h>

using namespace WPEFramework::SimpleSerial;

BleKeyboard bleKeyboard("Doofah", "Metrological");
Protocol::Message buffer;
std::vector<Payload::Device> devices;

// EEPROM addresses
constexpr uint8_t MessageBaseAddress = 0x00;

struct Config {
  // The default values if there is no EEPROM setting yet..
  Config() 
    : ProductId(0)
    , VendorId(0)
    , Address(0) {
  }

  // Values that can be set to change the bahaviour of the Doofah..
  uint16_t ProductId;
  uint16_t VendorId;
  uint8_t Address;

} _config;

#ifdef __DEBUG__
void log(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  std::vector<char> buffer(strlen(fmt));

  int sz = vsnprintf(&buffer[0], buffer.size(), fmt, args);

  if (sz >= int(buffer.size()))
  {
    buffer.resize(sz + 1);
    vsnprintf(&buffer[0], buffer.size(), fmt, args);
  }
  va_end(args);

  Serial2.println(&buffer[0]);
}
#else 
void log (const char *, ...)
{
}
#endif

// Store information in the EEPROM that should be persited over boots
// ------------------------------------------------------------------------
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

Protocol::ResultType KeyMessage(const Payload::KeyEvent& event)
{
  uint8_t keycode(event.code & 0xff);

  log("Sending key event: 0x%02X (0x%04X)", keycode, event.code);

  if (event.pressed == Payload::Action::PRESSED)
  {
    bleKeyboard.press(keycode);
  }
  else
  {
    bleKeyboard.release(keycode);
  }

  return Protocol::ResultType::OK;
}

void SendMessage(const Protocol::Message &message)
{
  uint8_t data(0);

  log("Sending %d bytes with operation=0x%02X result=0x%02X...", message.Size(), message.Operation(), message.Result());

  while (message.Serialize(sizeof(data), &data) != 0)
  {
    Serial.write(data);
  }
}


void Process(Protocol::Message &message)
{
  bool reboot = false;

  log("Processing %d 0x%02X...", message.Size(), message.Operation());

  if (message.IsValid() == false) {
    message.PayloadLength(0);
    message.Result(Protocol::ResultType::CRC_INVALID);
  }
  else {
    Protocol::ResultType result = Protocol::ResultType::OPERATION_INVALID;

    switch (message.Operation())
    {
    case Protocol::OperationType::ALLOCATE:
      result = Protocol::ResultType::OK;
      message.PayloadLength(0);
      break;

    case Protocol::OperationType::FREE:
      result = Protocol::ResultType::OK;
      message.PayloadLength(0);
      break;

    case Protocol::OperationType::KEY:
      if (message.PayloadLength() == sizeof(Payload::KeyEvent))
      {
        result = KeyMessage(*(reinterpret_cast<const Payload::KeyEvent *>(message.Payload())));
      }
      message.PayloadLength(0);
      break;

    case Protocol::OperationType::RESET:
      if (message.Address() == static_cast<Protocol::DeviceAddressType>(Payload::Peripheral::ROOT))
      {
        reboot = true;
        result = Protocol::ResultType::OK;
        message.PayloadLength(0);
      }
      break;

    case Protocol::OperationType::SETTINGS:
      result = Protocol::ResultType::OK;
      message.PayloadLength(0);
      break;

    case Protocol::OperationType::STATE:
      message.Offset(0);
      message.PayloadLength(devices.size() * sizeof(Payload::Device));

      for (const auto &dev : devices)
      {
        message.Deserialize(sizeof(dev), reinterpret_cast<const uint8_t *>(&dev));
      }

      result = Protocol::ResultType::OK;
      break;

    case Protocol::OperationType::EVENT:
      ASSERT(false); // We should be generating this...
      break;

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

void setup()
{
  #ifdef __DEBUG__
  Serial2.begin(LOGBAUDRATE, SERIAL_8N1, LOGRXPIN, LOGTXPIN);
  #endif

  buffer.Clear();

  devices.push_back({0x00, Payload::PeripheralState::AVAILABLE, Payload::Peripheral::ROOT});
  devices.push_back({0x01, Payload::PeripheralState::AVAILABLE, Payload::Peripheral::BLE});

  EEPROM.begin(sizeof(_config));
  bleKeyboard.begin();
  
  Load();

  NimBLEAddress address = NimBLEDevice::getAddress();
  log("Starting BLE work on [%s] endpoint build %s", address.toString().c_str(), __TIMESTAMP__);
}

void loop()
{
  while (Serial.available() > 0)
  {
    uint8_t byte = Serial.read();
    buffer.Deserialize(sizeof(byte), &byte);

    if (buffer.IsComplete())
    {
      log("Received a complete message!");
      Process(buffer);
    }
  }
}
