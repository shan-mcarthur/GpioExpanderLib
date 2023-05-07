#ifndef GPIOEXPANDERBUTTONTYPES_H
#define GPIOEXPANDERBUTTONTYPES_H

struct GpioExpanderButton
{
    bool isUsed = false;
    uint8_t pin;
    uint8_t mode = LOW;  //CHANGE, LOW, or HIGH
    uint8_t lastState;
    unsigned long lastStateChange;
};

#endif //GPIOEXPANDERBUTTONTYPES_H