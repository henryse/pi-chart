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
#include <ctype.h>
#include "pi_template_generator.h"
#include "pi_utils.h"
#include "pi_intmap.h"
#define if_stack_depth 32

typedef struct pi_template_generator_struct {

    void *context_ptr;
    function_string_ptr_t function_string_ptr;
    function_boolean_ptr_t function_boolean_ptr;

    pi_intmap_ptr symbol_map;

    pi_string_ptr output_buffer;

    int if_stack_top;
    bool if_stack[if_stack_depth];
} pi_template_generator_t;


typedef enum {
    operator_type_invalid = 0,
    operator_type_variable,
    operator_type_If,
    operator_type_Else,
    operator_type_EndIf,
    operator_type_output,
} operator_type_t;

#define OPERATOR_SYMBOL(name) operator_type_##name

void pi_template_generator_create(pi_template_generator_t *ptg_context,
                                  pi_string_ptr output_buffer,
                                  void *context_ptr,
                                  function_boolean_ptr_t function_boolean_ptr,
                                  function_string_ptr_t function_string_ptr) {

    memory_clear(ptg_context, sizeof(pi_template_generator_t));

    ptg_context->output_buffer = output_buffer;

    // These need to be in sync with operator_type enum
    //
    ptg_context->symbol_map = pi_intmap_new(64);
    pi_intmap_put(ptg_context->symbol_map, "If", OPERATOR_SYMBOL(If));
    pi_intmap_put(ptg_context->symbol_map, "Else", OPERATOR_SYMBOL(Else));
    pi_intmap_put(ptg_context->symbol_map, "EndIf", OPERATOR_SYMBOL(EndIf));
    pi_intmap_put(ptg_context->symbol_map, "=", OPERATOR_SYMBOL(output));

    ptg_context->context_ptr = context_ptr;
    ptg_context->function_string_ptr = function_string_ptr;
    ptg_context->function_boolean_ptr = function_boolean_ptr;
}

void pi_template_generator_destroy(pi_template_generator_t *ptg_context) {

    pi_intmap_delete(ptg_context->symbol_map);
}

static const char *pi_template_skip_white(const char *pch, const char chDelim) {
    while (isspace(*pch) || chDelim == *pch) {
        pch++;
    }

    return pch;
}

pi_string_ptr pi_template_get_symbol(const char *begin_tag, char **end_tag) {
    ASSERT(NULL != end_tag);

    if (NULL == end_tag) {
        return NULL;
    }

    // Skip white space at start of the string.
    //
    begin_tag = pi_template_skip_white(begin_tag, ' ');

    // OK now from here on out lets determine if we see a "symbol" +-% etc or are we
    // looking an alpha numeric value?
    //
    *end_tag = (char *) begin_tag;

    // Check to see if we see symbol
    //
    if (!isalnum(**end_tag)){
        // Huh, it must be something special, we only take symbols one at a time.
        //
        (*end_tag)++;
    }
    else {
        // We are good to go either this is a number of a symbol.
        while (!isspace(**end_tag) && '%' != **end_tag && **end_tag) {
            (*end_tag)++;
        }
    }

    pi_string_ptr symbol = pi_string_new(*end_tag - begin_tag);
    pi_string_append_str_length(symbol, begin_tag, *end_tag - begin_tag);

    return symbol;
}

bool pi_template_resolve_symbol(pi_template_generator_t *ptg_context,
                                pi_string_ptr symbol_buffer,
                                pi_string_ptr result_buffer) {

    operator_type_t operator_type = (operator_type_t) pi_intmap_get_value(ptg_context->symbol_map,
                                                                          pi_string_c_string(symbol_buffer));

    bool valid = false;

    if ( operator_type == operator_type_invalid || operator_type == operator_type_variable) {

        if (result_buffer) {
            pi_string_reset(result_buffer);
        }

        valid = (*ptg_context->function_string_ptr)(ptg_context->context_ptr,
                                                         pi_string_c_string(symbol_buffer),
                                                         result_buffer);

        if (valid && operator_type == operator_type_invalid) {
            pi_intmap_put(ptg_context->symbol_map, pi_string_c_string(symbol_buffer), operator_type_variable);
        }

    }

    return valid;
}

