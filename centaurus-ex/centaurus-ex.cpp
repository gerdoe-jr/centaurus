#include <stdio.h>
#include <centaurus.h>

int main(int argc, char** argv)
{
    if (!InitCentaurusAPI())
    {
        fprintf(stderr, "Failed to init centaurus API!\n");
        return 1;
    }

    std::wstring bankPath = L"K:\\Cronos\\TestBanks\\Test1"
        L"\\11_Республика Коми Нарьян-Мар\\Phones";
    ICentaurusBank* bank = centaurus->ConnectBank(bankPath);

    if (!bank)
    {
        fprintf(stderr, "Failed to open bank!\n");
        return 1;
    }
    printf("bank %p\n", bank);

    try {
        bank->ExportHeaders();
        centaurus->LogBankFiles(bank);
    } catch (const std::exception& e) {
        fprintf(stderr, "centaurus bank exception: %s\n", e.what());
    }

    centaurus->DisconnectBank(bank);
    ExitCentaurusAPI();
    return 0;
}
