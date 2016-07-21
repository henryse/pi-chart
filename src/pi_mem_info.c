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
#include "pi_mem_info.h"
#include "stdio.h"
#include "pi_utils.h"

const char *pi_mem_info_get_file_name(){
#ifdef __MACH__
    // Mac OS Emulator code
    //
    return "./proc/meminfo";
#else
    // Debian meminfo file:
    //
    return "/proc/meminfo";
#endif
}

bool pi_mem_info_get_attribute(pi_string_ptr output_string, const char *symbol) {

    FILE *file_ptr = fopen(pi_mem_info_get_file_name(), "r");
    bool found_value = false;

    if(file_ptr != NULL) {
        const size_t buffer_size = 256;
        char buffer[buffer_size];
        memory_clear(buffer, sizeof(buffer));

        char *ptr_to_token = NULL;

        while(*fgets(buffer, (buffer_size-1), file_ptr) != EOF){
            ptr_to_token = strtok(buffer, ": ");
            if(strcmp(ptr_to_token, symbol) == 0) {
                pi_string_append_str(output_string, strtok(NULL, ": "));
                found_value = true;
                break;
            }

            memory_clear(buffer, sizeof(buffer));
        }
    }
    else {
        ERROR_LOG("meminfo not found: %s", pi_mem_info_get_file_name());
    }

    fclose(file_ptr);

    return found_value;
}