#include <stdio.h>
#include <centaurus.h>
#include <win32util.h>

void FixHeader(const std::wstring& datPath)
{
    /*FILE* fDat = _wfopen(datPath.c_str(), L"r+b");
    if (fDat)
    {
        fseek(fDat, 0, SEEK_SET);
        fwrite("CroFile", 7, 1, fDat);
        fclose(fDat);
    }*/
}

void FindBanks(const std::wstring& path)
{
    auto [files, dirs] = ListDirectory(path);
    for (auto& file : files)
    {
        if (file == L"CroBank.dat")
        {
            centaurus->ConnectBank(path);
            break;
        }
    }

    for (auto& dir : dirs)
        if (dir != L"Voc") FindBanks(path + L"\\" + dir);
}

int main(int argc, char** argv)
{
    if (!Centaurus_Init(L"K:\\Centaurus"))
    {
        fprintf(stderr, "Failed to init centaurus API!\n");
        return 1;
    }

    //std::wstring defaultBank = L"K:\\Cronos\\TestBanks\\Test1"
    //    L"\\11_Республика Коми Нарьян-Мар\\Phones";
    //std::wstring defaultBank = L"K:\\Cronos\\TestBanks\\Test1"
    //    L"\\11_Республика Коми РЕГ_Коми Прописка 2005";
    std::wstring defaultBank = L"K:\\Cronos\\TestBanks\\Test4\\testbank1";
    //std::wstring defaultBank = L"K:\\Cronos\\Banks\\МОС ГИБДД Права 2018";
    //std::wstring defaultBank = L"K:\\Cronos\\Banks\\Блогеры";
    std::wstring bankPath = argc >= 2
        ? AnsiToWchar(argv[1], 1251) : defaultBank;

    centaurus->ConnectBank(bankPath);
    ICentaurusBank* bank = centaurus->WaitBank(bankPath);

    if (!centaurus->IsBankLoaded(bank))
    {
        fprintf(stderr, "Failed to open bank!\n");
        return 1;
    }

    /*try {
        centaurus->LogBankFiles(bank);

        ICentaurusTask* exportTask = CentaurusTask_Export(bank);
        centaurus->StartTask(exportTask);
    } catch (const std::exception& e) {
        fprintf(stderr, "centaurus bank exception: %s\n", e.what());
    }*/

    FindBanks(L"K:\\Cronos\\TestBanks");

    getc(stdin);

    Centaurus_RunThread();
    Centaurus_Idle();

    //centaurus->DisconnectBank(bank);
    Centaurus_Exit();
    return 0;
}
