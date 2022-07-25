#include <Arduino.h>
#include <BleKeyboard.h>
#include <SimpleSerial.h>
#include <EEPROM.h>

#define EEPROM_SIZE 2

using namespace WPEFramework::SimpleSerial;

static const char *ID()
{
  static char id[23] = {};

  if (strlen(id) == 0)
  {
    // The chip ID is essentially its MAC address(length: 6 bytes).
    uint64_t chipid = ESP.getEfuseMac();
    snprintf(id, 23, "Doofah-%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
  }

  return id;
}

BleKeyboard bleKeyboard(ID(), "Metrological");
Protocol::Message buffer;
std::vector<Payload::Device> devices;


constexpr uint8_t InvalidEEPROMValue = 0xFF;

//EEPROM addresses
constexpr uint8_t MessageBaseAddress = 0x00;
constexpr uint8_t MessageOperationAddress = MessageBaseAddress;
constexpr uint8_t MessageSequenceAddress = MessageBaseAddress + 1;

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

Protocol::ResultType KeyMessage(const Payload::KeyEvent *event)
{
  uint8_t keycode(event->code & 0xff);

  log("Sending key event: 0x%02X (0x%04X)", keycode, event->code);

  if (event->pressed == Payload::Action::PRESSED)
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

  while (message.Serialize(sizeof(data), &data) == 0)
  {
    Serial.write(data);
  }
}

void Persist(const uint8_t operation, const uint8_t sequence)
{
  EEPROM.write(MessageOperationAddress, operation);
  EEPROM.write(MessageSequenceAddress, sequence);
  EEPROM.commit();
}

void HandleReboot()
{
  uint8_t operation = EEPROM.read(MessageOperationAddress);

  if (operation != InvalidEEPROMValue)
  {
    log("Handle reset: 0x%02X was found on MessageOperationAddress", operation);

    buffer.Operation(static_cast<Protocol::OperationType>(operation));
    buffer.Sequence(EEPROM.read(MessageSequenceAddress));
    buffer.Result(Protocol::ResultType::OK);
    buffer.PayloadLength(0);

    buffer.Finalize();

    SendMessage(buffer);

    buffer.Clear();
    
    // reset EEPROM
    Persist(InvalidEEPROMValue, InvalidEEPROMValue);
  }
}

void Process(Protocol::Message &message)
{
  Protocol::ResultType result(Protocol::ResultType::OK);

  log("Processing %d 0x%02X...", message.Size(), message.Operation());

  if (message.IsValid() == true)
  {

    Protocol::ResultType result = Protocol::ResultType::OPERATION_INVALID;
    message.Offset(0);

    switch (message.Operation())
    {
    case Protocol::OperationType::ALLOCATE:
      result = Protocol::ResultType::OK;
      message.PayloadLength(0);
      break;

    case Protocol::OperationType::EVENT:
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
        result = KeyMessage(reinterpret_cast<const Payload::KeyEvent *>(message.Payload()));
      }

      message.PayloadLength(0);
      break;

    case Protocol::OperationType::RESET:
      if (message.Address() == static_cast<Protocol::DeviceAddressType>(Payload::Peripheral::ROOT))
      {
        Persist(static_cast<uint8_t>(message.Operation()), message.Sequence());
        ESP.restart();
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

    default:
      message.PayloadLength(0);
      result = Protocol::ResultType::OPERATION_INVALID;
      break;
    }

    message.Result(result);
  }
  else
  {
    message.PayloadLength(0);
    message.Result(Protocol::ResultType::CRC_INVALID);
  }

  message.Finalize();
  message.Offset(0);

  SendMessage(message);

  message.Clear();
}

void setup()
{
  EEPROM.begin(EEPROM_SIZE);
  Serial2.begin(LOGBAUDRATE, SERIAL_8N1, LOGRXPIN, LOGTXPIN);

  log("Starting %s endpoint build %s", ID(), __TIMESTAMP__);
  
  buffer.Clear();

  devices.push_back({0x00, Payload::PeripheralState::AVAILABLE, Payload::Peripheral::ROOT});
  devices.push_back({0x01, Payload::PeripheralState::AVAILABLE, Payload::Peripheral::BLE});

  bleKeyboard.begin();
  Serial.begin(115200);

  HandleReboot();
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
