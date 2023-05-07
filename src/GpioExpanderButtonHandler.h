#ifndef GPIOEXPANDERBUTTONHANDLER_H
#define GPIOEXPANDERBUTTONHANDLER_H

static QueueHandle_t xGpioExpanderButtonEventQueue;

enum GpioExpanderButtonEventEnum {Pressed, Released};

struct GpioExpanderButtonEvent
{
    uint8_t pin;
    GpioExpanderButtonEventEnum event;
    GpioExpander* expander;
};

void GpioExpanderButtonHandler(GpioExpander* expander, uint16_t pin, GpioExpanderButton* device, uint16_t state) 
{

#if GPIOEXPANDERBUTTONS_FLASH_BUILTIN_LED == TRUE
    // flash the LED in debug mode
    digitalWrite(LED_BUILTIN, HIGH);
#endif
    unsigned long now = millis();
    bool track = false;

    digitalWrite(33, state);
    
    // check if this is a change and if the change needs to be debounced
    if (device->lastState != state )
    {
        // the state has changed
        if (now - device->lastStateChange < 20)
        {
            // this is too fast.  Ignore
            Serial.print("debounce ");
            Serial.println (state);
            device->lastStateChange = now;
            return;
        }
    }
    else
    {
        // temporary - no change, return
        Serial.print ("no change ");
        Serial.println (state);
        return;
    }
    // check the mode for this device
    if ((device->mode == LOW || device->mode == CHANGE) && state == LOW)
    {
        track = true;
    }
    else if ((device->mode == HIGH || device->mode == CHANGE) && state == HIGH)
    {
        track = true;
    }
    // check if this is a button press (versus a release)
    if (track)
    {
        GpioExpanderButtonEvent event;
        event.expander = expander;
        event.pin = pin;
        event.event = (state == LOW)?Pressed: Released;

        digitalWrite(15, HIGH);

        // send a button press to the queue
        xQueueSend( xGpioExpanderButtonEventQueue, &event, portMAX_DELAY);

        digitalWrite(15, LOW);
    }

    device->lastState = state;
    device->lastStateChange = now;
    digitalWrite(32, state);

#if GPIOEXPANDERBUTTONS_FLASH_BUILTIN_LED == TRUE
    // clear the LED flash in debug mode
    digitalWrite(LED_BUILTIN, LOW);
#endif
}

#endif // GPIOEXPANDERBUTTONHANDLER_H