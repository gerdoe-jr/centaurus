#ifndef __CENTAURUS_FETCH_H
#define __CENTAURUS_FETCH_H

#include "worker.h"
#include <string>
#include <queue>

class CentaurusFetch : public CentaurusWorker
{
public:
    CentaurusFetch();

    void RequestBank(const std::wstring& path);
    void RequestBanks(std::vector<std::wstring> dirs);
    void RequestAll(const std::wstring& path);
protected:
    void Execute() override;

    bool LoadPath(const std::wstring& path);
private:
    std::queue<std::wstring> m_Dirs;
public:
    std::wstring m_LoadedPath;
    ICentaurusBank* m_pLoadedBank;
};

#endif