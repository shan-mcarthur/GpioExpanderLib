#ifndef GPIOEXPANDERLIB_H
#define GPIOEXPANDERLIB_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>

#define GPIOEXPANDER_MAX_EXPANDERS 8

// a couple of handy macros to deal with a single integer containing the buttons as bits (0-15)
#define GPIOEXPANDERBUTTONS_PIN(i) (1<<i)
#define GPIOEXPANDERBUTTONS_PIN_PRESSED(var,pos) (!((var) & (1<<(pos))))
#define GPIOEXPANDERBUTTONS_PIN_STATE(var,pos) (((var) & (1<<(pos)))!=0?HIGH:LOW)

// task notification and queue for interrupt and background processing
static QueueHandle_t xGpioExpanderEventQueue;
static TaskHandle_t xGpioExpanderTaskToNotify;

void print_binary (uint8_t n)
  {
    uint8_t i;
    for (i = 1 << 7; i > 0; i = i / 2)
    {
      if((n & i) != 0)
      {
        Serial.print("1");
      }
      else
      {
        Serial.print("0");
      }
    }
  }

struct GpioExpanderButton
{
    bool isUsed = false;
    uint8_t pin;
    uint8_t mode = LOW;  //CHANGE, LOW, or HIGH
    uint8_t lastState;
    unsigned long lastStateChange;
};

enum GpioExpanderRotaryEncoderEventEnum {Still, Clockwise, CounterClockwise};

struct GpioExpanderRotaryEncoder
{
    bool isUsed = false;
    uint8_t pin1 = 255;
    uint8_t pin2 = 255;
    bool fullCycleBetweenDetents = false;
    unsigned long debounceMs = 0;
    // unsigned long pin1TimeMs;
    // unsigned long pin2TimeMs;
    // unsigned long pin1TransitionTimeMs;
    // unsigned long pin2TransitionTimeMs;
    unsigned long lastMovementMs = 0;
    uint8_t pin1State = 255;
    uint8_t pin2State = 255;
    uint8_t index = 255;
    GpioExpanderRotaryEncoderEventEnum lastMovement = Still;
};

class GpioExpander
{
    private:
        static void IRAM_ATTR GpioExpanderInterrupt();
        static void GpioExpanderServiceTask(void *parameter);  
        uint8_t _interruptPin;
        uint8_t _maxButtons;
        uint8_t _maxRotaryEncoders;
        GpioExpanderButton* _buttons;
        GpioExpanderRotaryEncoder* _rotaryEncoders;

    public: 
        GpioExpander(uint8_t maxButtons=16, uint8_t maxRotaryEncoders=8);
        void Init(Adafruit_MCP23X17 *expander, uint8_t interruptPin);
        GpioExpanderButton* AddButton(uint8_t pin, uint8_t mode=LOW);
        GpioExpanderRotaryEncoder* AddRotaryEncoder (uint8_t pin1, uint8_t pin2, bool fullCycleBetweenDetents = false, unsigned long debounceMs = 200);
        Adafruit_MCP23X17 *_expander;
        uint8_t GetMaxPins() { return 16; } //maximum number of pins on this expander
        uint8_t GetMaxButtons() { return _maxButtons; }
        uint8_t GetInterruptPin() { return _interruptPin; }
        uint8_t GetMaxRotaryEncoders() { return _maxRotaryEncoders;}
        uint16_t getCapturedInterrupt();
        uint8_t getLastInterruptPin();
        uint8_t digitalRead(uint8_t pin);
        void clearInterrupts();
        GpioExpanderButton *GetButton(uint8_t index) { if  (index < GetMaxButtons()) {return &_buttons[index];}else{return (GpioExpanderButton *)nullptr;}};
        GpioExpanderRotaryEncoder *GetRotaryEncoder(uint8_t index) { if  (index < GetMaxRotaryEncoders()) {return &_rotaryEncoders[index];}else{return (GpioExpanderRotaryEncoder *)nullptr;}};
};

// global dictionary of registered expanders so that event handler can look up which one raised the interrupt
static GpioExpander *GlobalGpioExpanders[GPIOEXPANDER_MAX_EXPANDERS] = {};

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


// event details
struct GpioExpanderEvent
{
    GpioExpander *expander;
    uint8_t pinNumber;
    uint8_t state;
};

#include "GpioExpanderButtonHandler.h"
#include "GpioExpanderRotaryEncoderHandler.h"
void GpioExpander::GpioExpanderServiceTask(void *parameter) 
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
                    if (expander->digitalRead(expander->GetInterruptPin()) == LOW)
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
                uint8_t pin = 255;
                pin = expander->getLastInterruptPin();
                if (pin != 255)
                {
                    // retrieve all the pin states as of the time of the last interrupt
                    allPins = expander->getCapturedInterrupt();

                    // clear the interrupt, enabling the expander chip to raise a new interrupt
                    expander->clearInterrupts();
                }

                // find out which device is attached to this pin
                // check for the buttons
                for (uint8_t i=0; i<expander->GetMaxButtons(); i++)
                {
                    GpioExpanderButton *device = expander->GetButton(i);
                    if (device != nullptr && device->isUsed && device->pin == pin)
                    {
                        
                        GpioExpanderButtonHandler(expander, pin, device, GPIOEXPANDERBUTTONS_PIN_STATE(allPins, device->pin));
                        break;
                    }
                }

                // process all rotary encoders
                for (uint8_t i=0; i<expander->GetMaxRotaryEncoders(); i++)
                {
                    GpioExpanderRotaryEncoder *device = expander->GetRotaryEncoder(i);
                    if (device != nullptr && device->isUsed)
                    {
                        // Serial.print(millis());

                        // Serial.print(": device ");
                        // Serial.print(i);
                        // Serial.print(" ");

                        // Serial.print("p1 (");
                        // Serial.print(device->pin1);
                        // Serial.print (")=");
                        // Serial.print((allPins & 1 << device->pin1)>0?"1":"0");
                        // Serial.print(" p2 (");
                        // Serial.print(device->pin2);
                        // Serial.print (")=");
                        // Serial.print((allPins & 1 << device->pin2)>0?"1":"0");

                        // Serial.print("\tallPins=");
                        // Serial.print("    0b ");
                        // print_binary(allPins & 255);
                        // Serial.println("");


                        GpioExpanderRotaryEncoderHandler(expander, device, 
                                GPIOEXPANDERBUTTONS_PIN_STATE(allPins, device->pin1), 
                                GPIOEXPANDERBUTTONS_PIN_STATE(allPins, device->pin2));
                        
                    }
                }
                Serial.println();

            }
