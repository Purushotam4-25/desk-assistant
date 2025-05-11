#include "rot_encoder.h"

#define ENCODER_CLK 32
#define ENCODER_DT 33
#define ENCODER_SW 15

OneButton button(ENCODER_SW, true);

int lastCLK = HIGH;
int position = 0;

void handleClick() {
  Serial.println("Clicked");
}

void handleDoubleClick() {
  Serial.println("Double Clicked");
}

void handleLongPress() {
  Serial.println("Long Press");
}

void setupEncoder() {
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);

  button.attachClick(handleClick);
  button.attachDoubleClick(handleDoubleClick);
  button.attachLongPressStart(handleLongPress);
}

void updateEncoder() {
  button.tick();

  int currentCLK = digitalRead(ENCODER_CLK);
  if (currentCLK != lastCLK) {
    if (digitalRead(ENCODER_DT) != currentCLK) {
      position++;
      Serial.print("CW: "); Serial.println(position);
    } else {
      position--;
      Serial.print("CCW: "); Serial.println(position);
    }
  }
  lastCLK = currentCLK;
}
