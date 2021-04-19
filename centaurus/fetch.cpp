#include "fetch.h"
#include "bank.h"
#include "cronos_api.h"
#include <win32util.h>

CentaurusFetch::CentaurusFetch()
{
    m_LoadedPath = L"";
    m_pLoadedBank = NULL;
}

void CentaurusFetch::RequestBank(const std::wstring& path)
{
    {
        auto lock = scoped_lock(m_DataLock);
        m_Dirs.push(path);
    }
    m_SyncCond.notify_one();
}

void CentaurusFetch::RequestBanks(std::vector<std::wstring> dirs)
{
    {
        auto lock = scoped_lock(m_DataLock);
        for (const auto& dir : dirs) m_Dirs.push(dir);
    }
    m_SyncCond.notify_one();
}

void CentaurusFetch::Execute()
{
    if (m_Dirs.empty())
    {
        m_State = Waiting;
        return;
    }

    std::wstring dir;
    {
        auto lock = scoped_lock(m_DataLock);
        dir = m_Dirs.front();
        m_Dirs.pop();
    }

    m_LoadedPath = dir;
    if (!LoadPath(dir))
    {
        m_pLoadedBank = NULL;
        LogLoaderFail(dir);
    }
}

bool CentaurusFetch::LoadPath(const std::wstring& path)
{
    auto cro = std::make_unique<CronosAPI>("CentaurusFetch");
    CentaurusBank* bank = new CentaurusBank();
    m_pLoadedBank = NULL;

    try {
        bank->SetBankPath(path);
        cro->LoadBank(bank);

        bank->LoadStructure(cro.get());
        m_pLoadedBank = bank;
    }
    catch (const std::exception& e) {
        Error("%s", e.what());

        delete bank;
        m_pLoadedBank = NULL;
    }

    return m_pLoadedBank != NULL;
}

void CentaurusFetch::LogLoaderFail(const std::wstring& dir)
{
    Error("failed to load %s\n", WcharToAnsi(dir).c_str());
}
