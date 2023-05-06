#ifndef GPIOEXPANDERROTARYENCODERHANDLER_H
#define GPIOEXPANDERROTARYENCODERHANDLER_H

static QueueHandle_t xGpioExpanderRotaryEncoderEventQueue;

enum GpioExpanderRotaryEncoderEventEnum {Clockwise, CounterClockwise};

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

    // Serial.print("pin1TimeMs=");
    // Serial.print(device->pin1TimeMs);
    // Serial.print(" pin2TimeMs=");
    // Serial.println(device->pin2TimeMs);

    Serial.print(now);
    Serial.print("= ");

    Serial.print("device ");
    Serial.print(device->index);
    Serial.print(": ");

    Serial.print("ms = ");
    Serial.print (device->pin1TimeMs);
    Serial.print (" ");
    Serial.print (device->pin2TimeMs);
    Serial.print (" : ");

    Serial.print("last = ");
    Serial.print (device->pin1State);
    Serial.print (" ");
    Serial.print (device->pin2State);
    Serial.print (" : ");

    Serial.print("new = ");
    Serial.print (pin1State);
    Serial.print (" ");
    Serial.print (pin2State);
    Serial.print (" : ");

    GpioExpanderRotaryEncoderEventEnum direction;

    // check if pin1 has changed
    if (pin1State != device->pin1State)
    {
        Serial.print("pin 1 ");
        // Serial.print(pin1State);
        // Serial.print(") ");
        // Serial.print(device->pin1);
        // Serial.print(" : ");

        if (pin1State == LOW)
        {
            Serial.print("LOW ");
            // record the event
            device->pin1TimeMs = now;

            if (device->pin2TimeMs != 0 && now - device->pin2TimeMs < device->debounceMs)
            {
                // we have an unprocessed pin2 drop that we are following.
                // direction is backwards
                event.event = CounterClockwise;
                isEvent = true;
            }
            else
            {
                // no recent event to follow, but clear the other timer
                device->pin2TimeMs = 0;
            }
        }
        else
        { 
            Serial.print("HIGH ");
            if (device->fullCycleBetweenDetents)
            {
                // we can ignore the transitions back to high as this encoder has a full cycle HIGH->LOW->HIGH for every detent
                Serial.write ("skip");

                // reset things if the pin goes up
                device->pin1TimeMs = 0;
            }
            else
            {
                // record the event
                device->pin1TimeMs = now;

                if (device->pin2TimeMs != 0 && now - device->pin2TimeMs < device->debounceMs)
                {
                // we have an unprocessed pin2 drop that we are following.
                // direction is backwards
                event.event = CounterClockwise;
                isEvent = true;
                }
                else
                {
                    // no recent event to follow, but clear the other timer
                    device->pin2TimeMs = 0;
                }
            }
        }
    }
    else if (pin2State != device->pin2State)
    {
        Serial.print("pin 2 ");
        if (pin2State == LOW)
        {
            Serial.print("LOW ");
            // record the event
            device->pin2TimeMs = now;

            if (device->pin1TimeMs != 0 && now - device->pin1TimeMs < device->debounceMs)
            {
                // we have an unprocessed pin1 drop that we are following.
                // direction is forwards
                event.event = Clockwise;
                isEvent = true;
            }
            else
            {
                // no recent event to follow, but clear the other timer
                device->pin1TimeMs = 0;
            }
        }
        else
        { 
            Serial.print("HIGH ");
            if (device->fullCycleBetweenDetents)
            {
                // we can ignore the transitions back to high as this encoder has a full cycle HIGH->LOW->HIGH for every detent
                Serial.write ("skip");

                // reset things if the pin goes up
                device->pin2TimeMs = 0;
            }
            else
            {
                // record the event
                device->pin2TimeMs = now;

                if (device->pin1TimeMs != 0 && now - device->pin1TimeMs < device->debounceMs)
                {
                // we have an unprocessed pin1 drop that we are following.
                // direction is forward
                event.event = Clockwise;
                isEvent = true;
                }
                else
                {
                    // no recent event to follow, but clear the other timer
                    device->pin1TimeMs = 0;
                }
            }

        }
    }
    else
    {
        // no change
        Serial.print ("no change in pins");
    }
    Serial.println();

    device->pin1State = pin1State;
    device->pin2State = pin2State;

    if (isEvent)
    {
        // send a rotary encoder movement to the queue
        xQueueSend( xGpioExpanderRotaryEncoderEventQueue, &event, portMAX_DELAY);
        device->pin1TimeMs = 0;
        device->pin2TimeMs = 0;

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