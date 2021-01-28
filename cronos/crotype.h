#ifndef __CROTYPE_H
#define __CROTYPE_H

#include <stdint.h>

typedef uint64_t cronos_off;
typedef uint64_t cronos_size;

typedef uint32_t cronos_id;
typedef uint32_t cronos_idx;

#define INVALID_CRONOS_ID 0

typedef enum {
    CRONOS_TAD,
    CRONOS_DAT
} cronos_filetype;

#endif
