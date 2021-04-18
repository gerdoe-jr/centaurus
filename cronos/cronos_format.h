#ifndef __CRONOS_FORMAT_H
#define __CRONOS_FORMAT_H

/* Cronos Versions */

// 01.02    3x, small model
// 01.03    3x, big model
// 01.04    3x, lite

// 01.11    4x
// 01.13    4x, pro
// 01.14    4x, lite

// 01.19    7x

/* Cronos file header */

// hh hh hh hh  hh hh hh ??
// ?? ?? vs vs  vs vs vs vs
// fl fl dl dl  .. .. .. ..

#define CRONOS_HEADER_SIZE  248
#define CRONOS_HEADER_SIG   "CroFile"

#define CRONOS_SECRET       0x13
#define CRONOS_LITESECRET   0x33

#define CRONOS_ENCRYPTION   (1<<0)
#define CRONOS_COMPRESSION  (1<<1)

/* Cronos 3x format */

#define CRONOS3              3

#define CRONOS3_PAD_OFFSET   0xF8
#define CRONOS3_PAD_SIZE     4

#define CRONOS3_CRYPT_OFFSET 0xFC
#define CRONOS3_CRYPT_SIZE   512

/* TAD */

// of of of of  sz sz sz sz
// fl fl fl fl
#define CRONOS3_MASK_OFFSET 0x1FFFFFFF
#define CRONOS3_MASK_FSIZE  0x7FFFFFFF

#define TAD_V3_BASE         0x08
#define TAD_V3_SIZE         0x0C
#define TAD_V3_DELETED      0xFFFFFFFF
#define TAD_V3_RZ_NOBLOCK   0x80000000
#define TAD_V3_OFFSET(off)  (off&CRONOS3_MASK_OFFSET)
#define TAD_V3_FSIZE(off)   (off&CRONOS3_MASK_FSIZE)
#define TAD_V3_INVALID      (TAD_V3_DELETED & CRONOS3_MASK_FSIZE)

/* Cronos 4x format */

#define CRONOS4              4

#define CRONOS4_PAD_OFFSET   0xF8
#define CRONOS4_PAD_SIZE     0

#define CRONOS4_CRYPT_OFFSET 0xF8
#define CRONOS4_CRYPT_SIZE   512

/* TAD */

// rz of of of  of of of of
// sz sz sz sz  fl fl fl fl
#define CRONOS4_MASK_OFFSET 0x0000FFFFFFFFFFFF
#define CRONOS4_MASK_FSIZE  0xFFFFFFFFFFFFFFFF

#define TAD_V4_BASE         0x10
#define TAD_V4_SIZE         0x10
#define TAD_V4_RZ(off)      (off>>56)
#define TAD_V4_RZ_DELETED   (1<<1)
#define TAD_V4_DELETED      0xFFFFFFFFFFFFFFFF
#define TAD_V4_OFFSET(off)  (off&CRONOS4_MASK_OFFSET)
#define TAD_V4_FSIZE(off)   (off&CRONOS4_MASK_FSIZE)
#define TAD_V4_INVALID      (TAD_V4_DELETED & CRONOS4_MASK_FSIZE)

/* Cronos 7x format */

#define CRONOS7              7

#define CRONOS7_PAD_OFFSET   0xF8
#define CRONOS7_PAD_SIZE     0

#define CRONOS7_CRYPT_OFFSET 0xF8
#define CRONOS7_CRYPT_SIZE   256

#endif
