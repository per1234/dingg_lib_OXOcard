/**-----------------------------------------------------------------------------
 * \file    OXOcard.cpp
 * \author  jh, tg
 * \date    xx.02.2017
 * @{
 -----------------------------------------------------------------------------*/

/* Includes --------------------------------------------------- */
#include "OXOcard.h"

/* Global variables ------------------------------------------- */
unsigned int autoTurnOffAfter = -1;
volatile unsigned int autoTurnOffCnt = 0;
volatile bool goingToTurnOff = false;

/* Public ----------------------------------------------------- */
/* System functions */
/** ===========================================================
 * \fn      begin
 * \brief   initializes the OXOcard
 ============================================================== */
void OXOcard::begin()
{
  DebugOXOcard_begin(DEBUG_BAUDRATE_OXOCARD);
  DebugOXOcard_println();

  initPins();
  disableUnusedCpuFunctions();
  initTimerIRQ(1, 1024, 7812);   // interrupt every 1 Second

  /* init LED matrix (~30ms) */
  DebugOXOcard_println(F("init LED matrix"));
  matrix = new IS31FL3731(Wire, &P_EN_LED_DRIVER, EN_LED_DRIVER, 8, 8);
  matrix->begin(400000L);
  matrix->clear();

  /* init accelerometer (~2ms) */
  DebugOXOcard_println(F("init accelerometer"));
  accel = new MMA7660FC(Wire);
  accel->begin(400000L);

  /* init BLE (~1500ms) */
  DebugOXOcard_println(F("init BLE"));
  bleSerial = new SoftwareSerial0(PIN_NR_SW_RXD, PIN_NR_SW_TXD);
  ble = new HM11_SoftwareSerial0(*bleSerial, &P_SW_RXD, SW_RXD, &P_SW_TXD, SW_TXD, &P_EN_BLE, EN_BLE, &P_RST_BLE, RST_BLE);
  //ble->forceRenew();
  ble->begin(BAUDRATE_BLE);
}

/** ===========================================================
 * \fn      turnOff
 * \brief   turns off the device by putting it in a power down
 *          sleep-mode, it will wake up again if INTn occures
 *
 * \requ    <avr/interrupt.h> and <avr/sleep.h>
 *
 * \param   leftButton    enable wake-up by the left button
 * \param   middleButton  enable wake-up by the middle button
 * \param   rightButton   enable wake-up by the right button
 ============================================================== */
void OXOcard::turnOff(bool leftButton, bool middleButton, bool rightButton)
{
  DebugOXOcard_println(F("turnOff"));

  /* check periphery state */
  bool isBlueLEDEnabled = getLEDBlue();

  /* disable periphery */
  matrix->end();
  accel->end();
  ble->disable();
  clearBit(P_PIEZO, PIEZO);
  setLEDBlue(LOW);
  setLEDRed(LOW);

  /* disable HW-Serial */
  Serial.flush();
  uint8_t ucsrb = UCSR0B;
  UCSR0B = 0;
  clearBit(P_RXD, RXD);  // necessary!

  delay(1);

  /* all interrupts have to be disabled during turn off configurations */
  cli();

  if (leftButton)   // PCINT2
  {
    /* mask the correct pin */
    setBit(PCMSK2, PCINT20); // BUTTON1 = PCINT20

    /* clear the interrupt flags */
    setBit(PCIFR, PCIF2);

    /* enable external interrupt PCINTn (to wake up later) */
    setBit(PCICR, PCIE2);
  }

  if (middleButton) // INT1
  {
    /* define that a low-level of INTn generates an interrupt request */
    clearBit(EICRA, ISC11);
    clearBit(EICRA, ISC10);

    /* clear the interrupt flags */
    setBit(EIFR, INTF1);

    /* enable external interrupt INTn (to wake up later) */
    setBit(EIMSK, INT1);
  }

  if (rightButton)  // INT0
  {
    /* define that a low-level of INTn generates an interrupt request */
    clearBit(EICRA, ISC01);
    clearBit(EICRA, ISC00);

    /* clear the interrupt flags */
    setBit(EIFR, INTF0);

    /* enable external interrupt INTn (to wake up later) */
    setBit(EIMSK, INT0);
  }

  /* disable other interrupts */
  clearBit(TIMSK1, OCIE1A);   // autoTurnOff
  //clearBit(TIMSK2, OCIE2A);

  /* choose "Power-down" sleep mode */
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  /* enable sleep-mode */
  sleep_enable();

  /* reenable interrupts */
  sei();

  /* system actually sleeps here */
  sleep_cpu();

  /* zzzZZZzzzZZZ... */

  /* INTn occured (button click) -> wakes up! system continues here... */

  /* reenable other interrupts */
  setBit(TIMSK1, OCIE1A);
  //setBit(TIMSK2, OCIE2A);

  /* reenable HW-Serial */
  UCSR0B = ucsrb;
  setBit(P_RXD, RXD);

  /* reenable periphery */
  setLEDRed(HIGH);
  matrix->begin();
  accel->begin();
  ble->enable();
  if (isBlueLEDEnabled) setLEDBlue(HIGH);
}

