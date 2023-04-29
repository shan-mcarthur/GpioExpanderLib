#ifndef GPIOEXPANDERBUTTONS_H
#define GPIOEXPANDERBUTTONS_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>

// task notification and queue for interrupt and background processing
static TaskHandle_t xGpioExpanderButtonsTaskToNotify;


void GpioExpanderButtonServiceTask(void *parameter) 
{
    uint16_t allPins;   //pin status while this interrupt occured
    static uint32_t thread_notification;
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 500 );
    uint32_t ulNotifiedValue;

    // continuously process new task notifications from interrupt handler
    while(1) 
    {
        // wait for a task notification raised from the interrupt handler
        thread_notification = xTaskNotifyWait(pdTRUE, ULONG_MAX, &ulNotifiedValue, portMAX_DELAY);

        if (thread_notification == pdPASS)
        {
#if GPIOEXPANDERBUTTONS_FLASH_BUILTIN_LED == TRUE
            // flash the LED in debug mode
            digitalWrite(LED_BUILTIN, HIGH);
#endif
            // find the correct expander
            bool bDone = false;
            GpioExpander *expander = nullptr;
            uint8_t expanderNumber = 255;

            // loop through the array until you find one that has a pending interrupt
            for (int i=0; i<GPIOEXPANDER_MAX_EXPANDERS && !bDone; i++)
            {
                expander = GlobalGpioExpanders[i];
                if (expander != nullptr)
                {
                    // check to see if the interrupt line is low
                    if (digitalRead(expander->GetInterruptPin()) == LOW)
                    {
                        // this one has an interrupt for us
                        bDone = true;
                        expanderNumber = i;
                    }
                }
            }

            // check that we found one interrupt line that was active
            if (expanderNumber != 255)
            {
                // get the details from the GPIO expander based on the last interrupt
                uint8_t iPin = 255;
                iPin = expander->getLastInterruptPin();
                if (iPin != 255)
                {
                    // retrieve all the pin states as of the time of the last interrupt
                    allPins = expander->getCapturedInterrupt();

                    // clear the interrupt, enabling the expander chip to raise a new interrupt
                    expander->clearInterrupts();
                }

                // check if this is a button press (versus a release)
                if (iPin != 255 && GPIOEXPANDERBUTTONS_PIN_PRESSED(allPins, iPin))
                {
                    GpioExpanderEvent event;
                    event.expander = expander;
                    event.pinNumber = iPin;

                    // send a button press to the queue
                    //xQueueSend( xGpioExpanderButtonEventQueue, &event, portMAX_DELAY);
                }
            }
#if GPIOEXPANDERBUTTONS_FLASH_BUILTIN_LED == TRUE
            // clear the LED flash in debug mode
            digitalWrite(LED_BUILTIN, LOW);
#endif
        }
    }
}


#endif // GPIOEXPANDERBUTTONS_H