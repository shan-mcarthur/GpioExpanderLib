#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include "../../../src/GpioExpanderLib.h"

// Pins for interrupt
#define EXPANDER_INT_PIN 14      // microcontroller pin attached to INTA/B

Adafruit_MCP23X17 mcp;
GpioExpander expander;

void setup() 
{
  Serial.begin(115200);
  
  // Configure pins
  pinMode(LED_BUILTIN, OUTPUT);

  // initialize the MCP23x17 chip
  if (!mcp.begin_I2C()) 
  {
    Serial.println("Error connecting to MCP23017 module.  Stopping.");
    while (1);
  }

  expander.AddRotaryEncoder(0, 1);
  expander.AddButton(2);
  expander.Init(&mcp, EXPANDER_INT_PIN);
}

void loop() 
{
  // process all pending butten events in the queue
  while (uxQueueMessagesWaiting(xGpioExpanderButtonEventQueue))
  {
    // retrieve the next button event from the queue and process it
    GpioExpanderButtonEvent event;

    // retrieve the event from the queue and obtain the pin info
    xQueueReceive(xGpioExpanderButtonEventQueue, &event, portMAX_DELAY);
    
    // dump the event details
    Serial.print ("pin ");
    Serial.print (event.pin);
    Serial.println (" pressed");
  }

  // process all pending rotary encoder events in the queue
  while (uxQueueMessagesWaiting(xGpioExpanderRotaryEncoderEventQueue))
  {
    // retrieve the next button event from the queue and process it
    GpioExpanderRotaryEncoderEvent event;

    // retrieve the event from the queue and obtain the pin info
    xQueueReceive(xGpioExpanderRotaryEncoderEventQueue, &event, portMAX_DELAY);
    
    // dump the event details
    if (event.event == Clockwise)
    {
        Serial.println (">");
    }
    else
    {
        Serial.println ("<");
    }
  }

}