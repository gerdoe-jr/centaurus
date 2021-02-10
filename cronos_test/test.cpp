#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include "crofile.h"
#include <algorithm>
#include "win32util.h"

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

    std::wstring fileName = L"CroStru";
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

    /*record_id idx = 1;
    do {
        uint32_t burst = bank.GetOptimalEntryCount();
        RecordTable table = bank.LoadRecordTable(idx, burst);
        if (!table.GetRecordCount()) break;

        record_id i = 0;
        do {
            BlockTable block = bank.LoadBlockTable(table, i);
            record_off start, end;
            block.GetRange(&start, &end);

            printf("%u\t%u\tBLOCK at %016llx, %u records in %u bytes\n",
                    table.Id() + start, table.Id() + end,
                    block.GetBlockTableOffset(),
                    block.GetBlockTableCount(),
                    block.GetBlockTableSize());

            i += block.GetBlockTableCount();
        } while (i < table.GetRecordCount());

        idx += table.GetRecordCount();
    } while (!bank.IsEndOfEntries());*/

    terminal_entry_mode(bank);

    bank.Close();
    return 0;
}
