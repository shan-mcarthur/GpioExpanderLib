#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include "../../../src/GpioExpanderLib.h"

// Pins for interrupt
#define EXPANDER_INT_PIN 14      // microcontroller pin attached to INTA/B

// global variables for GPIO expander
Adafruit_MCP23X17 mcp;
GpioExpander expander;
GpioExpanderRotaryEncoder* rotaryDevices[2];

// global variable to handle dial value
long dials[2];

void setup() 
{
  Serial.begin(115200);
  
  // Configure pins
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(32, OUTPUT);
  pinMode(15, OUTPUT);
  pinMode(33, OUTPUT);

  // initialize the MCP23x17 chip
  if (!mcp.begin_I2C()) 
  {
    Serial.println("Error connecting to MCP23017 module.  Stopping.");
    while (1);
  }

  // initialize the button library with the set of pins to support as buttons
  rotaryDevices[0] = expander.AddRotaryEncoder(0, 1);
  expander.AddButton(2, CHANGE);

  rotaryDevices[1] = expander.AddRotaryEncoder(3, 4);
  expander.AddButton(5, CHANGE);
  expander.Init(&mcp, EXPANDER_INT_PIN);

  Serial.println ("");
  Serial.println ("");

}

void loop() 
{
  // process all pending butten events in the queue
  while (uxQueueMessagesWaiting(xGpioExpanderButtonEventQueue))
  {
    // retrieve the event from the queue and obtain the pin info
    GpioExpanderButtonEvent event;
    xQueueReceive(xGpioExpanderButtonEventQueue, &event, portMAX_DELAY);

    unsigned long now = micros();
    Serial.print (now);

    // dump the event details
    Serial.print (" pin ");
    Serial.print (event.pin);
    if (event.event == Pressed)
    {
      Serial.println(" Pressed");
    }
    else
    {
      Serial.println(" Released");
    }
  }

 // process all pending rotary encoder events in the queue
  while (uxQueueMessagesWaiting(xGpioExpanderRotaryEncoderEventQueue))
  {
    // retrieve the event from the queue and obtain the pin info
    GpioExpanderRotaryEncoderEvent event;
    xQueueReceive(xGpioExpanderRotaryEncoderEventQueue, &event, portMAX_DELAY);

    unsigned long now = millis();
    // Serial.print (now);
    // Serial.print (": ");
    int encoder;
    // check which encoder this is
    if (event.device == rotaryDevices[0])
    {
      encoder=0;
    }
    else
    {
      encoder=1;
    }

    // Serial.print(encoder);
    // Serial.print("\t");

    // dump the event details
    if (event.event == Clockwise)
    {
        dials[encoder]++;
        // Serial.print (" >>>> ");
    }
    else
    {
        dials[encoder]--;
        // Serial.print (" <<<< ");
    }
    // Serial.print("\t\t");
    // Serial.print(dials[0]);
    // Serial.print(" : ");
    // Serial.println(dials[1]);

    if (event.event != Clockwise)
    {
      Serial.println ("******  ERROR *****");
      Serial.print("device ");
      Serial.print(event.device->index);
      Serial.print(" ms=");
      Serial.print(event.eventMillis);
      Serial.println ("");
      Serial.println ("");
    }
  }
}