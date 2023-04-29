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
    // check if this is a button press (versus a release)
    if (state == LOW)
    {
        GpioExpanderButtonEvent event;
        event.expander = expander;
        event.pin = pin;
        event.event = Pressed;

        // send a button press to the queue
        xQueueSend( xGpioExpanderButtonEventQueue, &event, portMAX_DELAY);
    }
#if GPIOEXPANDERBUTTONS_FLASH_BUILTIN_LED == TRUE
    // clear the LED flash in debug mode
    digitalWrite(LED_BUILTIN, LOW);
#endif
}

#endif // GPIOEXPANDERBUTTONHANDLER_H