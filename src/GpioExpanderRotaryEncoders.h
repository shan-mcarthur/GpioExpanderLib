#ifndef GPIOEXPANDERROTARYENCODERS_H
#define GPIOEXPANDERROTARYENCODERS_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>

#define GPIOEXPANDER_MAX_EXPANDERS 8

static TaskHandle_t xGpioExpanderRotaryTaskToNotify;

class GpioExpanderRotaryEncoder
{
    public: 
        GpioExpanderRotaryEncoder();
        void Init(Adafruit_MCP23X17 *expander, uint8_t mcu_interrupt_pin);

    private:
        static void IRAM_ATTR GpioExpanderRotaryEncoderInterrupt();
        static void GpioExpanderRotaryEncoderServiceTask(void *parameter);         
        Adafruit_MCP23X17 *_expander;
        uint16_t getCapturedInterrupt();
        uint8_t getLastInterruptPin();
        void clearInterrupts();
};

#endif