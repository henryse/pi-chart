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

#include <memory.h>
#include <stdlib.h>
#include "pi_mem_info.h"
#include "stdio.h"
#include "pi_utils.h"

bool pi_mem_info_get_attribute(pi_string_ptr output_string, const char *attribute) {

    // TODO(CHART-2): need to return the actual Value.  The values can be found in
    // cat /proc/meminfo on linux and we need to find the same for the emulator.
    // for the emulator we should use the local /proc/meminfo file

    #ifdef __MACH__
        // Mac OS Emulator code


    FILE *fp = fopen("./proc/meminfo", "r");

    if(fp == 0) {
        ERROR_LOG("Value not found: %s", attribute);
        return false;
    }

    char buffer[255];
    memory_clear(buffer, sizeof(buffer));

    char *ptr_to_token = NULL;

    while(*fgets(buffer, 255, fp) != EOF){
        ptr_to_token = strtok(buffer, ": ");
        if(strcmp(ptr_to_token,attribute) == 0) {
            fclose(fp);
            pi_string_append_str(output_string, strtok(NULL, ": "));
            return true;
        }
        memory_clear(buffer, sizeof(buffer));
    }

    fclose(fp);

    #else
        // Linux
    #endif

    return false;
}