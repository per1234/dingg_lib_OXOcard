/**-----------------------------------------------------------------------------
 * \file    OXOCardRunner.h
 * \author  jh, tg
 * \date    xx.02.2017
 *
 * \version 1.0
 *
 * \brief   The OXOCardRunner is a set of simplified functions for
 *          programming

 *
 * @{
 -----------------------------------------------------------------------------*/

/* Define to prevent recursive inclusion -----------------------*/
#ifndef OXOCARD_RUNNER_H_
#define OXOCARD_RUNNER_H_

/* Includes --------------------------------------------------- */
#include "OXOCard.h"

/* global variables ------------------------------------------- */
uint16_t autoTurnOffAfter = -1;
volatile uint16_t autoTurnOffCnt = 0;
volatile bool goingToTurnOff = false;

enum orientation : byte {UNKNOWN = 0, UP = 1, DOWN = 2, HORIZONTALLY = 3, VERTICALLY = 4};

/* Object instantiations -------------------------------------- */
OXOCard globalOXOCard;

/* System functions ------------------------------------------- */
void initVariant() {      // hook function from Arduino to allow 3rd party variant-specific initialization
  globalOXOCard.begin();
}

/* ------------------------------------- */
void turnOff() {
   globalOXOCard.turnOff();
}

/* ------------------------------------- */
void handleAutoTurnOff(uint16_t seconds = DEFAULT_AUTO_TURN_OFF)
{
  if (seconds < autoTurnOffAfter)
  {
    /* counter has been changed -> reset the interrupt counter */
    autoTurnOffCnt = 0;
  }
  autoTurnOffAfter = seconds;

  if (goingToTurnOff)
  {
    goingToTurnOff = false;
    globalOXOCard.turnOff();
  }
}

/* ------------------------------------- */
bool isLeftButtonPressed() {
  bool pressed = false;
  if (button1Pressed)
  {
    autoTurnOffCnt = 0; // prevent autoturnoff after an action
    pressed = true;
  }
  return pressed;
}

/* ------------------------------------- */
bool isMiddleButtonPressed() {
  bool pressed = false;
  if (button2Pressed)
  {
    autoTurnOffCnt = 0; // prevent autoturnoff after an action
    pressed = true;
  }
  return pressed;
}

/* ------------------------------------- */
bool isRightButtonPressed() {
  bool pressed = false;
  if (button3Pressed)
  {
    autoTurnOffCnt = 0; // prevent autoturnoff after an action
    pressed = true;
  }
  return pressed;
}

/* LED-Matrix functions --------------------------------------- */
void clearDisplay() {
   globalOXOCard.matrix->fillScreen(0);
}

/* ------------------------------------- */
void turnDisplayOn() {
  globalOXOCard.matrix->fillScreen(255);
}

/* ------------------------------------- */
void fillDisplay(byte brightness=255) {
  globalOXOCard.matrix->fillScreen(brightness);
}

/* ------------------------------------- */
void drawPixel(byte x, byte y, byte brightness=255) {
  globalOXOCard.matrix->drawPixel(x,y,brightness);
}

/* ------------------------------------- */
void drawRectangle(byte x, byte y, byte l, byte h, byte b=255) {
  globalOXOCard.matrix->drawRectangle(x,y,l,h,b);
};

/* ------------------------------------- */
void drawFilledRectangle(byte x, byte y, byte l, byte h, byte b=255) {
  globalOXOCard.matrix->drawFilledRectangle(x,y,l,h,b);
};

/* ------------------------------------- */
void drawLine(byte x0, byte y0, byte x1, byte y1, byte b=255)
{
  globalOXOCard.matrix->drawLine(x0,y0,x1,y1,b);
};

/* ------------------------------------- */
void drawCircle(byte x0, byte y0, byte r, byte b=255)
{
  globalOXOCard.matrix->drawCircle(x0,y0,r,b);
};

/* ------------------------------------- */
void drawFilledCircle(byte x0, byte y0, byte r, byte b=255)
{
  globalOXOCard.matrix->drawFilledCircle(x0,y0,r,b);
};

/* ------------------------------------- */
void drawTriangle(byte x0, byte y0, byte x1, byte y1, byte x2, byte y2, byte b=255) {
  globalOXOCard.matrix->drawTriangle(x0,y0,x1,y1,x2,y2,b);
}

/* ------------------------------------- */
void drawFilledTriangle(byte x0, byte y0, byte x1, byte y1, byte x2, byte y2, byte b=255) {
  globalOXOCard.matrix->drawFilledTriangle(x0,y0,x1,y1,x2,y2,b);
}

