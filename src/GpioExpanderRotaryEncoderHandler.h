#ifndef GPIOEXPANDERROTARYENCODERHANDLER_H
#define GPIOEXPANDERROTARYENCODERHANDLER_H

// structure to define rotary movement and latch behavior
// default is to latch on values of 0 and 3 (index of 1, 3)
static uint8_t GpioExpanderRotaryMovement[4] = {2, 0, 1, 3};
static uint8_t GpioExpanderRotaryMovements = 4;

static QueueHandle_t xGpioExpanderRotaryEncoderEventQueue;

static uint8_t GpioExpanderRotaryEncoderFindPositionIndex (uint8_t positionValue)
{
    for (int i=0; i<4; i++)
    {
        if (positionValue == GpioExpanderRotaryMovement[i])
        {
            return i;
        }
    }
    
    // we should never get here
    return 255;
}

struct GpioExpanderRotaryEncoderEvent
{
    uint8_t pin;
    GpioExpanderRotaryEncoderEventEnum event;
    GpioExpander* expander;
    GpioExpanderRotaryEncoder* device;
    unsigned long eventMillis;
};

void GpioExpanderRotaryEncoderHandler(GpioExpander* expander, GpioExpanderRotaryEncoder* device,  uint8_t pin1State, uint8_t pin2State) 
{
    #if GPIOEXPANDERBUTTONS_FLASH_BUILTIN_LED == TRUE
    // flash the LED in debug mode
    digitalWrite(LED_BUILTIN, HIGH);
#endif
    unsigned long now = millis();
    bool isEvent = false;
    
    // stage the event
    GpioExpanderRotaryEncoderEvent event;
    event.expander = expander;
    event.device = device;
    event.eventMillis = now;

    uint8_t positionValue = pin2State * 2 + pin1State;
    uint8_t positionIndex = GpioExpanderRotaryEncoderFindPositionIndex(positionValue);

    uint8_t lastPositionValue = device->pin2State * 2 + device->pin1State;
    uint8_t lastPositionIndex = GpioExpanderRotaryEncoderFindPositionIndex(lastPositionValue);

    uint8_t delta = (positionIndex - lastPositionIndex) & 3;

    Serial.print(now);
    Serial.print("= ");

    Serial.print("device ");
    Serial.print(device->index);
    Serial.print(": ");

    Serial.print("ms = ");
    Serial.print(device->lastMovementMs);
    Serial.print (" : ");

    Serial.print ("pos=");
    Serial.print (positionValue);
    Serial.print (" - ");

    Serial.print ("index=");
    Serial.print (positionIndex);
    Serial.print (" : ");

    Serial.print("last=");
    Serial.print(lastPositionValue);
    Serial.print (" : ");

    Serial.print("delta=");
    Serial.print(delta);

    Serial.print (" : ");
    
    GpioExpanderRotaryEncoderEventEnum direction;

    // check if the position has changed
    if (positionValue != lastPositionValue)
    {
        // we have some change in position
        // calculate if we have moved forward or backwards
        switch (delta)
        {
            case 1:
                // we have moved forward one click
                device->lastMovement = Clockwise;
                break;
            case 3:
                // we have moved backwards one click
                device->lastMovement = CounterClockwise;
                break;
            default:
                // need to trust previous movement
                Serial.println ("jumped");
                break;
        }

        // check if we are on an indent
        if(positionValue == 0 || positionValue == 3)
        {
            isEvent = true;
            Serial.print ("moved");

            event.event = device->lastMovement;
        }
        else
        {
            Serial.print ("transitional");
        }

        device->lastMovementMs = now;
        device->pin1State = pin1State;
        device->pin2State = pin2State;
    }
    else
    {
        // no change
        Serial.print ("no change");
    }
    Serial.println();

    if (isEvent)
    {
        // send a rotary encoder movement to the queue
        //xQueueSend( xGpioExpanderRotaryEncoderEventQueue, &event, portMAX_DELAY);

        Serial.println();
        Serial.print("Move ");
        Serial.print(device->index);
        if(event.event == Clockwise)
        {
            Serial.println(" Clockwise");    
        }
        else
        {
            Serial.println(" Counter-Clockwise");    

        }
        Serial.println();
    }

    // Serial.println();

#if GPIOEXPANDERBUTTONS_FLASH_BUILTIN_LED == TRUE
    // clear the LED flash in debug mode
    digitalWrite(LED_BUILTIN, LOW);
#endif
}
#endif //GPIOEXPANDERROTARYENCODERHANDLER_H