bool pi_template_test_symbol(pi_template_generator_t *ptg_context,
                             pi_string_ptr symbol_buffer,
                             bool *value) {

    operator_type_t operator_type = (operator_type_t) pi_intmap_get_value(ptg_context->symbol_map,
                                                                          pi_string_c_string(symbol_buffer));

    bool valid = false;

    if ( operator_type == operator_type_invalid || operator_type == operator_type_variable) {

        if (value) {
            *value = false;
        }

        valid = (*ptg_context->function_boolean_ptr)(ptg_context->context_ptr,
                                                          pi_string_c_string(symbol_buffer),
                                                          value);

        if (valid && operator_type == operator_type_invalid) {
            pi_intmap_put(ptg_context->symbol_map, pi_string_c_string(symbol_buffer), operator_type_variable);
        }

    }

    return valid;
}

operator_type_t pi_template_lookup_symbol(pi_template_generator_t *ptg_context,
                                          const char *begin_tag,
                                          pi_string_ptr result_buffer,
                                          bool *response_result) {
    ASSERT(result_buffer != NULL && response_result != NULL);
    ASSERT(*begin_tag == '<' && *(begin_tag + 1) == '%');

    if (result_buffer) {
        pi_string_reset(result_buffer);
    }

    if (response_result) {
        *response_result = false;
    }

    operator_type_t operator_type = operator_type_invalid;

    if (*begin_tag == '<' && *(begin_tag + 1) == '%') {
        //  Move past the '<%' tag marker and any leading white spaces.
        //
        char *end_tag = NULL;
        pi_string_ptr first_symbol = pi_template_get_symbol(begin_tag + 2, &end_tag);

        operator_type = (operator_type_t) pi_intmap_get_value(ptg_context->symbol_map,
                                                              pi_string_c_string(first_symbol));

        pi_string_ptr second_symbol = NULL;

        switch (operator_type) {
            case operator_type_If:
                second_symbol = pi_template_get_symbol(end_tag, &end_tag);
                if (!pi_template_test_symbol(ptg_context, second_symbol, response_result)) {
                    operator_type = operator_type_invalid;
                }
                break;

            case operator_type_Else:
            case operator_type_EndIf:
                break;

            case operator_type_output:
                second_symbol = pi_template_get_symbol(end_tag, &end_tag);
                if (!pi_template_resolve_symbol(ptg_context, second_symbol, result_buffer)) {
                    operator_type = operator_type_invalid;
                }
                break;

            case operator_type_variable:
                if (!pi_template_resolve_symbol(ptg_context, first_symbol, result_buffer)) {
                    operator_type = operator_type_invalid;
                }
                break;

            case operator_type_invalid:
            default:
                ERROR_LOG("Unknown symbol or operator: %s", pi_string_c_string(first_symbol));
                break;

        }

        pi_string_delete(first_symbol, true);
        pi_string_delete(second_symbol, true);
    }

    return operator_type;
}

void pi_template_push_if(pi_template_generator_t *ptg_context, bool value) {
    if (ptg_context->if_stack_top < if_stack_depth) {
        ptg_context->if_stack[ptg_context->if_stack_top] = value;
    }
    ptg_context->if_stack_top++;
}

void pi_template_push_else(pi_template_generator_t *ptg_context) {
    if (ptg_context->if_stack_top > 0 && ptg_context->if_stack_top < if_stack_depth) {
        ptg_context->if_stack[ptg_context->if_stack_top - 1] =
                !ptg_context->if_stack[ptg_context->if_stack_top - 1];
    }
}

