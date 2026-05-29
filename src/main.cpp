#include <Arduino.h>
#include <CircularBuffer.h>

// Define the pins for indicators
const uint8_t PIN_AIRBAG = 2;
const uint8_t PIN_CRUISE_MAIN = 3;
const uint8_t PIN_CRUISE_START = 4;
const uint8_t PIN_IMMOBILIZER = 5;
const uint8_t PIN_CHECK_ENGINE = 6;
const uint8_t PIN_BATTERY = 7;
const uint8_t PIN_OIL_PRESSURE = 8;
const uint8_t PIN_HEAD_LIGHT = 9;

const uint8_t PIN_LED = 10;

CircularBuffer<uint8_t, 68> uartBuffer;
uint8_t lastFuelLevel = 0;
uint8_t fuelValues[3] = {0};
uint8_t fuelIndex = 0;
bool fuelValuesFilled = false;

bool fuelLevelFound = false;
// Define the interval for sending data (300ms)
const unsigned long interval = 100;
unsigned long previousMillis = 0;
int full = 75; // old 60
int dim = 30;

void setup()
{

  Serial.begin(9600); // Initialize UART communication
  // Configure pins as inputs
  pinMode(PIN_CHECK_ENGINE, INPUT);
  pinMode(PIN_BATTERY, INPUT);
  pinMode(PIN_OIL_PRESSURE, INPUT);
  pinMode(PIN_CRUISE_MAIN, INPUT);
  pinMode(PIN_CRUISE_START, INPUT);
  pinMode(PIN_AIRBAG, INPUT);
  pinMode(PIN_HEAD_LIGHT, INPUT);
  pinMode(PIN_IMMOBILIZER, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  // analogWrite(10, 0);
  delay(200);

  // while (true)
  //{

  for (int i = 0; i <= 255; i++)
  {
    // Quadratic easing: start slow, end fast
    float t = (float)i / 255.0;
    float eased = t * t; // ease-in (quadratic)

    int pwm = (int)(eased * 255);
    analogWrite(10, pwm);

    delay(8); // Fixed delay — smooth ramp-up
  }

  for (int i = 255; i >= full; i--)
  {
    analogWrite(10, i); // Decrease brightness
    delay(5);
  }
  //  }
  // digitalWrite(PIN_LED, true);

  // Fast PWM, 8-bit resolution, non-inverting mode
  // TCCR1A = _BV(COM1B1) | _BV(WGM10); // Clear OC1B on compare match
  // TCCR1B = _BV(WGM12) | _BV(CS11);   // Fast PWM with prescaler = 8

  // Force 8-bit Fast PWM
  // TCCR1A = _BV(COM1B1) | _BV(WGM10);
  // TCCR1B = _BV(WGM12) | _BV(CS11); // Prescaler = 8

  // OCR1B = full; // 50% duty cycle (0–255)
}

void loop()
{

  if (!fuelLevelFound)
  {
    // Step 1: Receive Data into the Buffer
    while (Serial.available())
    {
      uint8_t incomingByte = Serial.read();
      if (!uartBuffer.isFull())
      {
        uartBuffer.push(incomingByte);
      }
      else
      {
        // Handle buffer overflow if necessary
      }
    }

    // Step 2: Validate Packets and Extract Fuel Level
    while (uartBuffer.size() >= 17)
    { // Ensure there are at least 17 bytes to process
      bool packetFound = false;
      for (int i = 0; i <= uartBuffer.size() - 17; i++)
      {
        if (uartBuffer[i + 1] == 42 && uartBuffer[i + 15] == 95 && uartBuffer[i + 16] == 161)
        {
          // Valid packet detected
          uint8_t fuelLevel = uartBuffer[i + 3]; // Extract fuel byte at index 3

          // Store the fuel level in the array
          fuelValues[fuelIndex] = fuelLevel;
          fuelIndex = (fuelIndex + 1) % 3;
          if (fuelIndex == 0)
          {
            fuelValuesFilled = true;
          }

          // Compare fuel values if the array is filled
          if (fuelValuesFilled)
          {
            if (fuelValues[0] == fuelValues[1] && fuelValues[1] == fuelValues[2])
            {
              lastFuelLevel = fuelValues[0]; // Update last confirmed fuel level
              // Serial.print("Fuel Level Updated: ");
              // Serial.println(lastFuelLevel);
              fuelLevelFound = true;
            }
            // else
            //{
            //  Serial.print("Fuel Level Inconsistent. Retaining Last Value: ");
            //  Serial.println(lastFuelLevel);
            //}
          }

          // Remove the processed packet from the buffer
          for (int j = 0; j < 17; j++)
          {
            uartBuffer.shift();
          }
          packetFound = true;
          break; // Exit the loop after processing the packet
        }
      }
      if (!packetFound)
      {
        // If no valid packet was found, remove the first byte and continue checking
        uartBuffer.shift();
      }
    }
  }

  unsigned long currentMillis = millis();
  //  Send pin states every interval
  if (currentMillis - previousMillis > interval)
  {
    previousMillis = currentMillis;
    // Toggle LED states
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    // digitalWrite(10, !digitalRead(10));

    bool checkEngineState = !digitalRead(PIN_CHECK_ENGINE);
    bool batteryState = !digitalRead(PIN_BATTERY);
    bool oilPressureState = !digitalRead(PIN_OIL_PRESSURE);
    bool cruiseMainState = !digitalRead(PIN_CRUISE_MAIN);
    bool cruiseStartState = !digitalRead(PIN_CRUISE_START);
    bool airbagState = !digitalRead(PIN_AIRBAG);
    bool HeadLightOn = !digitalRead(PIN_HEAD_LIGHT);
    bool immobilizerState = !digitalRead(PIN_IMMOBILIZER);
    // Send saved states to Serial Monitor
    Serial.print("[");
    Serial.print(airbagState);
    Serial.print(",");
    Serial.print(cruiseMainState);
    Serial.print(",");
    Serial.print(cruiseStartState);
    Serial.print(",");
    Serial.print(immobilizerState);
    Serial.print(",");
    Serial.print(checkEngineState);
    Serial.print(",");
    Serial.print(batteryState);
    Serial.print(",");
    Serial.print(oilPressureState);
    Serial.print(",");
    Serial.print(HeadLightOn);
    Serial.print(",");
    Serial.print(lastFuelLevel);
    Serial.println("]");
    Serial.flush();
    previousMillis = millis();
    fuelLevelFound = false;
    lastFuelLevel = 0;
    if (HeadLightOn)
    {
      analogWrite(10, dim);
    }
    else
    {
      analogWrite(10, full);
    }
  }
}
