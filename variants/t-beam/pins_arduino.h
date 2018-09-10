#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

#define EXTERNAL_NUM_INTERRUPTS 16
#define NUM_DIGITAL_PINS        20
#define NUM_ANALOG_INPUTS       16

#define analogInputToDigitalPin(p)  (((p)<20)?(esp32_adc2gpio[(p)]):-1)
#define digitalPinToInterrupt(p)    (((p)<40)?(p):-1)
#define digitalPinHasPWM(p)         (p < 34)

// SPI LoRa Radio
#define LORA_SCK 5      // GPIO5  - SX1276 SCK
#define LORA_MISO 19    // GPIO19 - SX1276 MISO
#define LORA_MOSI 27    // GPIO27 - SX1276 MOSI
#define LORA_CS 18      // GPIO18 - SX1276 CS
#define LORA_RST 23     // GPIO23 - SX1276 RST
#define LORA_IO0 26     // GPIO26 - SX1276 IO0
#define LORA_IO1 33     // GPIO33 - SX1276 IO1
#define LORA_IO2 32     // GPIO32 - SX1276 IO2

static const uint8_t TX = 1;
static const uint8_t RX = 3;

static const uint8_t SDA = 21;
static const uint8_t SCL = 22;
 
static const uint8_t SS    = 18;
static const uint8_t MOSI  = 27;
static const uint8_t MISO  = 19;
static const uint8_t SCK   = 5;

static const uint8_t A6 = 34;
static const uint8_t A7 = 35;

static const uint8_t A11 = 0;
static const uint8_t A12 = 2;
static const uint8_t A17 = 27;
static const uint8_t A18 = 25;
static const uint8_t A19 = 26;

static const uint8_t T0 = 4;
static const uint8_t T1 = 0;
static const uint8_t T2 = 2;
static const uint8_t T3 = 15;
static const uint8_t T4 = 13;
static const uint8_t T5 = 12;
static const uint8_t T6 = 14;
static const uint8_t T8 = 32;
static const uint8_t T9 = 33;

static const uint8_t DAC2 = 25;

#endif /* Pins_Arduino_h */
