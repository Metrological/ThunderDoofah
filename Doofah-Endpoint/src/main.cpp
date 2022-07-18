#include <Arduino.h>
#include <BleKeyboard.h>

BleKeyboard bleKeyboard("BLE Test Keyboard", "Metrological");

enum Action : uint8_t {
  KEY_SEND = 0x01,
  KEY_PRESS,
  KEY_RELEASE,
  KEY_RELEASE_ALL,
  BATTERY_LEVEL,
  SET_VENDOR_ID,
  SET_PRODUCT_ID,
  SET_NAME,
};

struct Frame
{
  uint8_t length;
  Action action;
  uint8_t data[];
};

void setup() {
  Serial.begin(115200);
  bleKeyboard.begin();
}

void loop() {
  if(bleKeyboard.isConnected()) {
    Serial.println("Sending 'Hello world'...");
    delay(10000);
    bleKeyboard.print("Hello world");

    delay(1000);

    Serial.println("Sending Enter key...");
    bleKeyboard.write(KEY_RETURN);

    delay(1000);

    Serial.println("Sending Play/Pause media key...");
    bleKeyboard.write(KEY_MEDIA_PLAY_PAUSE);

    delay(1000);

   //
   // Below is an example of pressing multiple keyboard modifiers 
   // which by default is commented out.
    /*
    Serial.println("Sending Ctrl+Alt+Delete...");
    bleKeyboard.press(KEY_LEFT_CTRL);
    bleKeyboard.press(KEY_LEFT_ALT);
    bleKeyboard.press(KEY_DELETE);
    delay(100);
    bleKeyboard.releaseAll();
    */
  }
}