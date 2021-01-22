#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include "cronos.h"
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
            snprintf(szHdrText, 32, "%02d.%02d", cro.GetMajor(),
                    cro.GetMinor());
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
        fprintf(stderr, "CroFile error %d\n", st);
        return 1;
    }

    printf("bank version %d\n", bank.GetVersion());

    record_id idx = 1;
    do {
        uint32_t burst = bank.GetOptimalEntryCount();
        RecordTable table = bank.LoadRecordTable(idx, burst);
        if (!table.GetRecordCount()) break;

        /*BlockTable block = bank.LoadBlockTable(table, 0);
        printf("%016llx\t%llu\t%u\n", block.GetBlockTableOffset(),
                block.GetBlockTableSize(), block.GetBlockTableCount());*/
        record_id i = 0;
        do {
            BlockTable block = bank.LoadBlockTable(table, i);
            printf("block record count %u\n", block.GetBlockTableCount());

            i += block.GetBlockTableCount();
        } while (i < table.GetRecordCount());

        /*printf("\t%u blocks\n", table.BlockRecordCount(idx, 64*1024*1024));

        for (unsigned i = 0; i < table.GetRecordCount(); i++)
        {
            RecordEntry entry = table.GetRecordEntry(i);
            if (!entry.IsActive()) continue;

            printf("%u\t%016llx\t%08x\t%08x\n", 
                    entry.Id(), entry.GetOffset(),
                    entry.GetSize(), entry.GetFlags());
        }*/

        idx += table.GetRecordCount();
    } while (!bank.IsEndOfEntries());

    bank.Close();
    return 0;
}
