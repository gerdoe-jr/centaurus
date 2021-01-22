#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cronos.h"

const char* s_pDefBank = "K:\\Cronos\\TestBanks\\Test3\\voters";

void dump_tad(FILE* fTad)
{
	unsigned idx = 0;
	fseek(fTad, TAD_V4_BASE, SEEK_SET);
	while (!feof(fTad))
	{
		tad_v4_t tad;
		fread(&tad, sizeof(tad), 1, fTad);
		if (!tad_v4_entry_active(&tad))
			break;
		printf("%u\t\t%016LX,%u\t&%X\n", idx++,
			TAD_V4_OFFSET(tad.offset), tad.size,
			tad.flags);
	}
}

int main(int argc, char** argv)
{
	char szBankPath[PATH_MAX] = {0};
	char szFile[PATH_MAX] = {0};
	strcpy(szBankPath, argc == 1 ? s_pDefBank : argv[1]);

	strcpy(szFile, szBankPath);
	strcat(szFile, "\\CroBank.tad");
	FILE* fTad = fopen(szFile, "rb");
	dump_tad(fTad);
	fclose(fTad);

	return 0;
}