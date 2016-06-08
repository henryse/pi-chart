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

#ifndef PI_TEMPLATE_GENERATOR_H
#define PI_TEMPLATE_GENERATOR_H

#include "pi_string.h"

typedef enum {
    pie_template_no_error = 0,
    pie_template_invalid_input,
} pi_template_error_t;


typedef bool ( *function_string_ptr_t )(void *context_ptr,
                                        const char *symbol,
                                        pi_string_ptr value);

typedef bool ( *function_boolean_ptr_t )(void *context_ptr,
                                         const char *symbol,
                                         bool *value);

pi_template_error_t pi_template_generate_output(pi_string_ptr input_buffer,
                                                pi_string_ptr output_buffer,
                                                void *context_ptr,
                                                function_string_ptr_t function_string_ptr,
                                                function_boolean_ptr_t function_boolean_ptr);

#endif //PI_TEMPLATE_GENERATOR_H
