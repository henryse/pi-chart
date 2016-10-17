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

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include "pi_am2315.h"
#include "pi_utils.h"

// Checking to see if we are on a Raspberry PI
//
#ifndef BCMHOST
#define ENABLE_PI_EMULATOR 1
#endif

#ifndef ENABLE_PI_EMULATOR

#include <wiringPi.h>
#include <wiringPiI2C.h>

#endif

#define AM2315_I2CADDR       0x5C
#define AM2315_READREG       0x03

float pi_am2315_compute_humidity(unsigned char msb, unsigned char lsb) {
    int humidity_h, humidity_l;
    humidity_h = msb << 8;
    humidity_l = lsb;
    return (float) ((humidity_h + humidity_l) / 10.0);
}

float pi_am2315_compute_temperature(unsigned char msb, unsigned char lsb) {
    int temperature_h, temperature_l;
    float tmp;

    temperature_h = msb & 0x7F; // ignore first bit
    temperature_l = lsb;
    tmp = (temperature_h << 8) + temperature_l;
    tmp /= 10.0;

    if(msb & 0x80) // check if negative
        tmp *= -1;

    return tmp;
}


uint16_t pi_am2315_crc16(unsigned char *ptr, unsigned char len) {
    unsigned short crc = 0xFFFF;
    unsigned char i;

    while(len--) {
        crc ^= *ptr++;
        for(i = 0; i < 8; i++) {
            if(crc & 0x01) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }

    }
    return crc;
}


int pi_am2315_open(){
#ifdef ENABLE_PI_EMULATOR
    return 1;
#else
    return wiringPiI2CSetup (AM2315_I2CADDR);
#endif
}

// The AM2315 manual advisess that continuous samples must be at least 2 seconds apart).
// Calling this method avoids the double I2C request.
//

bool pi_am2315_readTemperatureAndHumidity(int fd, float *temperature, float *humidity){
#ifdef ENABLE_PI_EMULATOR

    sleep(3);
    printf("write returned %d bytes\n",4);
    sleep(1);
    *temperature = 20.0;
    *humidity = 10.0;
    uint16_t crc = 0, crc_res = 0;

#else

    sleep(2);

    // read request - 3 is the read register command
    // 0 is the address to start at
    // 4 is the number of bytes to read
    //
    unsigned char read_request[3] = {3, 0, 4};

    // buffer for the response: command byte, length byte, 4 bytes data, 2 bytes of checksum
    //
    unsigned char response[8];
    memory_clear(response, sizeof(response));

    // dummy data sent to wake up device
    //
    unsigned char dummy[1] = {0};

    // send some data to wake it up
    //
    int n = write(fd, dummy, 1);
    n = write(fd, dummy, 1);

    // just sleep a bit
    //
    sleep(1);

    // send the read request
    //
    n = write(fd, read_request, AM2315_READREG);

    printf("write returned %d bytes\n",n);

    // very short delay to allow device to do data conversion
    //
    sleep(1);

    // read the response
    n = read(fd, response, 8);
    printf("read returned %d bytes\n",n);

    // compute humidity value
	*humidity = pi_am2315_compute_humidity(response[2], response[3]);

	// compute temperature value
	*temperature = pi_am2315_compute_temperature(response[4], response[5]);

 	// compute crc
	uint16_t crc_res = pi_am2315_crc16(response, 6);
	uint16_t crc = (response[7] << 8) + response[6];

#endif

    return crc_res == crc;
}