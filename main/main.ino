#include <Encoder.h>

// Encoder inputs (One of these two have to be a interrupt pin )
#define encoderPinA 3
#define encoderPinB 2

// LED Outputs
#define ledCW 8
#define ledCCW 9

Encoder encoder(encoderPinA, encoderPinB);

long encoderPosition = 0;

void setup()
{
  pinMode(ledCCW, OUTPUT);
  pinMode(ledCW, OUTPUT);

  Serial.begin(9600);
}

void loop()
{
  if (Serial.available())
  {
    String command = Serial.readStringUntil('\n');
    if (command.equals("reset"))
    {
      Serial.println("-----------------------------");
      Serial.println("reset");
      Serial.println("-----------------------------");
      encoder.write(0);
    }
  }

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
}
