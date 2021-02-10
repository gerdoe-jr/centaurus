#ifndef __CROTYPE_H
#define __CROTYPE_H

#include <stdint.h>
#include <inttypes.h>
#include <utility>

typedef int cronos_version;
using cronos_abi_num = std::pair<cronos_version, cronos_version>;

#define cronos_abi_version(major, minor) std::make_pair(major, minor)

#define CRONOS_V3 3
#define CRONOS_V4 4

#define INVALID_CRONOS_VERSION (cronos_version)-1
#define INVALID_CRONOS_ABI (std::make_pair(INVALID_CRONOS_VERSION,   \
    INVALID_CRONOS_VERSION))

typedef uint64_t cronos_off;
typedef uint64_t cronos_size;
typedef uint32_t cronos_flags;

#define cronos_rel cronos_off
#define cronos_pos cronos_off

#define FCroOff PRIx64
#define FCroSize PRIu64
#define FCroFlags PRIu32

#define INVALID_CRONOS_OFFSET   (cronos_off)-1

typedef uint32_t cronos_id;
typedef uint32_t cronos_idx;

#define FCroId PRIu32
#define FCroIdx PRIu32

#define INVALID_CRONOS_ID 0

typedef enum {
    CRONOS_INVALID_FILETYPE,
    CRONOS_MEM = 0,
    CRONOS_TAD,
    CRONOS_DAT
} cronos_filetype;

#endif
