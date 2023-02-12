#include <Encoder.h>
#include <EEPROM.h>

// Encoder inputs (One of these two have to be an interrupt pin )
#define encoderPinA 3
#define encoderPinB 2

// Define buttons
#define btn1 6
#define btn2 7

// Define buttons
#define trigerLower 10
#define trigerLift 11

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

int movingStatus = 0; // 0 = to standby, 1 = Moving up , -1 = Moving down

bool isCalibrated = false;

long motionRange = 0;
long EEPROMMotionRangeAddress = 20;

long positionOne = 0;
long EEPROMPositionOneAddress = 40;
long positionTwo = 0;
long EEPROMPositionTwoAddress = 60;

long movingTo = 0;

struct calibrationAux
{
  long value;
  bool isCalibrated;
};

calibrationAux calibrationAuxA = {value : 0, isCalibrated : false};
calibrationAux calibrationAuxB = {value : 0, isCalibrated : false};

void moveToPosition(long targetPosition)
{
  if (!isCalibrated)
  {
    Serial.println("----------CAN_NOT_MOVE_NO_CALIBRATED----------");
    return;
  }

  if (movingStatus != 0)
  {
    movingStatus = 0;
    digitalWrite(trigerLift, LOW);
    digitalWrite(trigerLower, LOW);
    return;
  }

  if (targetPosition < 0 || targetPosition > motionRange)
  {
    Serial.println("----------CAN_NOT_MOVE_OUT_OF_RANGE----------");
    return;
  }

  movingTo = targetPosition;
  if (encoderPosition > targetPosition)
  {
    digitalWrite(trigerLower, HIGH);
    movingStatus = -1;
  }
  else
  {
    movingStatus = 1;
    digitalWrite(trigerLift, HIGH);
  }
}

void setNotCalibrated()
{
  digitalWrite(ledRed, HIGH);
  digitalWrite(ledGreen, LOW);

  isCalibrated = false;
  calibrationAuxA = {value : 0, isCalibrated : false};
  calibrationAuxB = {value : 0, isCalibrated : false};
  EEPROM.put(EEPROMMotionRangeAddress, -100);
  Serial.println("----------SETTING_NO_CALIBRATED----------");
}

void setCalibrated(long range)
{
  motionRange = range;
  isCalibrated = true;
  EEPROM.put(EEPROMMotionRangeAddress, range);
  digitalWrite(ledGreen, HIGH);
  digitalWrite(ledRed, LOW);
  Serial.println("----------SETTING_CALIBRATED----------");
}

void setup()
{
  pinMode(ledRed, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(trigerLower, OUTPUT);
  pinMode(trigerLift, OUTPUT);
  pinMode(btn1, INPUT_PULLUP);
  pinMode(btn2, INPUT_PULLUP);

  EEPROM.get(EEPROMPositionOneAddress, positionOne);
  EEPROM.get(EEPROMPositionTwoAddress, positionTwo);
  EEPROM.get(encoderPositionAddress, lastEncoderPositionStoredInEeprom);
  encoder.write(lastEncoderPositionStoredInEeprom);

  EEPROM.get(EEPROMMotionRangeAddress, motionRange);

  isCalibrated = motionRange > 10;

  Serial.begin(9600);

  if (isCalibrated)
  {
    digitalWrite(ledRed, LOW);
    digitalWrite(ledGreen, HIGH);
  }
  else
  {
    digitalWrite(ledRed, HIGH);
    digitalWrite(ledGreen, LOW);
  }
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

    if (isCalibrated && (encoderPosition < 0 || encoderPosition > motionRange))
    {
      setNotCalibrated();
    }

    Serial.print("Value:");
    Serial.print(encoderPosition);
    Serial.print(" Motion:");
    Serial.println(motionRange);
  }

  // Saving encoder position

  EEPROM.get(encoderPositionAddress, lastEncoderPositionStoredInEeprom);
  if (millis() - lastEncoderMovementTiemestamp > DELAY_BEFORE_ENCODER_SAVE_POSITION && encoderPosition != lastEncoderPositionStoredInEeprom)
  {
    EEPROM.put(encoderPositionAddress, encoderPosition);
    Serial.println("----------ENCODER_POSITION_SAVED----------");
  }

  // Handling movement

  if (movingStatus > 0 && encoderPosition >= movingTo)
  {
    movingStatus = 0;
    digitalWrite(trigerLift, LOW);
  }
  else if (movingStatus < 0 && encoderPosition <= movingTo)
  {
    movingStatus = 0;
    digitalWrite(trigerLower, LOW);
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
        moveToPosition(positionOne);
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
      calibrationAuxA = {encoderPosition, isCalibrated : true};

      if (calibrationAuxB.isCalibrated)
      {
        long range = abs(calibrationAuxB.value - calibrationAuxA.value);

        if (calibrationAuxA.value > calibrationAuxB.value)
        {
          encoder.write(range);
          encoderPosition = range;
        }
        else
        {
          encoder.write(0);
          encoderPosition = 0;
        }

        setCalibrated(range);
      }
    }
    else
    {
      Serial.println("----------SAVING_POSITION_ONE----------");
      EEPROM.put(EEPROMPositionOneAddress, encoderPosition);
      positionOne = encoderPosition;
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
        moveToPosition(positionTwo);
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
      setNotCalibrated();
    }
    else
    {
      Serial.println("btn2 long action called");

      if (!isCalibrated)
      {
        calibrationAuxB = {encoderPosition, isCalibrated : true};

        if (calibrationAuxA.isCalibrated)
        {

          long range = abs(calibrationAuxB.value - calibrationAuxA.value);

          if (calibrationAuxB.value > calibrationAuxA.value)
          {
            encoder.write(range);
            encoderPosition = range;
          }
          else
          {
            encoder.write(0);
            encoderPosition = 0;
          }

          setCalibrated(range);
        }
      }
      else
      {
        Serial.println("----------SAVING_POSITION_TWO----------");
        EEPROM.put(EEPROMPositionTwoAddress, encoderPosition);
        positionTwo = encoderPosition;
      }
    }
    wasBtn2LongActionCalled = true;
  }
}
