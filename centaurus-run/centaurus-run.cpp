#include <stdio.h>
#include <centaurus.h>
#include <win32util.h>

bool testMode = false;
bool exportMode = true;
std::wstring rootPath;
std::wstring bankPath;
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

static ICentaurusLogger* findLog;

void FindBanks(const std::wstring& path)
{
    findLog->Log("%s\n", WcharToTerm(path).c_str());
    
    auto [files, dirs] = ListDirectory(path);
    for (auto& file : files)
    {
        if (file == L"CroBank.dat")
        {
            centaurus->ConnectBank(path);
            ICentaurusBank* bank = centaurus->WaitBank(path);
            if (!centaurus->IsBankLoaded(bank))
            {
                findLog->Error("Failed to connect %s\n",
                    WcharToTerm(path).c_str());
                break;
            }

            std::string name = WcharToAnsi(bank->BankName());
            findLog->Log("\"%s\", ID %d\n", name.c_str(), bank->BankId());

            if (testMode)
            {
                findLog->Log("TEST \"%s\"\n", WcharToTerm(path).c_str());
            }
            else if (exportMode)
            {
                if (centaurus->IsBankExported(bank->BankId()))
                {
                    findLog->Log("\"%s\" already exported\n", name.c_str());
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
        {
            FindBanks(JoinFilePath(path, dir));
        }
    }
}

int main(int argc, char** argv)
{
    if (IsWindowsSystem())
    {
        rootPath = L"K:\\Centaurus";
        bankPath = L"K:\\Cronos\\TestBanks";
    }
    else if (IsLinuxSystem())
    {
        rootPath = L"/media/Volume_K/Centaurus";
        bankPath = L"/media/Volume_K/Cronos/TestBanks/Test4";
    }

    for (int i = 1; i < argc; i++)
    {
        std::string option = argv[i];
        if (option == "--test") testMode = true;
        else if (option == "--export") exportMode = true;
        else if (option == "--root")
            rootPath = TermToWchar(argv[++i]);
        else if (option == "--bank")
            bankPath = TermToWchar(argv[++i]);
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

    centaurus->SetMemoryLimit(tableLimit * 1024 * 1024);
    centaurus->SetWorkerLimit(workerLimit);

    findLog = CentaurusLogger_Forward("FindBanks");
    FindBanks(bankPath);

    Centaurus_RunThread();
    Centaurus_Idle();

    CentaurusLogger_Destroy(findLog);
    Centaurus_Exit();
    return 0;
}
