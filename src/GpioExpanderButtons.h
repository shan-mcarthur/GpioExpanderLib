#ifndef GPIOEXPANDERBUTTONS_H
#define GPIOEXPANDERBUTTONS_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>

// a couple of handy macros to deal with a single integer containing the buttons as bits (0-15)
#define GPIOEXPANDERBUTTONS_PIN(i) (1<<i)
#define GPIOEXPANDERBUTTONS_PIN_PRESSED(var,pos) (!((var) & (1<<(pos))))
#define GPIOEXPANDERBUTTONS_MAX_EXPANDERS 8

// task notification and queue for interrupt and background processing
static QueueHandle_t xGpioExpanderButtonEventQueue;
static TaskHandle_t xGpioExpanderButtonsTaskToNotify;

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
        uint8_t _interruptPin;
        uint16_t getCapturedInterrupt();
        uint8_t getLastInterruptPin();
        void clearInterrupts();
};

// global dictionary of registered expanders so that event handler can look up which one raised the interrupt
static GpioExpanderButtons *GlobalGpioExpanderButtons[GPIOEXPANDERBUTTONS_MAX_EXPANDERS] = {};

// Button press event details
struct GpioExpanderButtonsEvent
{
    GpioExpanderButtons *expander;
    uint8_t pinNumber;
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
            GpioExpanderButtons *expander = nullptr;
            uint8_t expanderNumber = 255;

            // loop through the array until you find one that has a pending interrupt
            for (int i=0; i<GPIOEXPANDERBUTTONS_MAX_EXPANDERS && !bDone; i++)
            {
                expander = GlobalGpioExpanderButtons[i];
                if (expander != nullptr)
                {
                    // check to see if the interrupt line is low
                    if (digitalRead(expander->_interruptPin) == LOW)
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
                    GpioExpanderButtonsEvent event;
                    event.expander = expander;
                    event.pinNumber = iPin;

                    // send a button press to the queue
                    xQueueSend( xGpioExpanderButtonEventQueue, &event, portMAX_DELAY);
                }
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
    _interruptPin = mcu_interrupt_pin;

    // we have to maintain a global list of expanders as there is only one interrupt routine and task notification
    // add this instance to the global list (so that ISRs can find pins and interrogate chips)
    bool bDone = false;
    bool isFirstOne = false;

    for (int i=0; i<GPIOEXPANDERBUTTONS_MAX_EXPANDERS && !bDone; i++)
    {
        // check if we are at the end of the array yet (nullptr)
        if (GlobalGpioExpanderButtons[i] == nullptr)
        {
            if (i == 0)
            {
                // this is the first one added.  Remember this for later
                isFirstOne = true;
            }

            // add this to the list
            GlobalGpioExpanderButtons[i] = this;
            bDone = true;
        }
    }

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

    // we only need one background task handling the notifications from the interrupt
    if (isFirstOne)
    {
        // initiate background task to handle the button presses
        xTaskCreate(
                GpioExpanderButtons::GpioExpanderButtonServiceTask,  // Function to be called
                "Service Button Events",   // Name of task
                3000,         // Stack size (bytes in ESP32, words in FreeRTOS)
                NULL,         // Parameter to pass to function
                1,            // Task priority (0 to configMAX_PRIORITIES - 1)
                &xGpioExpanderButtonsTaskToNotify);         // Task handle

        // initialize a queue to use for the button press events
        xGpioExpanderButtonEventQueue = xQueueCreate(50, sizeof(GpioExpanderButtonsEvent));
    }
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