/** ===========================================================
 * \fn      handleAutoTurnOff
 * \brief   handles the functinality to turnOff after a defined
 *          time
 *
 * \param   (uint) number of seconds until turnOff
 ============================================================== */
void OXOcard::handleAutoTurnOff(uint16_t seconds)
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
    turnOff();
  }
}

/** ===========================================================
 * \fn      isLeftButtonPressed
 * \brief   checks if the left button is pressed
 *
 * \return  true if button is pressed
 ============================================================== */
bool OXOcard::isLeftButtonPressed()
{
  bool pressed = false;
  if (button1Pressed)
  {
    autoTurnOffCnt = 0; // prevent autoturnoff after an action
    pressed = true;
  }
  return pressed;
}

/** ===========================================================
 * \fn      isMiddleButtonPressed
 * \brief   checks if the middle button is pressed
 *
 * \return  true if button is pressed
 ============================================================== */
bool OXOcard::isMiddleButtonPressed()
{
  bool pressed = false;
  if (button2Pressed)
  {
    autoTurnOffCnt = 0; // prevent autoturnoff after an action
    pressed = true;
  }
  return pressed;
}

/** ===========================================================
 * \fn      isRightButtonPressed
 * \brief   checks if the right button is pressed
 *
 * \return  true if button is pressed
 ============================================================== */
bool OXOcard::isRightButtonPressed()
{
  bool pressed = false;
  if (button3Pressed)
  {
    autoTurnOffCnt = 0; // prevent autoturnoff after an action
    pressed = true;
  }
  return pressed;
}

/* BLE functions */
/** ===========================================================
 * \fn      setupAsIBeacon
 * \brief   setup the BLE module of the OXOcard as iBeacon
 *
 * \param   (str)  iBeacon name (max. 20 characters)
 *          (enum) advertInterval
 * \return  -
 ============================================================== */
void OXOcard::setupAsIBeacon(String beaconName, HM11::advertInterval_t interv)
{
  if (beaconName.length() > 20)
  {
    DebugOXOcard_println("iBeacon name is too long! (max. 20 characters)");
    beaconName = beaconName.substring(0, 20);
  }

  /* convert to hex char array */
  HM11::iBeaconData_t iBeacon;
  iBeacon.name = BLE_NAME;
  iBeaconNameToIBeaconUUID(beaconName, &iBeacon);
  iBeacon.interv = interv;
  ble->setupAsIBeacon(&iBeacon);
  setLEDBlue(HIGH);
}

/** ===========================================================
 * \fn      setupAsIBeacon
 * \brief   setup the BLE module of the OXOcard as iBeacon
 *
 * \param   (uint) iBeacon number (1... 65534)
 *          (enum) advertInterval
 * \return  -
 ============================================================== */
void OXOcard::setupAsIBeacon(uint16_t beaconNr, HM11::advertInterval_t interv)
{
  HM11::iBeaconData_t iBeacon;
  iBeacon.name = BLE_NAME;
  iBeacon.uuid = BLE_DEFAULT_UUID;
  iBeacon.marjor = BLE_DEFAULT_MARJOR;
  iBeacon.minor = beaconNr;
  iBeacon.interv = interv;
  ble->setupAsIBeacon(&iBeacon);
  setLEDBlue(HIGH);
}

/** ===========================================================
 * \fn      findIBeacon
 * \brief   find near iBeacon with given iBeacon name
 *
 * \param   (str)  iBeacon name (max. 20 characters)
 * \return  (int)  txPower in dBm
 ============================================================== */
