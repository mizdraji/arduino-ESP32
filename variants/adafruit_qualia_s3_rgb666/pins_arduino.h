#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

#define USB_VID            0x239A
#define USB_PID            0x8147

#define USB_MANUFACTURER   "Adafruit"
#define USB_PRODUCT        "Qualia ESP32-S3 RGB666"
#define USB_SERIAL         "" // Empty string for MAC adddress

#define EXTERNAL_NUM_INTERRUPTS 46
#define NUM_DIGITAL_PINS        48
#define NUM_ANALOG_INPUTS       2

#define analogInputToDigitalPin(p)  (((p)<NUM_ANALOG_INPUTS)?(analogChannelToDigitalPin(p)):-1)
#define digitalPinToInterrupt(p)    (((p)<EXTERNAL_NUM_INTERRUPTS)?(p):-1)
#define digitalPinHasPWM(p)         (p < 46)

static const uint8_t PCA_TFT_SCK = 0;
static const uint8_t PCA_TFT_CS = 1;
static const uint8_t PCA_TFT_RESET = 2;
static const uint8_t PCA_CPT_IRQ = 3;
static const uint8_t PCA_TFT_BACKLIGHT = 4;
static const uint8_t PCA_BUTTON_UP = 5;
static const uint8_t PCA_BUTTON_DOWN = 6;
static const uint8_t PCA_TFT_MOSI = 7;

static const uint8_t TX = 16;
static const uint8_t RX = 17;
#define TX1 TX
#define RX1 RX

static const uint8_t SDA = 8;
static const uint8_t SCL = 18;

static const uint8_t SS   = 15;
static const uint8_t MOSI = 7;
static const uint8_t MISO = 6;
static const uint8_t SCK  = 5;

static const uint8_t A0 = 17;
static const uint8_t A1 = 16;

static const uint8_t T3  = 3;  // Touch pin IDs map directly
static const uint8_t T8  = 8;  // to underlying GPIO numbers NOT
static const uint8_t T9  = 9;  // the analog numbers on board silk
static const uint8_t T10 = 10;
static const uint8_t T11 = 11;
static const uint8_t T12 = 12;


static const uint8_t 
  TFT_R1 = 11,
  TFT_R2 = 10,
  TFT_R3 = 9,
  TFT_R4 = 46,
  TFT_R5 = 3,
  TFT_G0 = 48,
  TFT_G1 = 47,
  TFT_G2 = 21,
  TFT_G3 = 14,
  TFT_G4 = 13,
  TFT_G5 = 12,
  TFT_B1 = 40,
  TFT_B2 = 39,
  TFT_B3 = 38,
  TFT_B4 = 0,
  TFT_B5 = 45,
  TFT_PCLK = 1,
  TFT_DE = 2,
  TFT_HSYNC = 41,
  TFT_VSYNC = 42;

#endif /* Pins_Arduino_h */
