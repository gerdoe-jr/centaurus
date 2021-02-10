#include <stdio.h>
#include <stdlib.h>
#include "crofile.h"
#include "croexception.h"
#include "win32util.h"

#ifdef min
#undef min
#endif
#include <algorithm>

static std::wstring testBank = 
    L"K:\\Cronos\\TestBanks\\Test1\\11_Республика Коми Нарьян-Мар\\Phones";
static std::vector<std::string> known_versions;

void scan_directory(const std::wstring& path)
{
    bool logged = false;
    auto [files, dirs] = ListDirectory(path);

    for (auto& file : files)
    {
        if (file.find(L"CroBank.dat") != std::wstring::npos
                || file.find(L"CroStru.dat") != std::wstring::npos)
        {
            if (!logged)
            {
                 printf("%s\n", WcharToAnsi(path).c_str());
                 logged = true;
            }

            printf("\t%s\t", WcharToAnsi(file).c_str());
            CroFile cro = CroFile(path + L"\\" + file.substr(0, 7));
            crofile_status st = cro.Open();
            if (st != CROFILE_OK)
            {
                printf("[%u,\"%s\"]\n", cro.GetStatus(),
                        cro.GetError().c_str());
                continue;
            }

            char szHdrText[32] = {0};
            cronos_abi_num ver = cro.GetABIVersion();
            snprintf(szHdrText, 32, "%02d.%02d", ver.first, ver.second);
            std::string hdrtext = szHdrText;
            printf("%s\n", hdrtext.c_str());
            if (std::find(known_versions.begin(), known_versions.end(),
                    hdrtext) == known_versions.end())
            {
                known_versions.push_back(hdrtext);
            }

            cro.Close();
        }
    }

    for (auto& dir : dirs)
        scan_directory(path + L"\\" + dir);
}

void terminal_entry_mode(CroFile& bank)
{
    printf("get entry mode\n:<entry> <count>\n\n");

    int id_entry = INVALID_CRONOS_ID;
    unsigned id_count;

    do {
        printf(":");
        if (scanf("%d %u", &id_entry, &id_count) < 2)
            break;
        bank.Reset();

        CroEntryTable table = bank.LoadEntryTable(
            (cronos_id)id_entry, id_count);
        printf("TABLE %" FCroId "->%" FCroId
            " off %016" FCroOff " size %" FCroSize "\n",
            table.IdStart(), table.IdEnd(),
            table.TableOffset(), table.TableSize()
        );

        for (cronos_id id = table.IdStart(); id != table.IdEnd(); id++)
        {
            CroEntry entry = table.GetEntry(id);
            if (entry.IsActive())
            {
                printf("%" FCroId " RECORD at %" FCroOff ", size %" FCroSize
                    ", flags %" FCroFlags "\n",
                    entry.Id(), entry.EntryOffset(), entry.EntrySize(),
                    entry.EntryFlags()
                );
            }
            else printf("%" FCroId " INACTIVE RECORD\n", entry.Id());                   
        }
    } while (id_entry != INVALID_CRONOS_ID);
}

CroBuffer dump_record(CroData& dat, CroEntry& entry)
{
    CroBuffer record;

    // block header

    return record;
}

void dump_crofile(CroFile* file)
{
    file->Reset();

    cronos_id tad_table_id = 1;
    while (!file->IsEndOfEntries())
    {
        cronos_idx tad_count = file->OptimalEntryCount();
        CroEntryTable tad = file->LoadEntryTable(tad_table_id, tad_count);
        if (tad.IsEmpty()) break;
        
        cronos_id dat_table_id = tad_table_id;
        while (dat_table_id < tad_table_id + tad_count)
        {
            cronos_idx dat_count = std::min(tad_count,
                file->OptimalRecordCount(tad, tad.IdStart()));
            CroRecordTable dat = file->LoadRecordTable(
                tad, tad.Id(), dat_count);
            FILE* fTableDat = fopen("table_dat.bin", "wb");
            fwrite(dat.GetData(), dat.GetSize(), 1, fTableDat);
            fclose(fTableDat);

            for (cronos_id id = dat.IdStart(); id != dat.IdEnd(); id++)
            {
                if (!tad.GetEntry(id).IsActive())
                    continue;
                CroBlock block = dat.FirstBlock(id);

                printf("%" FCroId " BLOCK %" FCroOff " size %"
                    FCroSize " record %" FCroOff " NEXT %" FCroOff "\n",
                    id, block.GetStartOffset(), block.BlockSize(),
                    block.RecordOffset(), block.BlockNext()
                );
            }

            printf("DAT entry count %u\n", dat.GetEntryCount());
            break;
            dat_table_id += dat.GetEntryCount();
        }

        tad_table_id += tad.GetEntryCount();
    }

    printf("\n");
}

int main(int argc, char** argv)
{
    std::wstring bankPath = testBank;
    if (argc >= 2)
    {
        wchar_t szBank[MAX_PATH] = {0};
        MultiByteToWideChar(CP_THREAD_ACP, 0, argv[1], -1,
                szBank, MAX_PATH);
        bankPath = szBank;
    }

    std::wstring fileName = L"CroBank";
    if (argc == 3)
    {
        wchar_t szFile[64] = {0};
        MultiByteToWideChar(CP_THREAD_ACP, 0, argv[2], -1,
                szFile, 64);
        fileName = szFile;
    }

    if (fileName == L"--scan")
    {
        scan_directory(bankPath);
        printf("\n\n== found version headers:\n");
        for(auto& ver : known_versions)
            printf("\t%s\n", ver.c_str());
        return 0;
    }

    CroFile bank(bankPath + L"\\" + fileName);
    crofile_status st;

    if ((st = bank.Open()) != CROFILE_OK)
    {
        fprintf(stderr, "CroFile error %d\n\t%s\n",
                st, bank.GetError().c_str());
        return 1;
    }

    printf("bank version %d\n", bank.GetVersion());

    printf("optimal entry count %u\n", bank.OptimalEntryCount());
    CroEntryTable tad = bank.LoadEntryTable(1, bank.OptimalEntryCount());

    cronos_idx count = bank.OptimalRecordCount(tad, 1);
    printf("optimal record count %u\n", count);

    cronos_off start, end;
    bank.RecordTableOffsets(tad, 1, count, start, end);
    printf("%" FCroOff "-%" FCroOff " record table\n", start, end);

    dump_crofile(&bank);
    terminal_entry_mode(bank);

    bank.Close();
    return 0;
}
