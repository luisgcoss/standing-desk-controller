#include <Encoder.h>
#include <EEPROM.h>

// Encoder inputs (One of these two have to be a interrupt pin )
#define encoderPinA 3
#define encoderPinB 2

// Define buttons
#define btn1 6
#define btn2 7

// LED Outputs
#define ledGreen 8
#define ledRed 9

Encoder encoder(encoderPinA, encoderPinB);

long encoderPosition = 0;
long encoderPositionAddress = 0;

int btn1CurrentState;
int btn1LastState = LOW;
bool wasBtn1LongActionCalled = false;
unsigned long btn1LastTimePressedTimestamp;
long btn1ElapsedTimeSinceLastTimePressed = 0;

int btn2CurrentState;
int btn2LastState = LOW;
bool wasBtn2LongActionCalled = false;
unsigned long btn2LastTimePressedTimestamp;
long btn2ElapsedTimeSinceLastTimePressed = 0;

const unsigned long DEBUNCE_TIME = 50;
const unsigned long LONG_PRESS_TIME = 2000;
const unsigned long SAVE_ENCODER_POSITION_AFTER_TIME = 50;
const unsigned long DELAY_BEFORE_ENCODER_SAVE_POSITION = 100;
const unsigned long DELAY_BETWEEN_BT1_AND_BT2_LAST_TIME_PRESSED_TO_TOGGLE_CALIBRATION_MODE = 200;

long lastEncoderMovementTiemestamp = 0;
long lastEncoderPositionStoredInEeprom;

bool isCalibrated = false;
int EEPROMisCalibratedAddress = 10;

long lowestPosition = -100;
long EEPROMLowestPositionAddrees = 20;

long highestPosition = -100;
long EEPROMHighestPositionAddrees = 30;

void setCalibrated(bool calibrated)
{
  if (EEPROM.read(EEPROMisCalibratedAddress) == calibrated)
    return;

  if (!calibrated)
  {
    digitalWrite(ledRed, HIGH);
    digitalWrite(ledGreen, LOW);

    isCalibrated = false;
    lowestPosition = -100;
    highestPosition = -100;
    EEPROM.write(EEPROMisCalibratedAddress, false);
    Serial.println("----------SETTING_NO_CALIBRATED----------");
  }
  else
  {
    EEPROM.write(EEPROMisCalibratedAddress, true);
    digitalWrite(ledGreen, HIGH);
    digitalWrite(ledRed, LOW);
    Serial.println("----------SETTING_CALIBRATED----------");
  }
}