int16_t OXOcard::findIBeacon(String beaconName)
{
  static bool firstTime = true;
  int16_t txPower = 0;

  if (firstTime)
  {
    ble->setupAsIBeaconDetector();
    firstTime = false;
  }

  static HM11::iBeaconData_t iBeacon;
  iBeaconNameToIBeaconUUID(beaconName, &iBeacon);

  DebugOXOcard_println(F("detectIBeacon..."));
  if (ble->detectIBeacon(&iBeacon))
  {
    setLEDBlue(HIGH);
    DebugOXOcard_print(F("iBeacon.accessAddress = ")); DebugOXOcard_println(iBeacon.accessAddress);
    DebugOXOcard_print(F("iBeacon.uuid = ")); DebugOXOcard_println(iBeacon.uuid);
    DebugOXOcard_print(F("iBeacon.deviceAddress = ")); DebugOXOcard_println(iBeacon.deviceAddress);
    DebugOXOcard_print(F("iBeacon.marjor = ")); DebugOXOcard_println(iBeacon.marjor);
    DebugOXOcard_print(F("iBeacon.minor = ")); DebugOXOcard_println(iBeacon.minor);
    DebugOXOcard_print(F("iBeacon.txPower = ")); DebugOXOcard_println(iBeacon.txPower);
    txPower = iBeacon.txPower;
  }
  else
  {
    DebugOXOcard_println(F("no iBeacon found"));
    setLEDBlue(LOW);
  }
  DebugOXOcard_println();

  return txPower;
}

/** ===========================================================
 * \fn      findIBeacon
 * \brief   find near iBeacon with given number
 *
 * \param   (uint) iBeacon number (1... 65534)
 * \return  (int)  txPower in dBm
 ============================================================== */
int16_t OXOcard::findIBeacon(uint16_t beaconNr)
{
  static bool firstTime = true;
  int16_t txPower = 0;

  static HM11::iBeaconData_t iBeacon;
  iBeacon.minor = beaconNr;              // minor you wish to search for

  if (firstTime)
  {
    iBeacon.uuid = BLE_DEFAULT_UUID;      // UUID you wish to search for
    iBeacon.marjor = BLE_DEFAULT_MARJOR;  // marjor you wish to search for
    ble->setupAsIBeaconDetector();
    firstTime = false;
  }

  DebugOXOcard_println(F("detectIBeacon..."));
  if (ble->detectIBeacon(&iBeacon))
  {
    setLEDBlue(HIGH);
    DebugOXOcard_print(F("iBeacon.accessAddress = ")); DebugOXOcard_println(iBeacon.accessAddress);
    DebugOXOcard_print(F("iBeacon.uuid = ")); DebugOXOcard_println(iBeacon.uuid);
    DebugOXOcard_print(F("iBeacon.deviceAddress = ")); DebugOXOcard_println(iBeacon.deviceAddress);
    DebugOXOcard_print(F("iBeacon.marjor = ")); DebugOXOcard_println(iBeacon.marjor);
    DebugOXOcard_print(F("iBeacon.minor = ")); DebugOXOcard_println(iBeacon.minor);
    DebugOXOcard_print(F("iBeacon.txPower = ")); DebugOXOcard_println(iBeacon.txPower);
    txPower = iBeacon.txPower;
  }
  else
  {
    DebugOXOcard_println(F("no iBeacon found"));
    setLEDBlue(LOW);
  }
  DebugOXOcard_println();

  return txPower;
}

/* Private ---------------------------------------------------- */
/** ===========================================================
 * \fn      initPins
 * \brief   initializes used I/Os of the uC
 ============================================================== */
void OXOcard::initPins()
{
  DebugOXOcard_println(F("initPins"));

  /* Port B */
  DDRB =  0b00000010; // SW-TXD (must be defined as output!)
  PORTB = 0b00111000; // L

  /* Port C */
  DDRC =  0b00000111; // EN_LED_DRIVER, !BLUE,  !RED
  PORTC = 0b00000010; // L            , H    ,  L

  /* Port D */
  setBit(P_EN_BLE, EN_BLE);
  DDRD =  0b11100000; // PIEZO, !RST_BLE, !EN_BLE
  PORTD = 0b00111101; // L    , L       , H
}

/** ===========================================================
 * \fn      disableUnusedCpuFunctions
 * \brief   disables unused cpu-functions to reduce power
 *          consumption
 *
 * \note    uC: ATmega328P
 ============================================================== */
