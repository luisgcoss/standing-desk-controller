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
boolean wasBtn1LongActionCalled = false;
unsigned long btn1LastTimePressedTimestamp;
long btn1ElapsedTimeSinceLastTimePressed = 0;

int btn2CurrentState;
int btn2LastState = LOW;
boolean wasBtn2LongActionCalled = false;
unsigned long btn2LastTimePressedTimestamp;
long btn2ElapsedTimeSinceLastTimePressed = 0;

const unsigned long DEBUNCE_TIME = 50;
const unsigned long LONG_PRESS_TIME = 2000;
const unsigned long SAVE_ENCODER_POSITION_AFTER_TIME = 50;
const unsigned long DELAY_BEFORE_ENCODER_SAVE_POSITION = 100;
const unsigned long DELAY_BETWEEN_BT1_AND_BT2_LAST_TIME_PRESSED_TO_TOGGLE_CALIBRATION_MODE = 200;

long lastEncoderMovementTiemestamp = 0;
long lastEncoderPositionStoredInEeprom;

int EEPROMisCalibratedAddrees = 250;

void toogleCalibrated()
{
  if (EEPROM.read(EEPROMisCalibratedAddrees))
  {
    EEPROM.write(EEPROMisCalibratedAddrees, false);
    digitalWrite(ledGreen, LOW);
    digitalWrite(ledRed, HIGH);
  }
  else
  {
    EEPROM.write(EEPROMisCalibratedAddrees, true);
    digitalWrite(ledGreen, HIGH);
    digitalWrite(ledRed, LOW);
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

  if (EEPROM.read(EEPROMisCalibratedAddrees))
  {
    digitalWrite(ledGreen, HIGH);
    digitalWrite(ledRed, LOW);
  }
  else
  {
    digitalWrite(ledGreen, LOW);
    digitalWrite(ledRed, HIGH);
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
      Serial.println("reset");
      Serial.println("-----------------------------");
      encoder.write(0);
      EEPROM.put(encoderPositionAddress, encoder.read());
    }
  }

  // Handling encoder

  long newEncoderPosition = encoder.read();
  if (newEncoderPosition != encoderPosition)
  {
    lastEncoderMovementTiemestamp = millis();
    encoderPosition = newEncoderPosition;
    Serial.print("Value:");
    Serial.println(encoderPosition);
  }

  // Saving encoder position

  EEPROM.get(encoderPositionAddress, lastEncoderPositionStoredInEeprom);
  if (millis() - lastEncoderMovementTiemestamp > DELAY_BEFORE_ENCODER_SAVE_POSITION && encoderPosition != lastEncoderPositionStoredInEeprom)
  {
    Serial.println("Encoder poisition saved");
    EEPROM.put(encoderPositionAddress, encoderPosition);
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
        toogleCalibrated();
      }
    }
    btn1LastState = btn1CurrentState;
  }
  else if (btn1LastState == LOW && btn1ElapsedTimeSinceLastTimePressed > LONG_PRESS_TIME && !wasBtn1LongActionCalled)
  {
    wasBtn1LongActionCalled = true;
    Serial.println("btn1 long action called");
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
    }
    else
    {
      Serial.println("btn2 long action called");
    }
    wasBtn2LongActionCalled = true;
  }
}
