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

int btn1LastState = LOW;
int btn1CurrentState;
unsigned long btn1LastTimePressedTimestamp;
boolean wasBtn1LongActionCalled = false;

int btn2LastState = LOW;
int btn2CurrentState;
unsigned long btn2LastTimePressedTimestamp;
boolean wasBtn2LongActionCalled = false;

const unsigned long DEBUNCE_TIME = 50;
const unsigned long DELAY_BEFORE_ENCODER_SAVE_POSITION = 100;
const unsigned long SAVE_ENCODER_POSITION_AFTER_TIME = 50;
const unsigned long LONG_PRESS_TIME = 1500;

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
      encoder.write(0);
      EEPROM.put(encoderPositionAddress, encoder.read());
    }
  }

  // Handling encoder
  // ---------------------------

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
  // ---------------------------

  // Btn1
  if (btn1LastState != btn1CurrentState)
  {
    if (btn1CurrentState == LOW)
    {
      wasBtn1LongActionCalled = false;
      btn1LastTimePressedTimestamp = millis();
    }
    else
    {
      long elapsedTime = millis() - btn1LastTimePressedTimestamp;

      if (elapsedTime > 50 && elapsedTime < LONG_PRESS_TIME)
      {
        Serial.println("btn1 short action called");
        toogleCalibrated();
      }
    }
    btn1LastState = btn1CurrentState;
  }
  else if (btn1LastState == LOW && millis() - btn1LastTimePressedTimestamp > LONG_PRESS_TIME && !wasBtn1LongActionCalled)
  {
    wasBtn1LongActionCalled = true;
    Serial.println("btn1 long action called");
  }

  // Btn2
  if (btn2LastState != btn2CurrentState)
  {
    if (btn2CurrentState == LOW)
    {
      wasBtn2LongActionCalled = false;
      btn2LastTimePressedTimestamp = millis();
    }
    else
    {
      long elapsedTime = millis() - btn2LastTimePressedTimestamp;

      if (elapsedTime > 50 && elapsedTime < LONG_PRESS_TIME)
      {
        Serial.println("btn2 short action called");
      }
    }
    btn2LastState = btn2CurrentState;
  }
  else if (btn2LastState == LOW && millis() - btn2LastTimePressedTimestamp > LONG_PRESS_TIME && !wasBtn2LongActionCalled)
  {
    wasBtn2LongActionCalled = true;
    Serial.println("btn2 long action called");
  }
}
