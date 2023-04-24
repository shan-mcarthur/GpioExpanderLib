#ifndef GPIOEXPANDERBUTTONS_H
#define GPIOEXPANDERBUTTONS_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>

// a couple of handy macros to deal with a single integer containing the buttons as bits (0-15)
#define GPIOEXPANDERBUTTONS_PIN(i) (1<<i)
#define GPIOEXPANDERBUTTONS_PIN_PRESSED(var,pos) (!((var) & (1<<(pos))))

// task notification and queue for interrupt and background processing
static TaskHandle_t xGpioExpanderButtonsTaskToNotify;
static QueueHandle_t xGpioExpanderButtonEventQueue;

class GpioExpanderButtons
{
    public: 
        GpioExpanderButtons();
        void Init(Adafruit_MCP23X17 *expander, uint8_t mcu_interrupt_pin, uint16_t buttonPins, uint16_t debounceMs = 20);

    private:
        static void IRAM_ATTR GpioExpanderInterrupt();
        static void GpioExpanderButtonServiceTask(void *parameter);         
        uint16_t _debounceMs;
        Adafruit_MCP23X17 *_expander;
        uint16_t _buttonPins;
        uint16_t getCapturedInterrupt();
        uint8_t getLastInterruptPin();
        void clearInterrupts();
};

// constructor
GpioExpanderButtons::GpioExpanderButtons()
{
}

// Hardware Interrupt Service Routine (ISR) for handling button interrupts
void IRAM_ATTR GpioExpanderButtons::GpioExpanderInterrupt() 
{
    // in ISR must be fast and cannot reach out to a sensor over the wire, so notify a lower priority task that an interrupt has occured
    // notify the button handler that a hardware interrupt has occured
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(xGpioExpanderButtonsTaskToNotify, &xHigherPriorityTaskWoken);

    // yield processor to other tasks
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void GpioExpanderButtons::GpioExpanderButtonServiceTask(void *parameter) 
{
    uint16_t allPins;   //pin status while this interrupt occured
    static uint32_t thread_notification;
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 500 );
    uint32_t ulNotifiedValue;

    // the inbound parameter is the pointer to the current expander class
    GpioExpanderButtons *expander = (GpioExpanderButtons *)parameter;

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
                // send a button press to the queue
                xQueueSend( xGpioExpanderButtonEventQueue, &iPin, portMAX_DELAY);
            }

#if GPIOEXPANDERBUTTONS_FLASH_BUILTIN_LED == TRUE
            // clear the LED flash in debug mode
            digitalWrite(LED_BUILTIN, LOW);
#endif
        }
    }
}

void GpioExpanderButtons::Init (Adafruit_MCP23X17 *expander, uint8_t mcu_interrupt_pin, uint16_t buttonPins, uint16_t debounceMs)
{
    // transfer parameters to private members of the class
    _expander = expander;
    _buttonPins = buttonPins;
    _debounceMs = debounceMs;

    // set up the expander module for interrupts
    _expander->setupInterrupts(true, false, LOW);

    // configure each of the expander module pins
    for (int i=0; i<16; i++)
    {
        // check if this pin is a button
        if (buttonPins & (1 << i))
        {
            // set the button pin mode to input with a pullup resistor, and enable interrupts on state change
            _expander->pinMode(i, INPUT_PULLUP);
            _expander->setupInterruptPin(i, CHANGE);
        }
    }
    
    // clear any pending interrupts
    _expander->clearInterrupts();

    // configure MCU pin that will receive the interrupt from the GPIO expander
    pinMode(mcu_interrupt_pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(mcu_interrupt_pin), GpioExpanderButtons::GpioExpanderInterrupt, FALLING);

    // initiate background task to handle the button presses
    xTaskCreate(
              GpioExpanderButtons::GpioExpanderButtonServiceTask,  // Function to be called
              "Service Button Events",   // Name of task
              2048,         // Stack size (bytes in ESP32, words in FreeRTOS)
              (void *)this,         // Parameter to pass to function
              1,            // Task priority (0 to configMAX_PRIORITIES - 1)
              &xGpioExpanderButtonsTaskToNotify);         // Task handle

    // initialize a queue to use for the button press events
    xGpioExpanderButtonEventQueue = xQueueCreate(50, sizeof(uint8_t));
}

// private method to access the interrupt details from the event handler task
uint16_t GpioExpanderButtons::getCapturedInterrupt()
{
    return _expander->getCapturedInterrupt();
}

// private method to access the pin details of the interrupt from the event handler task
uint8_t GpioExpanderButtons::getLastInterruptPin()
{
    return _expander->getLastInterruptPin();
}

// private method to clear the interrupts from within the init and event handler task
void GpioExpanderButtons::clearInterrupts()
{
    _expander->clearInterrupts();
}

#endif // GPIOEXPANDERBUTTONS_H