#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

#define USB_VID            0x239A
#define USB_PID            0x8145
#define USB_MANUFACTURER   "Adafruit"
#define USB_PRODUCT        "Metro ESP32-S3"
#define USB_SERIAL         ""   // Empty string for MAC adddress

#define EXTERNAL_NUM_INTERRUPTS 46
#define NUM_DIGITAL_PINS        48
#define NUM_ANALOG_INPUTS       20

#define analogInputToDigitalPin(p)  (((p)<20)?(analogChannelToDigitalPin(p)):-1)
#define digitalPinToInterrupt(p)    (((p)<48)?(p):-1)
#define digitalPinHasPWM(p)         (p < 46)

#define LED_BUILTIN     13

#define PIN_NEOPIXEL    45
#define NEOPIXEL_NUM    1

#define PIN_BUTTON1     0  // BOOT0 switch

static const uint8_t TX = 40;
static const uint8_t RX = 41;
#define TX1 TX
#define RX1 RX

static const uint8_t SDA = 47;
static const uint8_t SCL = 48;

static const uint8_t SS   = 42;
static const uint8_t MOSI = 35;
static const uint8_t SCK  = 36;
static const uint8_t MISO = 37;

static const uint8_t A0 = 14;
static const uint8_t A1 = 15;
static const uint8_t A2 = 16;
static const uint8_t A3 = 17;
static const uint8_t A4 = 18;
static const uint8_t A5 = 1;

static const uint8_t A6 = 40;
static const uint8_t A7 = 41;
static const uint8_t A8 = 2;
static const uint8_t A9 = 3;
static const uint8_t A10 = 4;
static const uint8_t A11 = 5;
static const uint8_t A12 = 6;
static const uint8_t A13 = 7;
static const uint8_t A14 = 8;
static const uint8_t A15 = 9;
static const uint8_t A16 = 10;
static const uint8_t A17 = 11;
static const uint8_t A18 = 12;
static const uint8_t A19 = 13;

static const uint8_t T1 = 1;
static const uint8_t T2 = 2;
static const uint8_t T3 = 3;
static const uint8_t T4 = 4;
static const uint8_t T5 = 5;
static const uint8_t T6 = 6;
static const uint8_t T7 = 7;
static const uint8_t T8 = 8;
static const uint8_t T9 = 9;
static const uint8_t T10 = 10;
static const uint8_t T11 = 11;
static const uint8_t T12 = 12;
static const uint8_t T13 = 13;
static const uint8_t T14 = 14;

#endif /* Pins_Arduino_h */
