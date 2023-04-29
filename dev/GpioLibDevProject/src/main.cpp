#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include "../../../src/GpioExpanderLib.h"

// Pins for interrupt
#define EXPANDER_INT_PIN 4      // microcontroller pin attached to INTA/B

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

  expander.Init(&mcp, EXPANDER_INT_PIN);
}

void loop() 
{
  // put your main code here, to run repeatedly:
}