#if GPIOEXPANDERBUTTONS_FLASH_BUILTIN_LED == TRUE
            // clear the LED flash in debug mode
            digitalWrite(LED_BUILTIN, LOW);
#endif
        }
    }
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
    _expander->setupInterrupts(true, false, CHANGE);

    // configure each of the buttons for expander interrupts
    for (int i=0; i<GetMaxButtons(); i++)
    {
        // check if this pin is a button
        if (_buttons[i].isUsed)
        {
            // set the button pin mode to input with a pullup resistor, and enable interrupts on state change
            _expander->pinMode(_buttons[i].pin, INPUT_PULLUP);
            _expander->setupInterruptPin(_buttons[i].pin, CHANGE);
        }
    }

    // configure each of the rotary encoders for expander interrupts
    for (int i=0; i<GetMaxRotaryEncoders(); i++)
    {
        // check if this slot is used
        if (_rotaryEncoders[i].isUsed)
        {
            // set the pin mode to input with a pullup resistor, and enable interrupts on state change
            _expander->pinMode(_rotaryEncoders[i].pin1, INPUT_PULLUP);
            _expander->setupInterruptPin(_rotaryEncoders[i].pin1, CHANGE);
            _expander->pinMode(_rotaryEncoders[i].pin2, INPUT_PULLUP);
            _expander->setupInterruptPin(_rotaryEncoders[i].pin2, CHANGE);
        }
    }

    // clear any pending interrupts
    _expander->clearInterrupts();

    // configure MCU pin that will receive the interrupt from the GPIO expander
    pinMode(_interruptPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(_interruptPin), GpioExpander::GpioExpanderInterrupt, FALLING);

    // we only need one background task for all modules handling the notifications from the interrupt
    if (isFirstOne)
    {
        // initiate background task to handle the button presses and rotary events
        xTaskCreate(
                GpioExpander::GpioExpanderServiceTask,  // Function to be called
                "GpioExpander Module Events",   // Name of task
                3000,         // Stack size (bytes in ESP32, words in FreeRTOS)
                NULL,         // Parameter to pass to function
                1,            // Task priority (0 to configMAX_PRIORITIES - 1)
                &xGpioExpanderTaskToNotify);         // Task handle

        // initialize a queue to use for the button press events
        xGpioExpanderButtonEventQueue = xQueueCreate(50, sizeof(GpioExpanderButtonEvent));

        // initialize a queue to use for the rotary encoder events
        xGpioExpanderRotaryEncoderEventQueue = xQueueCreate(50, sizeof(GpioExpanderRotaryEncoderEvent));
    }
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

uint8_t GpioExpander::digitalRead(uint8_t pin)
{
    return _expander->digitalRead(pin);
}

GpioExpanderButton* GpioExpander::AddButton(uint8_t pin, uint8_t mode)
{
    // validate the mode
    if (mode != CHANGE && mode != LOW && mode!=HIGH)
    {
        return nullptr;
    }

    for (uint8_t i=0; i<GetMaxButtons(); i++)
    {
        if (_buttons[i].isUsed == false)
        {
            _buttons[i].pin = pin;
            _buttons[i].isUsed = true;
            _buttons[i].mode = mode;
            return &_buttons[i];
        }
        else if(_buttons[i].pin == pin)
        {
            // this is a duplicate
            return nullptr;
        }
    }
    return nullptr;
}

GpioExpanderRotaryEncoder* GpioExpander::AddRotaryEncoder (uint8_t pin1, uint8_t pin2, bool fullCycleBetweenDetents, unsigned long debounceMs)
{
    for (uint8_t i=0; i<GetMaxRotaryEncoders(); i++)
    {
        if (_rotaryEncoders[i].isUsed == false)
        {
            // this is an empty slot.  Initialize it
            _rotaryEncoders[i].pin1 = pin1;
            _rotaryEncoders[i].pin2 = pin2;
            _rotaryEncoders[i].isUsed = true;
            _rotaryEncoders[i].fullCycleBetweenDetents = fullCycleBetweenDetents;
            _rotaryEncoders[i].debounceMs = debounceMs;
            _rotaryEncoders[i].index = i;

            return &_rotaryEncoders[i];
        }
        else if(_rotaryEncoders[i].pin1 == pin1 || _rotaryEncoders[i].pin2 == pin1 
            || _rotaryEncoders[i].pin1 == pin2 || _rotaryEncoders[i].pin2 == pin2)
        {
            // this is a duplicate
            return nullptr;
        }
    }
    return nullptr;
}
#endif  // GPIOEXPANDERLIB_H
