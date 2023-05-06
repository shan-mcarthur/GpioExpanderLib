#include <Arduino.h>

// debug macro to enable flashing BUILTIN_LED on button events
#define GPIOEXPANDERBUTTONS_FLASH_BUILTIN_LED TRUE

#include <Adafruit_MCP23X17.h>
#include "GpioExpanderButtons.h"

// Pins for interrupt
#define EXPANDER_INT_PIN 14      // microcontroller pin attached to INTA/B

// global variables for GPIO expander
Adafruit_MCP23X17 mcp;
GpioExpander expander;

// global variable to handle dial value
long dial;

void setup() 
{
  Serial.begin(115200);
  
  // Configure pins
  pinMode(BUILTIN_LED, OUTPUT);

  // initialize the MCP23x17 chip
  if (!mcp.begin_I2C()) 
  {
    Serial.println("Error connecting to MCP23017 module.  Stopping.");
    while (1);
  }

  // initialize the button library with the set of pins to support as buttons
  expander.AddRotaryEncoder(0, 1, false);  // this is for rotary encoders that don't have a full HIGH->LOW->HIGH cycle between detents
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
        dial++;
        Serial.print ("> ");
        Serial.println (dial);
    }
    else
    {
        dial--;
        Serial.print ("< ");
        Serial.println (dial);
    }
  }
}