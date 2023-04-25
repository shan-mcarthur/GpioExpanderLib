#include <Arduino.h>
// This is an example of how to use multiple MCP23017 boards
// you need to instantiate multiple instances of the Adafruit_MCP23X17 objects and initialize them with the appropriate device ID
// also needed is to instantiate multiple instances of the GpioExpanderButtons class.
// the button event will have a pointer to the class that initiated the button press event.

// debug macro to enable flashing BUILTIN_LED on button events
#define GPIOEXPANDERBUTTONS_FLASH_BUILTIN_LED TRUE

#include <Adafruit_MCP23X17.h>
#include "GpioExpanderButtons.h"

// Pins for interrupts of both expanders
#define EXPANDER1_INT_PIN 4
#define EXPANDER2_INT_PIN 14

// global variables for GPIO expander
Adafruit_MCP23X17 mcp1;
Adafruit_MCP23X17 mcp2;
GpioExpanderButtons expander1;
GpioExpanderButtons expander2;

void setup() 
{
  Serial.begin(115200);
  
  // Configure pins
  pinMode(BUILTIN_LED, OUTPUT);

  // initialize the MCP23x17 chip for the first expander
  if (!mcp1.begin_I2C(0x20)) 
  {
    Serial.println("Error connecting to first MCP23017 module.  Stopping.");
    while (1);
  }

  // initialize the MCP23x17 chip for the second expander
  if (!mcp2.begin_I2C(0x21)) 
  {
    Serial.println("Error connecting to second MCP23017 module.  Stopping.");
    while (1);
  }

  // initialize the button library with the set of pins to support as buttons on the first board
  uint16_t pins1 = GPIOEXPANDERBUTTONS_PIN(0) 
                | GPIOEXPANDERBUTTONS_PIN(1) 
                | GPIOEXPANDERBUTTONS_PIN(2)
                | GPIOEXPANDERBUTTONS_PIN(3);
  expander1.Init(&mcp1, EXPANDER1_INT_PIN, pins1);

  // initialize the button library with the set of pins to support as buttons on the second board
  uint16_t pins2 = GPIOEXPANDERBUTTONS_PIN(0) 
                | GPIOEXPANDERBUTTONS_PIN(1) 
                | GPIOEXPANDERBUTTONS_PIN(2)
                | GPIOEXPANDERBUTTONS_PIN(3);
  expander2.Init(&mcp2, EXPANDER2_INT_PIN, pins2);
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
    Serial.print (" on expander ");
    Serial.print (event.expander == &expander1?"1":"2");
    Serial.println (" pressed");
  }
}