void pi_template_pop_if(pi_template_generator_t *ptg_context) {
    if (ptg_context->if_stack_top > 0) {
        ptg_context->if_stack_top--;
    }
}

bool pi_template_if_set(pi_template_generator_t *ptg_context) {
    for (int i = 0; i < ptg_context->if_stack_top; i++) {
        if (!ptg_context->if_stack[i]) {
            return false;
        }
    }
    return true;
}

void pi_template_output(pi_template_generator_t *ptg_context, const char *ptr_in) {
    if (pi_template_if_set(ptg_context)) {
        pi_string_append_str(ptg_context->output_buffer, ptr_in);
    }
}

void pi_template_output_length(pi_template_generator_t *ptg_context, const char *ptr_in, size_t length) {
    if (pi_template_if_set(ptg_context)) {
        pi_string_append_str_length(ptg_context->output_buffer, ptr_in, length);
    }
}

pi_template_error_t pi_template_generate_output(pi_string_ptr input_buffer,
                                                pi_string_ptr output_buffer,
                                                void *context_ptr,
                                                function_string_ptr_t function_string_ptr,
                                                function_boolean_ptr_t function_boolean_ptr) {
    if (NULL == function_string_ptr
        || NULL == function_boolean_ptr
        || NULL == output_buffer
        || NULL == input_buffer) {
        return pie_template_invalid_input;
    }

    pi_string_reset(output_buffer);

    pi_template_generator_t ptg_context;

    pi_template_generator_create(&ptg_context,
                                 output_buffer,
                                 context_ptr,
                                 function_boolean_ptr,
                                 function_string_ptr);

    const char *ptr_in = pi_string_c_string(input_buffer);
    const char *ptr_EOF = ptr_in + pi_string_c_string_length(input_buffer);

    if (*ptr_in == '%') {
        pi_string_append_char(output_buffer, *ptr_in);
        ptr_in++;
    }

    pi_string_ptr symbol_buffer = pi_string_new(64);

    while (ptr_in < ptr_EOF) {
        //  Look for '%' and then see if we have a "<%"
        //  Initialization would have ensured that we wont crash...
        //
        const char *ptr_tag = strchr(ptr_in, '%');
        if (!ptr_tag) {
            //  No more tags, copy the rest of the data to the output
            //  buffer and break the loop.
            //
            pi_template_output(&ptg_context, ptr_in);
            break;
        }

        //  Set the ptr_tag to the beginning of the '<%' tag...
        ptr_tag--;

        size_t length_to_append = ptr_tag - ptr_in + 2 * (*ptr_tag != '<');

        pi_template_output_length(&ptg_context, ptr_in, length_to_append);

        ptr_in += length_to_append;
        if (*ptr_tag != '<') {
            continue;
        }

        bool bool_value = false;
        operator_type_t operator_type = pi_template_lookup_symbol(&ptg_context, ptr_in, symbol_buffer,
                                                                  &bool_value);
        switch (operator_type) {
            case operator_type_invalid:
            default:
                break;
            case operator_type_variable:
            case operator_type_output:
                pi_template_output(&ptg_context, pi_string_c_string(symbol_buffer));
                break;

            case operator_type_If:
                pi_template_push_if(&ptg_context, bool_value);
                break;

            case operator_type_Else:
                pi_template_push_else(&ptg_context);
                break;

            case operator_type_EndIf:
                pi_template_pop_if(&ptg_context);
                break;
        }

        // Skip over opening <%
        //
        if (*ptr_in == '<' && *(ptr_in + 1) == '%') {
            ptr_in += 2;
        }

        // Skip to closing %>
        //
        while ('%' != *ptr_in && *ptr_in) {
            ptr_in++;
        }

        if (*ptr_in == '%' && *(ptr_in + 1) == '>') {
            ptr_in += 2;
        }
    }
    pi_string_delete(symbol_buffer, true);

    pi_template_generator_destroy(&ptg_context);
    return pie_template_no_error;
}