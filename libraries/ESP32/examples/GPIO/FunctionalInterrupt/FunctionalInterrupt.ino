#include <Arduino.h>

#define BUTTON1 16
#define BUTTON2 17

struct Button {
  uint8_t PIN;
  volatile uint32_t numberKeyPresses;
  volatile int pressed;
  void (*isr)(struct Button*);
};

void isr(struct Button* button) {
  button->numberKeyPresses += 1;
  button->pressed = 1;
}

void checkPressed(struct Button* button) {
  if(button->pressed) {
    Serial.printf("Button on pin %u has been pressed %u times\n", button->PIN, button->numberKeyPresses);
    button->pressed = 0;
  }
}

struct Button button1 = {BUTTON1, 0, 0, isr};
struct Button button2 = {BUTTON2, 0, 0, isr};

void setup() {
  Serial.begin(115200);
  pinMode(button1.PIN, INPUT_PULLUP);
  pinMode(button2.PIN, INPUT_PULLUP);
  attachInterrupt(button1.PIN, &isr, FALLING);
  attachInterrupt(button2.PIN, &isr, FALLING);
}

void loop() {
  checkPressed(&button1);
  checkPressed(&button2);
}