void setup()
{
  pinMode(ledRed, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(btn1, INPUT_PULLUP);
  pinMode(btn2, INPUT_PULLUP);

  EEPROM.get(encoderPositionAddress, lastEncoderPositionStoredInEeprom);
  encoder.write(lastEncoderPositionStoredInEeprom);

  isCalibrated = EEPROM.read(EEPROMisCalibratedAddress);

  if (isCalibrated)
  {
    digitalWrite(ledRed, LOW);
    digitalWrite(ledGreen, HIGH);

    EEPROM.get(EEPROMLowestPositionAddrees, lowestPosition);
    EEPROM.get(EEPROMHighestPositionAddrees, highestPosition);
  }
  else
  {
    digitalWrite(ledRed, HIGH);
    digitalWrite(ledGreen, LOW);
  }

  Serial.begin(9600);
}

void loop()
{
  btn1CurrentState = digitalRead(btn1);
  btn2CurrentState = digitalRead(btn2);

  if (Serial.available())
  {
    String command = Serial.readStringUntil('\n');
    if (command.equals("reset"))
    {
      Serial.println("-----------------------------");
      encoder.write(0);
      EEPROM.put(encoderPositionAddress, encoder.read());
      Serial.println("----------RESET_COMMAND----------");
    }
  }

  // Handling encoder

  long newEncoderPosition = encoder.read();
  if (newEncoderPosition != encoderPosition)
  {
    lastEncoderMovementTiemestamp = millis();
    encoderPosition = newEncoderPosition;

    if (encoderPosition < lowestPosition || encoderPosition > highestPosition)
    {
      setCalibrated(false);
    }

    Serial.print("Value:");
    Serial.println(encoderPosition);
  }

  // Saving encoder position

  EEPROM.get(encoderPositionAddress, lastEncoderPositionStoredInEeprom);
  if (millis() - lastEncoderMovementTiemestamp > DELAY_BEFORE_ENCODER_SAVE_POSITION && encoderPosition != lastEncoderPositionStoredInEeprom)
  {
    EEPROM.put(encoderPositionAddress, encoderPosition);
    Serial.println("----------ENCODER_POSITION_SAVED----------");
  }

  // Handling Btns states

  // Btn1
  btn1ElapsedTimeSinceLastTimePressed = millis() - btn1LastTimePressedTimestamp;
  if (btn1LastState != btn1CurrentState)
  {
    if (btn1CurrentState == LOW)
    {
      wasBtn1LongActionCalled = false;
      btn1LastTimePressedTimestamp = millis();
    }
    else
    {

      if (btn1ElapsedTimeSinceLastTimePressed > DEBUNCE_TIME && btn1ElapsedTimeSinceLastTimePressed < LONG_PRESS_TIME)
      {
        Serial.println("btn1 short action called");
      }
    }
    btn1LastState = btn1CurrentState;
  }
  else if (btn1LastState == LOW && btn2LastState == HIGH && btn1ElapsedTimeSinceLastTimePressed > LONG_PRESS_TIME && !wasBtn1LongActionCalled)
  {
    wasBtn1LongActionCalled = true;
    Serial.println("btn1 long action called");

    if (!isCalibrated)
    {
      // save desktop lowest position
      encoder.write(0);
      lowestPosition = 0;
      encoderPosition = 0;
      EEPROM.put(encoderPositionAddress, 0);
      EEPROM.put(EEPROMLowestPositionAddrees, 0);
      Serial.println("----------STORING_LOWEST_POSITION----------");

      if (highestPosition != -100 && highestPosition > lowestPosition)
      {
        setCalibrated(true);
      }
    }
  }

  // Btn2
  btn2ElapsedTimeSinceLastTimePressed = millis() - btn2LastTimePressedTimestamp;
  if (btn2LastState != btn2CurrentState)
  {
    if (btn2CurrentState == LOW)
    {
      wasBtn2LongActionCalled = false;
      btn2LastTimePressedTimestamp = millis();
    }
    else
    {
      if (btn2ElapsedTimeSinceLastTimePressed > DEBUNCE_TIME && btn2ElapsedTimeSinceLastTimePressed < LONG_PRESS_TIME)
      {
        Serial.println("btn2 short action called");
      }
    }
    btn2LastState = btn2CurrentState;
  }
  else if (btn2LastState == LOW && btn2ElapsedTimeSinceLastTimePressed > LONG_PRESS_TIME && !wasBtn2LongActionCalled)
  {
    if (
        btn1LastState == LOW && btn1ElapsedTimeSinceLastTimePressed > (LONG_PRESS_TIME - DELAY_BETWEEN_BT1_AND_BT2_LAST_TIME_PRESSED_TO_TOGGLE_CALIBRATION_MODE))
    {
      wasBtn1LongActionCalled = true;
      Serial.println("btn1 and btn2 long action called");
      setCalibrated(false);
    }
    else
    {
      Serial.println("btn2 long action called");

      if (!isCalibrated)
      {
        // save desktop highest position
        EEPROM.put(EEPROMHighestPositionAddrees, encoderPosition);
        highestPosition = encoderPosition;
        Serial.println("----------STORING_HIGHEST_POSITION----------");

        if (lowestPosition == 0)
        {
          setCalibrated(true);
        }
      }
    }
    wasBtn2LongActionCalled = true;
  }
}
