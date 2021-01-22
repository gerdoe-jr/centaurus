#ifndef __CROREADER_H
#define __CROREADER_H

#include "win32ctrl.h"
#include <exception>
#include <string>
#include <vector>
#include <map>

class CroReader : public AppMonitor
{
public:
    static void Init();

    static void SetCroReaderProPath(const std::wstring& path);
    static CroReader* CreateCroReader(const std::wstring& bankDir);
    static void LogError(const std::exception& exc, const std::wstring& msg);

    CroReader(const std::wstring& bankDir);
    virtual ~CroReader();

    virtual std::wstring GetBankPath() const
    {
        return m_BankDir;
    }

    virtual void SetLogItem(int iLog) = 0;
    virtual int GetLogItem() const = 0;
    virtual bool OutputPathExists() const = 0;

    virtual bool StartCroReader() = 0;
    virtual bool WaitCroReaderInit() = 0;
    virtual void SetupCroReader() = 0;
    virtual void LoadBankProperties() = 0;
    virtual bool IsBankLoaded() const = 0;
    virtual void LoadBankInfo() = 0;
    virtual void WriteError(const std::wstring& error) = 0;
    virtual void StartExport() = 0;
    virtual bool HasExportForm() = 0;
    virtual bool HasExport() = 0;
    virtual void MoveFiles() = 0;

    virtual int GetExportProgress() = 0;

    virtual std::wstring GetBankName() const = 0;
    virtual long long GetBankId() const = 0;

    virtual bool HasMessageBox() = 0;
    virtual std::wstring GetMessageBoxText() = 0;
    virtual void CloseMessageBox() = 0;

    virtual void SetOutputPath(const std::wstring& path);
    virtual std::wstring GetOutputPath() const;
protected:
    static std::wstring s_CroReaderProExe;
private:
    std::wstring m_BankDir;
    std::wstring m_OutputDir;
};

class CroReaderPro : public CroReader
{
public:
    CroReaderPro(const std::wstring& bankDir);
    virtual ~CroReaderPro();

    virtual void SetLogItem(int iLog);
    virtual int GetLogItem() const;
    virtual bool OutputPathExists() const;

    virtual bool StartCroReader();
    virtual bool WaitCroReaderInit();
    virtual void SetupCroReader();

    void OnAppWindow(HWND hWnd) override; //AppMonitor

    virtual void LoadBankProperties();
    virtual bool IsBankLoaded() const;
    virtual void LoadBankInfo();
    virtual void WriteError(const std::wstring& error);

    virtual std::wstring GetBankName() const;
    virtual long long GetBankId() const;

    virtual bool HasMessageBox();
    virtual std::wstring GetMessageBoxText();
    virtual void CloseMessageBox();

    virtual void StartExport();
    virtual bool HasExportForm();
    virtual bool HasExport();
    virtual void MoveFiles();

    virtual int GetExportProgress();
//private:
    int m_iLogItem;
    bool m_bExists;

    HWND m_hApplication;

    HWND m_hMainForm;
    HWND m_hPageControl;
    std::map<std::wstring,HWND> m_Tabs;

    HWND m_hTabMemo;
    HWND m_hMemo;

    HWND m_hTabBase;
    HWND m_hBaseView;

    HWND m_hExportForm;
    HWND m_hProgressForm;

    std::wstring m_BankMemo;
    std::wstring m_BankName;
    long long m_llBankId;

    std::vector<std::wstring> m_BaseFiles;
};

#endif
