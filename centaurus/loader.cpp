#include "loader.h"
#include "export.h"
#include "bank.h"
#include <win32util.h>

void CentaurusLoader::RequestBank(const std::wstring& path)
{
    {
        auto lock = scoped_lock(m_DataLock);
        m_Dirs.push(path);
    }
    m_SyncCond.notify_one();
}

void CentaurusLoader::RequestBanks(std::vector<std::wstring> dirs)
{
    {
        auto lock = scoped_lock(m_DataLock);
        for (const auto& dir : dirs) m_Dirs.push(dir);
    }
    m_SyncCond.notify_one();
}

void CentaurusLoader::Execute()
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

bool CentaurusLoader::LoadPath(const std::wstring& path)
{
    auto exp = std::make_unique<CentaurusExport>();
    CentaurusBank* bank = new CentaurusBank();
    m_pLoadedBank = NULL;

    bank->AssociatePath(path);
    if (!exp->AcquireBank(bank))
    {
        delete bank;
        return false;
    }

    exp->SetTargetBank(bank);
    m_pLoadedBank = bank;

    bank->LoadBankInfo(exp.get());

    return true;
}

void CentaurusLoader::LogLoaderFail(const std::wstring& dir)
{
    fprintf(stderr, "[CentaurusLoader] failed to load %s\n",
        WcharToAnsi(dir).c_str());
}
