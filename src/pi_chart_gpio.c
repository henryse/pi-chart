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
#pragma ide diagnostic ignored "UnusedImportStatement"
#include "pi_chart_gpio.h"
#include "pi_utils.h"
#include "version_config.h"

// Checking to see if we are on a Raspberry PI
//
#ifndef BCMHOST

#define ENABLE_PI_EMULATOR 1

#endif

#ifndef ENABLE_PI_EMULATOR

#include <wiringPi.h>

#else

#define gpio_pin_count 26
gpio_signal gpio_list[gpio_pin_count];

gpio_mode gpio_mode_list[gpio_pin_count];

#endif

static char *gpio_modes [] = {"IN", "OUT", "ALT5", "ALT4", "ALT0", "ALT1", "ALT2", "ALT3"} ;

void setup_wiring_pi() {

#ifndef ENABLE_PI_EMULATOR
    INFO_LOG("Setting up WiringPi");
    wiringPiSetup();
#else
    INFO_LOG("WARNING: Running in emulation mode");

    for (int i = 0; i < gpio_pin_count; i++) {
        gpio_list[i] = LOW_SIGNAL;
        gpio_mode_list[i] = MODE_IN;
    }
#endif
}

gpio_signal gpio_get_digital(unsigned char pin) {
    gpio_signal result = LOW_SIGNAL;

#ifndef ENABLE_PI_EMULATOR
    result = digitalRead((int) pin) == LOW ? LOW_SIGNAL : HIGH_SIGNAL;
#else
    if (pin < gpio_pin_count) {
        result = gpio_list[pin];
    }
#endif

    return result;
}

const char *gpio_get_digital_str(unsigned char pin) {
    return gpio_get_digital(pin) == LOW_SIGNAL ? "LOW" : "HIGH";
}

const char *gpio_get_mode_str(unsigned char pin) {
    return gpio_modes[gpio_get_mode(pin)];
}

gpio_signal gpio_set_digital(unsigned char pin, gpio_signal value) {
    gpio_signal result = LOW_SIGNAL;

#ifndef ENABLE_PI_EMULATOR
    result = digitalRead((int) pin);
    digitalWrite((int) pin, value == LOW_SIGNAL ? LOW : HIGH);
#else
    if (pin < gpio_pin_count) {
        result = gpio_list[pin];
        gpio_list[pin] = value;
    }
#endif

    return result;
}

gpio_mode gpio_get_mode(unsigned char pin) {
    gpio_mode result = MODE_IN;

#ifndef ENABLE_PI_EMULATOR
    result = getAlt(pin);
#else
    if (pin < gpio_pin_count) {
        result = gpio_mode_list[pin];
    }
#endif

    return result;
}

#pragma clang diagnostic pop