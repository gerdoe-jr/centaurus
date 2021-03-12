﻿#include <stdio.h>
#include <centaurus.h>
#include <win32util.h>

int main(int argc, char** argv)
{
    if (!Centaurus_Init())
    {
        fprintf(stderr, "Failed to init centaurus API!\n");
        return 1;
    }

    std::wstring defaultBank = L"K:\\Cronos\\TestBanks\\Test1"
        L"\\11_Республика Коми Нарьян-Мар\\Phones";
    //std::wstring defaultBank = L"K:\\Cronos\\TestBanks\\Test1"
    //    L"\\11_Республика Коми РЕГ_Коми Прописка 2005";
    //std::wstring defaultBank = L"K:\\Cronos\\TestBanks\\Test4\\testbank1";
    std::wstring bankPath = argc >= 2
        ? AnsiToWchar(argv[1]) : defaultBank;

    std::wstring exportPath = L"K:\\Cronos\\Export";
    centaurus->SetExportPath(exportPath);

    ICentaurusBank* bank = centaurus->ConnectBank(bankPath);
    centaurus->WaitBank();

    if (!centaurus->IsBankLoaded(bank))
    {
        fprintf(stderr, "Failed to open bank!\n");
        return 1;
    }

    try {
        centaurus->LogBankFiles(bank);

        ICentaurusTask* exportTask = CentaurusTask_Export(bank);
        centaurus->StartTask(exportTask);

        centaurus->Idle();
    } catch (const std::exception& e) {
        fprintf(stderr, "centaurus bank exception: %s\n", e.what());
    }

    centaurus->DisconnectBank(bank);
    Centaurus_Exit();
    return 0;
}
