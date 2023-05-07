#ifndef GPIOEXPANDERROTARYENCODERTYPES_H
#define GPIOEXPANDERROTARYENCODERTYPES_H

enum GpioExpanderRotaryEncoderEventEnum {Still, Clockwise, CounterClockwise};

struct GpioExpanderRotaryEncoder
{
    bool isUsed = false;
    uint8_t pin1 = 255;
    uint8_t pin2 = 255;
    bool fullCycleBetweenDetents = false;
    unsigned long debounceMs = 0;
    unsigned long lastMovementMs = 0;
    uint8_t pin1State = 255;
    uint8_t pin2State = 255;
    uint8_t index = 255;
    GpioExpanderRotaryEncoderEventEnum lastMovement = Still;
};

#endif //GPIOEXPANDERROTARYENCODERTYPES_H