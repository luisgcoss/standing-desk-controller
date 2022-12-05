#include <Encoder.h>

// Encoder inputs (One of these two have to be a interrupt pin )
#define encoderPinA 3
#define encoderPinB 2

// Define buttons
#define btn1 6
#define btn2 7

// LED Outputs
#define ledCW 8
#define ledCCW 9

Encoder encoder(encoderPinA, encoderPinB);

long encoderPosition = 0;

int btn1LastState = LOW;
int btn1CurrentState;
unsigned long btn1LastTimePressedTimestamp;
boolean wasBtn1LongActionCalled = false;

int btn2LastState = LOW;
int btn2CurrentState;
unsigned long btn2LastTimePressedTimestamp;
boolean wasBtn2LongActionCalled = false;

int counter = 0;

const unsigned long DEBUNCE_TIME = 50;
const unsigned long LONG_PRESS_TIME = 1500;

void setup()
{
  pinMode(ledCCW, OUTPUT);
  pinMode(ledCW, OUTPUT);
  pinMode(btn1, INPUT_PULLUP);
  pinMode(btn2, INPUT_PULLUP);

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
      counter = 0;
    }
  }

  // Handling encoder
  // ---------------------------

  long newEncoderPosition = encoder.read();
  if (newEncoderPosition != encoderPosition)
  {
    if (newEncoderPosition < encoderPosition)
    {

      digitalWrite(ledCW, LOW);
      digitalWrite(ledCCW, HIGH);
    }
    else
    {
      digitalWrite(ledCW, HIGH);
      digitalWrite(ledCCW, LOW);
    }
    encoderPosition = newEncoderPosition;
    Serial.print("Value:");
    Serial.println(encoderPosition);
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
        counter++;
        Serial.print("btn1 short action called, Count: ");
        Serial.println(counter);
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
        counter--;
        Serial.print("btn2 short action called, Count: ");
        Serial.println(counter);
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
