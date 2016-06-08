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
#pragma ide diagnostic ignored "OCUnusedMacroInspection"

#define _GNU_SOURCE

#include <stdio.h>
#include "pi_intmap.h"
#include "pi_utils.h"

typedef struct pi_intmap_pair_struct{
    char *key;
    int  value_int;
} pi_intmap_pair_t;

typedef struct bucket_struct {
    unsigned int count;
    pi_intmap_pair_t *pairs;
} pi_intmap_bucket_t;

struct intmap_struct {
    unsigned int count;
    pi_intmap_bucket_t *buckets;
};

static pi_intmap_pair_t *pi_intmap_get_pair(pi_intmap_bucket_t *bucket, const char *key);

static unsigned long pi_intmap_hash(const char *str);

pi_intmap_t *pi_intmap_new(unsigned int capacity) {
    pi_intmap_t *map = memory_alloc(sizeof(pi_intmap_t));

    if (NULL == map) {
        return NULL;
    }

    map->count = capacity;
    map->buckets = memory_alloc(map->count * sizeof(pi_intmap_bucket_t));

    if (NULL == map->buckets) {
        memory_free(map);
        return NULL;
    }

    memset(map->buckets, 0, map->count * sizeof(pi_intmap_bucket_t));
    return map;
}

void pi_intmap_delete(pi_intmap_t *map) {
    if (NULL == map) {
        return;
    }
    unsigned int n = map->count;
    pi_intmap_bucket_t *bucket = map->buckets;
    unsigned int i = 0;
    while (i < n) {
        unsigned int m = bucket->count;
        pi_intmap_pair_t *pair = bucket->pairs;
        unsigned int j = 0;
        while (j < m) {
            memory_free(pair->key);
            pair++;
            j++;
        }
        memory_free(bucket->pairs);
        bucket++;
        i++;
    }
    memory_free(map->buckets);
    memory_free(map);
}

int pi_intmap_get_value(const pi_intmap_t *map, const char *key) {
    if (NULL == map || NULL == key) {
        return 0;
    }

    unsigned int index = (unsigned int) (pi_intmap_hash(key) % map->count);
    pi_intmap_bucket_t *bucket = &(map->buckets[index]);
    pi_intmap_pair_t *pair = pi_intmap_get_pair(bucket, key);
    if (NULL == pair) {
        return 0;
    }

    return pair->value_int;
}

bool pi_intmap_exists(const pi_intmap_t *map, const char *key) {
    if (NULL == map || NULL == key) {
        return false;
    }

    unsigned int index = (unsigned int) (pi_intmap_hash(key) % map->count);
    pi_intmap_bucket_t *bucket = &(map->buckets[index]);
    pi_intmap_pair_t *pair = pi_intmap_get_pair(bucket, key);

    if (NULL == pair) {
        return false;
    }

    return true;
}

bool pi_intmap_put(pi_intmap_t *map, const char *key, int value) {
    if (NULL == map || NULL == key) {
        return false;
    }

    size_t key_len = strlen(key);

    // Get a pointer to the bucket the key string hashes to
    //
    size_t index = pi_intmap_hash(key) % map->count;
    pi_intmap_bucket_t *bucket = &(map->buckets[index]);

    // Check if we can handle insertion by simply replacing
    // an existing value in a key-value pair in the bucket.
    //
    pi_intmap_pair_t *pair = NULL;
    if (NULL != (pair = pi_intmap_get_pair(bucket, key))) {
        /* Copy the new value into the pair that matches the key */
        pair->value_int = value;
        return true;
    }

    // Allocate space for a new key and value
    //
    char *new_key = memory_alloc((key_len + 1) * sizeof(char));
    if (NULL == new_key) {
        return false;
    }

    // Create a key-value pair
    //
    if (bucket->count == 0) {
        // The bucket is empty, lazily allocate space for a single
        // key-value pair.
        //
        bucket->pairs = memory_alloc(sizeof(pi_intmap_pair_t));
        if (NULL == bucket->pairs) {
            memory_free(new_key);
            return false;
        }
        bucket->count = 1;
    }
    else {
        // The bucket wasn't empty but no pair existed that matches the provided
        // key, so create a new key-value pair.
        //
        pi_intmap_pair_t *tmp_pairs = memory_realloc(bucket->pairs, (bucket->count + 1) * sizeof(pi_intmap_pair_t));
        if (NULL == tmp_pairs) {
            memory_free(new_key);
            return false;
        }
        bucket->pairs = tmp_pairs;
        bucket->count++;
    }
    // Get the last pair in the chain for the bucket
    //
    pair = &(bucket->pairs[bucket->count - 1]);
    pair->key = new_key;
    pair->value_int = value;

    // Copy the key and its value into the key-value pair
    //
    strcpy(pair->key, key);
    pair->value_int = value;
    return true;
}

int pi_intmap_get_count(const pi_intmap_t *map) {
    if (NULL == map) {
        return 0;
    }

    pi_intmap_bucket_t *bucket = map->buckets;
    unsigned int n = map->count;
    unsigned int i = 0;
    unsigned int count = 0;

    while (i < n) {
        pi_intmap_pair_t *pair = bucket->pairs;
        unsigned int m = bucket->count;
        unsigned int j = 0;
        while (j < m) {
            count++;
            pair++;
            j++;
        }
        bucket++;
        i++;
    }
    return count;
}

bool pi_intmap_enum(const pi_intmap_t *map, pi_intmap_enum_func enum_func, const void *obj) {
    if (NULL == map) {
        return false;
    }

    if (NULL == enum_func) {
        return true;
    }
    pi_intmap_bucket_t *bucket = map->buckets;
    unsigned int n = map->count;
    unsigned int i = 0;
    while (i < n) {
        pi_intmap_pair_t *pair = bucket->pairs;
        unsigned int m = bucket->count;
        unsigned int j = 0;
        while (j < m) {
            if (!enum_func(pair->key, pair->value_int, obj)) {
                goto exit;
            }
            pair++;
            j++;
        }
        bucket++;
        i++;
    }
    exit:;
    return true;
}

// Returns a pair from the bucket that matches the provided key,
// or null if no such pair exist.
//
static pi_intmap_pair_t *pi_intmap_get_pair(pi_intmap_bucket_t *bucket, const char *key) {
    unsigned int n = bucket->count;
    if (n == 0) {
        return NULL;
    }
    pi_intmap_pair_t *pair = bucket->pairs;
    unsigned int i = 0;
    while (i < n) {
        if (NULL != pair->key) {
            if (strcmp(pair->key, key) == 0) {
                return pair;
            }
        }
        pair++;
        i++;
    }

    return NULL;
}

// Returns a hash code for the provided string.
//
static unsigned long pi_intmap_hash(const char *str) {
    unsigned long hash = 5381;
    int c = 0;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}
#pragma clang diagnostic pop