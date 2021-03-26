#include <stdio.h>
#include <centaurus.h>
#include <win32util.h>

bool exportMode = true;
std::wstring rootPath = L"K:\\Centaurus";
std::wstring bankPath = L"K:\\Cronos\\TestBanks";
centaurus_size tableLimit = 512; //512 MB
unsigned workerLimit = 4;

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
    printf("[FindBanks] %s\n", WcharToAnsi(path, 866).c_str());

    auto [files, dirs] = ListDirectory(path);
    for (auto& file : files)
    {
        if (file == L"CroBank.dat")
        {
            centaurus->ConnectBank(path);
            ICentaurusBank* bank = centaurus->WaitBank(path);
            if (!centaurus->IsBankLoaded(bank))
            {
                fprintf(stderr, "[FindBanks] Failed to connect %s\n",
                    WcharToAnsi(path, 866).c_str());
                break;
            }

            std::string name = WcharToAnsi(bank->BankName(), 866);
            printf("[FindBanks] \"%s\", ID %llu\n",
                name.c_str(), bank->BankId());

            if (exportMode)
            {
                if (centaurus->IsBankExported(bank->BankId()))
                {
                    printf("[FindBanks] \"%s\" already exported\n",
                        name.c_str());
                }
                else
                {
                    ICentaurusTask* exportTask = CentaurusTask_Export(bank);
                    centaurus->StartTask(exportTask);
                }
            }
            break;
        }
    }

    for (auto& dir : dirs)
    {
        if (dir != L"Voc")
            FindBanks(path + L"\\" + dir);
    }
}

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; i++)
    {
        std::string option = argv[i];
        if (option == "--export") exportMode = true;
        else if (option == "--root")
            rootPath = AnsiToWchar(argv[++i], 1251);
        else if (option == "--bank")
            bankPath = AnsiToWchar(argv[++i], 1251);
        else if (option == "--buffer")
            tableLimit = atoi(argv[++i]);
        else if (option == "--workers")
            workerLimit = atoi(argv[++i]);
    }

    if (!Centaurus_Init(rootPath))
    {
        fprintf(stderr, "Failed to init centaurus API!\n");
        return 1;
    }

    centaurus->SetTableSizeLimit(tableLimit * 1024 * 1024);
    centaurus->SetWorkerLimit(workerLimit);

    FindBanks(bankPath);

    Centaurus_RunThread();
    Centaurus_Idle();

    Centaurus_Exit();
    return 0;
}