void OXOcard::disableUnusedCpuFunctions()
{
  DebugOXOcard_println(F("disableUnusedCpuFunctions"));

  setBit(ACSR, ACD);        // disable ACD
  clearBit(ADCSRA, ADEN);   // disable ADC

  /* set Power Reduction Register */
  setBit(PRR, PRADC);      // disable ADC
  //setBit(PRR, PRUSART0);    // disable USART0 -> used to debug
  setBit(PRR, PRSPI);      // disable SPI
  //setBit(PRR, PRTIM0);     // disable TIM0    -> used by the Arduino delay()-functions
  //setBit(PRR, PRTIM1);     // disable TIM1    -> used for automatic turnOff
  //setBit(PRR, PRTIM2);     // disable TIM2    -> used by the tone()-functions
  //setBit(PRR, PRTWI);      // disable TWI     -> used to control accelerometer and the LED matrix
}

/** ===========================================================
 * \fn      initTimerIRQ
 * \brief   initialize a timer in the Clear Timer on Compare
 *          Match (CTC) mode and enables its interrupt OCIEnA.
 *
 * \note    uC: ATtiny85, ATmega328P or ATmeag32U4
 *          implement: ISR (TIMERn_COMPA_vect)
 *          f_isr = ((fckl / prescaler) / (divisor + 1))
 *
 * \param   (byte) timer number (0, 1, 2, 3)
 *          (uint) prescaler value (1, 8, 64, 256, 1024)
 *          (uint) divisor value (0... 255 or 65535)
 * \return  -
 ============================================================== */
void OXOcard::initTimerIRQ(uint8_t timer_nr, uint16_t prescaler, uint16_t divisor)
{
  DebugOXOcard_println(F("initTimerIRQ"));

  cli();
  switch (prescaler)
  {
    case 1:    prescaler = uint8_t(0x01); break;
    case 8:    prescaler = uint8_t(0x02); break;
    case 64:   prescaler = uint8_t(0x03); break;
    case 256:  prescaler = uint8_t(0x04); break;
    case 1024: prescaler = uint8_t(0x05); break;
    default:
      DebugOXOcard_println(F("invalid TIM prescaler!"));
      for(;;);
  }
  switch (timer_nr)
  {
    case 0: // 8bit Timer/Counter
      TCCR0A = 0x02;            // enable CTC (Clear Timer on Compare match) mode
      TCCR0B = prescaler;       // select prescaler mode
      clearBit(TCCR0B, WGM02);  // enable CTC (Clear Timer on Compare match) mode
      #ifdef TIMSK0
        setBit(TIMSK0, OCIE0A);   // enable interrupt (register A)
      #else
        setBit(TIMSK, OCIE0A);    // enable interrupt (register A)
      #endif
      OCR0A = uint8_t(divisor); // set output compare register A
      TCNT0 = 0;                // reset counter register
      break;
    case 1: // 16bit Timer/Counter
      #ifdef TCCR1B
        TCCR1A = 0x00;
        TCCR1B = (TCCR1B & ~0x07) | prescaler;
        setBit(TCCR1B, WGM12);
      #else
        TCCR1 = (TCCR1 & ~0x0F) | prescaler;
      #endif
      #ifdef TIMSK1
        setBit(TIMSK1, OCIE1A);   // enable interrupt (register A)
      #else
        setBit(TIMSK, OCIE1A);    // enable interrupt (register A)
      #endif
      #ifdef OCR1AH
        OCR1AH = (divisor & 0xFF00) >> 8;
        OCR1AL = divisor & 0xFF;
      #else
        OCR1A = uint8_t(divisor);
      #endif
      TCNT1 = 0;
      break;
    #ifdef TCCR2A
    case 2: // 8bit Timer/Counter
      TCCR2A = 0x02;
      TCCR2B = prescaler;
      clearBit(TCCR2B, WGM22);
      setBit(TIMSK2, OCIE2A);
      OCR2A = uint8_t(divisor);
      TCNT2 = 0;
      break;
    #endif
    #ifdef TCCR3A
    case 3: // 16bit Timer/Counter
      TCCR3A = 0x00;
      TCCR3B = prescaler;
      setBit(TCCR3B, WGM32);
      setBit(TIMSK3, OCIE3A);
      OCR3AH = (divisor & 0xFF00) >> 8;
      OCR3AL = divisor & 0xFF;
      TCNT3 = 0;
      break;
    #endif
    default:
      DebugOXOcard_println(F("invalid timer_nr!"));
      for(;;);
  }
  sei();
}

/** ===========================================================
 * \fn      getLEDBlue
 * \brief   returns current pin state
 *
 * \param   -
 * \return  (bool) pin state
 ============================================================== */
