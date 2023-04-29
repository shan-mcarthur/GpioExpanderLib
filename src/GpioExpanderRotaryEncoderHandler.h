#ifndef GPIOEXPANDERROTARYENCODERHANDLER_H
#define GPIOEXPANDERROTARYENCODERHANDLER_H

static QueueHandle_t xGpioExpanderRotaryEncoderEventQueue;

enum GpioExpanderRotaryEncoderEventEnum {Clockwise, CounterClockwise};

struct GpioExpanderRotaryEncoderEvent
{
    uint8_t pin;
    GpioExpanderRotaryEncoderEventEnum event;
    GpioExpander* expander;
};

void GpioExpanderRotaryEncoderHandler(GpioExpander* expander, uint8_t pin, GpioExpanderRotaryEncoder* device,  uint8_t pin1state, uint8_t pin2state) 
{
    #if GPIOEXPANDERBUTTONS_FLASH_BUILTIN_LED == TRUE
    // flash the LED in debug mode
    digitalWrite(LED_BUILTIN, HIGH);
#endif
    unsigned long now = millis();
    GpioExpanderRotaryEncoderEventEnum direction;

    if (pin == device->pin1)
    {
        if (pin1state == LOW)
        {
            // record the event
            device->pin1TimeMs = now;

            if (device->pin2TimeMs != 0 && now - device->pin2TimeMs < 2000)
            {
                // we have an unprocessed pin2 drop that we are following.
                // direction is backwards
                GpioExpanderRotaryEncoderEvent event;
                event.expander = expander;
                event.pin = pin;
                event.event = CounterClockwise;

                device->pin1TimeMs = 0;
                device->pin2TimeMs = 0;

                // send a rotary encoder movement to the queue
                xQueueSend( xGpioExpanderRotaryEncoderEventQueue, &event, portMAX_DELAY);
            }
        }
        else
        { 
            // reset things if the pin goes up
            device->pin1TimeMs = 0;
        }
    }
    else if (pin == device->pin2)
    {
        if (pin2state == LOW)
        {
            // record the event
            device->pin2TimeMs = now;

            if (device->pin1TimeMs != 0 && now - device->pin1TimeMs < 2000)
            {
                // we have an unprocessed pin1 drop that we are following.
                // direction is forwards
                GpioExpanderRotaryEncoderEvent event;
                event.expander = expander;
                event.pin = pin;
                event.event = Clockwise;
                
                device->pin1TimeMs = 0;
                device->pin2TimeMs = 0;

                // send a rotary encoder movement to the queue
                xQueueSend( xGpioExpanderRotaryEncoderEventQueue, &event, portMAX_DELAY);
            }
        }
        else
        { 
            // reset things if the pin goes up
            device->pin2TimeMs = 0;
        }
    }

#if GPIOEXPANDERBUTTONS_FLASH_BUILTIN_LED == TRUE
    // clear the LED flash in debug mode
    digitalWrite(LED_BUILTIN, LOW);
#endif
}
#endif //GPIOEXPANDERROTARYENCODERHANDLER_H