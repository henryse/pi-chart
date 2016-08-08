/**********************************************************************
//    Copyright (c) 2016 Henry Seurer & Samuel Kelly
//
//    Permission is hereby granted, free of charge, to any person
//    obtaining a copy of this software and associated documentation
//    files (the "Software"), to deal in the Software without
//    restriction, including without limitation the rights to use,
//    copy, modify, merge, publish, distribute, sublicense, and/or sell
//    copies of the Software, and to permit persons to whom the
//    Software is furnished to do so, subject to the following
//    conditions:
//
//    The above copyright notice and this permission notice shall be
//    included in all copies or substantial portions of the Software.
//
//    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
//    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
//    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
//    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
//    OTHER DEALINGS IN THE SOFTWARE.
//
**********************************************************************/

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef PI_CHART_PI_CHART_GPIO_H
#define PI_CHART_PI_CHART_GPIO_H

typedef enum {
    LOW_SIGNAL = 0,
    HIGH_SIGNAL = 1
} gpio_signal;

typedef enum {
    MODE_IN = 0,
    MODE_OUT,
    MODE_ALT5,
    MODE_ALT4,
    MODE_ALT0,
    MODE_ALT1,
    MODE_ALT2,
    MODE_ALT3,
}gpio_mode;

void setup_wiring_pi();

const char *gpio_get_digital_str(unsigned char pin);

gpio_signal gpio_get_digital(unsigned char pin);

gpio_signal gpio_set_digital(unsigned char pin, gpio_signal value);

const char *gpio_get_mode_str(unsigned char pin);

gpio_mode gpio_get_mode(unsigned char pin);

#endif //PI_CHART_PI_CHART_GPIO_H

#pragma clang diagnostic pop