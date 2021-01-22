#include "cronos.h"

int tad_v3_entry_active(tad_v3_t* tad)
{
	return tad->size != 0 && tad->size != 0xFFFFFFFF;
}

int tad_v4_entry_active(tad_v4_t* tad)
{
	if (TAD_V4_RZ(tad->offset) & TAD_V4_RZ_FLAG_DELETED)
		return 0;
	return TAD_V4_OFFSET(tad->offset) != 0;
}