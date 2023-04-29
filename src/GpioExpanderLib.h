#ifndef GPIOEXPANDERLIB_H
#define GPIOEXPANDERLIB_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>

// a couple of handy macros to deal with a single integer containing the buttons as bits (0-15)
#define GPIOEXPANDERBUTTONS_PIN(i) (1<<i)
#define GPIOEXPANDERBUTTONS_PIN_PRESSED(var,pos) (!((var) & (1<<(pos))))
#define GPIOEXPANDER_MAX_EXPANDERS 8

// task notification and queue for interrupt and background processing
static QueueHandle_t xGpioExpanderEventQueue;
static TaskHandle_t xGpioExpanderTaskToNotify;

struct GpioExpanderButton
{
    uint8_t pin;
};

struct GpioExpanderRotaryEncoder
{
    uint8_t pin1;
    uint8_t pin2;
};

class GpioExpander
{
    private:
        static void IRAM_ATTR GpioExpanderInterrupt();
        uint8_t _interruptPin;
        uint8_t _maxButtons;
        uint8_t _maxRotaryEncoders;
        GpioExpanderButton* _buttons;
        GpioExpanderRotaryEncoder* _rotaryEncoders;

    public: 
        GpioExpander(uint8_t maxButtons=16, uint8_t maxRotaryEncoders=8);
        void Init(Adafruit_MCP23X17 *expander, uint8_t interruptPin);
        void AddButton(uint8_t pin);
        void AddRotaryEncoder (uint8_t pin1, uint8_t pin2);
        Adafruit_MCP23X17 *_expander;
        uint8_t GetMaxButtons() { return _maxButtons; }
        uint8_t GetInterruptPin() { return _interruptPin; }
        uint8_t GetMaxRotaryEncoders() { return _maxRotaryEncoders;}
        uint16_t getCapturedInterrupt();
        uint8_t getLastInterruptPin();
        void clearInterrupts();

};

// global dictionary of registered expanders so that event handler can look up which one raised the interrupt
static GpioExpander *GlobalGpioExpanders[GPIOEXPANDER_MAX_EXPANDERS] = {};

// Button press event details
struct GpioExpanderEvent
{
    GpioExpander *expander;
    uint8_t pinNumber;
};

// constructor
GpioExpander::GpioExpander(uint8_t maxButtons, uint8_t maxRotaryEncoders)
{
    _maxButtons = maxButtons;
    if (maxButtons > 0)
    {
        _buttons = new GpioExpanderButton[maxButtons];
    }

    _maxRotaryEncoders = maxRotaryEncoders;
    if (maxRotaryEncoders > 0)
    {
        _rotaryEncoders = new GpioExpanderRotaryEncoder[maxRotaryEncoders];
    }
}

// Hardware Interrupt Service Routine (ISR) for handling button interrupts
void IRAM_ATTR GpioExpander::GpioExpanderInterrupt() 
{
    // in ISR must be fast and cannot reach out to a sensor over the wire, so notify a lower priority task that an interrupt has occured
    // notify the button handler that a hardware interrupt has occured
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(xGpioExpanderTaskToNotify, &xHigherPriorityTaskWoken);

    // yield processor to other tasks
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


void GpioExpander::Init(Adafruit_MCP23X17 *expander, uint8_t interruptPin)
{
    // transfer parameters to private members of the class
    _expander = expander;
    _interruptPin = interruptPin;

    // we have to maintain a global list of expanders as there is only one interrupt routine and task notification
    // add this instance to the global list (so that ISRs can find pins and interrogate chips)
    bool bDone = false;
    bool isFirstOne = false;

    for (int i=0; i<GPIOEXPANDER_MAX_EXPANDERS && !bDone; i++)
    {
        // check if we are at the end of the array yet (nullptr)
        if (GlobalGpioExpanders[i] == nullptr)
        {
            if (i == 0)
            {
                // this is the first one added.  Remember this for later
                isFirstOne = true;
            }

            // add this to the list
            GlobalGpioExpanders[i] = this;
            bDone = true;
        }
    }

    // set up the expander module for interrupts
    _expander->setupInterrupts(true, false, LOW);

    // clear any pending interrupts
    _expander->clearInterrupts();

    // configure MCU pin that will receive the interrupt from the GPIO expander
    pinMode(_interruptPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(_interruptPin), GpioExpander::GpioExpanderInterrupt, FALLING);
}

// access the interrupt details from the event handler task
uint16_t GpioExpander::getCapturedInterrupt()
{
    return _expander->getCapturedInterrupt();
}

// access the pin details of the interrupt from the event handler task
uint8_t GpioExpander::getLastInterruptPin()
{
    return _expander->getLastInterruptPin();
}

// clear the interrupts from within the init and event handler task
void GpioExpander::clearInterrupts()
{
    _expander->clearInterrupts();
}
#endif  // GPIOEXPANDERLIB_H
#include "GpioExpanderButtons.h"
