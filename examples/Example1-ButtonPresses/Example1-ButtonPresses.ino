#include <Arduino.h>

// debug macro to enable flashing BUILTIN_LED on button events
#define GPIOEXPANDERBUTTONS_FLASH_BUILTIN_LED TRUE

#include <Adafruit_MCP23X17.h>
#include "GpioExpanderButtons.h"

// Pins for interrupt
#define EXPANDER_INT_PIN 4      // microcontroller pin attached to INTA/B

// global variables for GPIO expander
Adafruit_MCP23X17 mcp;
GpioExpanderButtons expander;

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
  uint16_t pins = GPIOEXPANDERBUTTONS_PIN(0) 
                | GPIOEXPANDERBUTTONS_PIN(1) 
                | GPIOEXPANDERBUTTONS_PIN(2)
                | GPIOEXPANDERBUTTONS_PIN(3);
  expander.Init(&mcp, EXPANDER_INT_PIN, pins);
}

void loop() 
{
  // process all pending butten events in the queue
  while (uxQueueMessagesWaiting(xGpioExpanderButtonEventQueue))
  {
    // retrieve the next button event from the queue and process it
    GpioExpanderButtonsEvent event;

    // retrieve the event from the queue and obtain the pin info
    xQueueReceive(xGpioExpanderButtonEventQueue, &event, portMAX_DELAY);
    
    // dump the event details
    Serial.print ("pin ");
    Serial.print (event.pinNumber);
    Serial.println (" pressed");
  }
}