/* ------------------------------------- */
void drawImage(byte image[8], byte brightness=255) {
  for (byte y =0;y<=7;y++) {
    byte b = image[y];
    for (byte x = 0;x<=7;x++) {
      if (b & (1 << (7-x))) {

        globalOXOCard.matrix->drawPixel(x,y,brightness);
      }
    }
  }
}

/* ------------------------------------- */
void drawImage(byte b0,
               byte b1,
               byte b2,
               byte b3,
               byte b4,
               byte b5,
               byte b6,
               byte b7,
               byte brightness=255) {

  byte image[8] = {b0,b1,b2,b3,b4,b5,b6,b7};
  drawImage(image, brightness);
};

/* ------------------------------------- */
void drawChar(byte x, byte y, char c, byte brightness=255) {
  globalOXOCard.matrix->drawChar(x, y,  c, brightness);
}

/* ------------------------------------- */
void drawDigit(byte x, byte y, byte digit, byte brightness=255) {
  drawChar(x,y,48 + (digit%10), brightness);
}

/* ------------------------------------- */
void drawNumber(byte number, byte brightness=255) {
  if (number > 99) {
    drawChar(0,1,'?');
    drawChar(5,1,'?');
  }
  else
  {
    drawChar(0,1,48 + (number/10));
    drawChar(5,1,48 + (number%10));
  }
}

/* Accelerometer functions ------------------------------------ */
float getXAcceleration() {
  float vector[3];
  globalOXOCard.accel->getAccelerationVector(vector);
  return vector[0];
}

/* ------------------------------------- */
float getYAcceleration() {
  float vector[3];
  globalOXOCard.accel->getAccelerationVector(vector);
  return vector[1];
}

/* ------------------------------------- */
float getZAcceleration() {
  float vector[3];
  globalOXOCard.accel->getAccelerationVector(vector);
  return vector[2];
}

/* ------------------------------------- */
byte getOrientation() {
  return byte(globalOXOCard.accel->getOrientation());    // 1 = UP, 2 = DOWN, 3 = HORIZONTALLY, 4 = VERTICALLY
}

// String getOrientation() {
//   String orientation;
//   switch(byte(globalOXOCard.accel->getOrientation()))
//   {
//     case UP:           orientation = "UP";           break;
//     case DOWN:         orientation = "DOWN";         break;
//     case HORIZONTALLY: orientation = "HORIZONTALLY"; break;
//     case VERTICALLY:   orientation = "VERTICALLY";   break;
//     default:           orientation = "UNKNOWN";      break;
//   };
//   return orientation;
// }

/* ------------------------------------- */
bool isOrientationUp() {
  return byte(globalOXOCard.accel->getOrientation()) == UP;
}

bool isOrientationDown() {
  return byte(globalOXOCard.accel->getOrientation()) == DOWN;
}

bool isOrientationHorizontally() {
  return byte(globalOXOCard.accel->getOrientation()) == HORIZONTALLY;
}

bool isOrientationVertically() {
  return byte(globalOXOCard.accel->getOrientation()) == VERTICALLY;
}

/* BLE functions ---------------------------------------------- */
void setupAsIBeacon(uint16_t beacon_nr) {    // 1... 65'534 (0xFFFE)
  globalOXOCard.setupAsIBeacon(beacon_nr);
}

/* ------------------------------------- */
int findIBeacon(uint16_t beacon_nr) {
  return globalOXOCard.findIBeacon(beacon_nr);
}

/* Tone functions --------------------------------------------- */
void tone(unsigned int frequency) {
  tone(PIN_NR_PIEZO, frequency, 0);
}

void tone(int frequency, int duration) {
  tone(PIN_NR_PIEZO, (unsigned int)(frequency), (unsigned long)(duration));
}

void noTone() {
  noTone(PIN_NR_PIEZO);
}

void playMelody(int tones[], int lengths[], int size, int pause = 100) {
  for (int i = 0;i<size;i++) {
    tone(tones[i],lengths);
    delay(lengths[i]);
    noTone();
    delay(pause);
  }


}

/* Interrupts ------------------------------------------------- */
/** ===========================================================
 * \fn      ISR (timer1 OCIE1A interrupt TIMER1_COMPA)
 * \brief   interrupt service routine (handler) which will be
 *          called from time to time
 *          (f_isr = fcpu / (prescaler * frequency_divisor))
 *
 * \param   'TIMER1_COMPA vector'
 * \return  -
 ============================================================== */
ISR (TIMER1_COMPA_vect)
{
  autoTurnOffCnt++;
  if (autoTurnOffCnt >= autoTurnOffAfter)
  {
    autoTurnOffCnt = 0;
    goingToTurnOff = true;
  }
}

#endif

/**
 * @}
 */
