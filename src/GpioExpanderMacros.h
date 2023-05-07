#ifndef GPIOEXPANDERMACROS_H
#define GPIOEXPANDERMACROS_H

// a couple of handy macros to deal with a single integer containing the buttons as bits (0-15)
#define GPIOEXPANDERBUTTONS_PIN(i) (1<<i)
#define GPIOEXPANDERBUTTONS_PIN_PRESSED(var,pos) (!((var) & (1<<(pos))))
#define GPIOEXPANDERBUTTONS_PIN_STATE(var,pos) (((var) & (1<<(pos)))!=0?HIGH:LOW)

#endif