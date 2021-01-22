#ifndef __CRONOS_H
#define __CRONOS_H

#include <stdint.h>

#pragma pack(push,1)

// OF OF OF SZ
// SZ SZ SZ SZ
// FL FL FL FL

#define TAD_V3_BASE				0x0C

typedef struct {
	uint32_t offset;
	uint32_t size;
	uint32_t flags;
} tad_v3_t;

#define	TAD_V4_BASE				0x10
#define TAD_V4_RZ(off) 			(off	>>	56)
#define TAD_V4_RZ_FLAG_DELETED 	(1		<<	1)
#define TAD_V4_OFFSET(off)		(off	&	0xFFFFFFFFFFFFFFF)

// RZ OF OF OF OF OF OF OF
// SZ SZ SZ SZ
// FL FL FL FL

typedef struct {
	uint64_t offset;
	uint32_t size;
	uint32_t flags;
} tad_v4_t;

#pragma pack(pop)

typedef struct {
	uint64_t offset;
	uint32_t size;
	uint32_t flags;
} tad_t;

int tad_v3_entry_active(tad_v3_t* tad);
int tad_v4_entry_active(tad_v4_t* tad);

#endif