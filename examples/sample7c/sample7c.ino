#include "OXOcardRunner.h"

void setup() {

}

byte a;

void loop() {
  drawNumber(a);
  delay(500);

  if (a < 7) {
    a = a + 1;
  }
  else {
    a = 0;
  }
}