inline bool OXOcard::getLEDBlue()
{
  return !getBit(PIN_LED_BLUE, LED_BLUE);
}

/** ===========================================================
 * \fn      iBeaconNameToIBeaconUUID
 * \brief   converts given iBeacon name string (max. 20 characters)
 *          into hex values for uuid, marjor and minor
 *
 * \param   (str) iBeacon name (max. 20 characters)
 *          iBeacon  iBeacon structure pointer (see struct in the HM11 header-file)
 * \return  -
 ============================================================== */
void OXOcard::iBeaconNameToIBeaconUUID(String beaconName, HM11::iBeaconData_t *iBeacon)
{
  char hexCharArray[20*2];
  for (uint8_t i = 0; i < 20*2; i++) hexCharArray[i] = '1';  // init with '1' since the HM-11 doesn't allow '0'
  //DebugOXOcard_print("String(hexCharArray) = "); DebugOXOcard_println(String(hexCharArray));
  for (uint8_t i = 0; i < beaconName.length(); i++)
  {
    String tempStr = ble->byteToHexString(uint8_t(beaconName[i]));
    hexCharArray[i*2]   = tempStr[0];
    hexCharArray[i*2+1] = tempStr[1];
  }
  //DebugOXOcard_print("String(hexCharArray) = "); DebugOXOcard_println(String(hexCharArray));

  iBeacon->uuid = "";
  for (uint8_t i = 0;    i < 16*2; i++) iBeacon->uuid.concat(hexCharArray[i]);
  //DebugOXOcard_print("iBeacon->uuid = "); DebugOXOcard_println(iBeacon->uuid);
  iBeacon->marjor = 0;
  for (uint8_t i = 16*2; i < 18*2; i++) iBeacon->marjor += uint8_t(hexCharArray[i] - '0');
  //DebugOXOcard_print("iBeacon->marjor = "); DebugOXOcard_println(iBeacon->marjor);
  iBeacon->minor = 0;
  for (uint8_t i = 18*2; i < 20*2; i++) iBeacon->minor  += uint8_t(hexCharArray[i] - '0');
  //DebugOXOcard_print("iBeacon->minor = "); DebugOXOcard_println(iBeacon->minor);
}

/* Interrupts ------------------------------------------------- */
/** ===========================================================
 * \fn      ISR (timer1 OCIE1A interrupt TIMER1_COMPA)
 * \brief   timer interrupt service routine (handler)
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

/** ===========================================================
 * \fn      ISR (external interrupt PCINT2)
 * \brief   interrupt service routine (handler) for the wake-up-
 *          interrupt (PCINT2)
 *
 * \requ    <avr/sleep.h>
 *
 * \param   'PCINT0 vector'
 * \return  -
 ============================================================== */
ISR(PCINT2_vect)
{
  /* disable wake up interrupt INTn */
  clearBit(PCICR, PCIE2); // BUTTON 1
  clearBit(EIMSK, INT1);  // BUTTON 2
  clearBit(EIMSK, INT0);  // BUTTON 3
  sleep_disable();
  DebugOXOcard_println(F("wkUpInt1 occured, waking up..."));
}

/** ===========================================================
 * \fn      ISR (external interrupt INT1)
 * \brief   interrupt service routine (handler) for the wake-up-
 *          interrupt (INT1)
 *
 * \requ    <avr/sleep.h>
 *
 * \param   'INT1 vector'
 * \return  -
 ============================================================== */
ISR(INT1_vect)
{
  /* disable wake up interrupt INTn */
  clearBit(PCICR, PCIE2); // BUTTON 1
  clearBit(EIMSK, INT1);  // BUTTON 2
  clearBit(EIMSK, INT0);  // BUTTON 3
  sleep_disable();
  DebugOXOcard_println(F("wkUpInt2 occured, waking up..."));
}

/** ===========================================================
 * \fn      ISR (external interrupt INT0)
 * \brief   interrupt service routine (handler) for the wake-up-
 *          interrupt (INT0)
 *
 * \requ    <avr/sleep.h>
 *
 * \param   'INT0 vector'
 * \return  -
 ============================================================== */
ISR(INT0_vect)
{
  /* disable wake up interrupt INTn */
  clearBit(PCICR, PCIE2); // BUTTON 1
  clearBit(EIMSK, INT1);  // BUTTON 2
  clearBit(EIMSK, INT0);  // BUTTON 3
  sleep_disable();
  DebugOXOcard_println("wkUpInt3 occured, waking up...");
}
