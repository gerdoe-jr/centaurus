#ifndef __CENTAURUS_FETCH_H
#define __CENTAURUS_FETCH_H

#include "worker.h"
#include <string>
#include <queue>

class CentaurusFetch : public CentaurusWorker
{
public:
    void RequestBank(const std::wstring& path);
    void RequestBanks(std::vector<std::wstring> dirs);
protected:
    void Execute() override;

    bool LoadPath(const std::wstring& path);
    void LogLoaderFail(const std::wstring& dir);
private:
    std::queue<std::wstring> m_Dirs;
public:
    std::wstring m_LoadedPath;
    ICentaurusBank* m_pLoadedBank;
};

#endif