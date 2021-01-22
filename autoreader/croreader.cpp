#include "croreader.h"
#include "win32util.h"
#include <strsafe.h>
#include <commctrl.h>
#include <time.h>
#include <cstddef>
#include <fstream>
#include <algorithm>
#include "json_file.h"

/* CroReader */

std::wstring CroReader::s_CroReaderProExe;

void CroReader::Init()
{
}

void CroReader::SetCroReaderProPath(const std::wstring& exe)
{
    s_CroReaderProExe = exe;
}

CroReader* CroReader::CreateCroReader(const std::wstring& bankDir)
{
    // 1. определить версию банка
    // 2. создать класс CroReader'а для работы с нужной версией exe
    // 2. запустить нужный exe CroReader'а под версию банка
    return dynamic_cast<CroReader*>(new CroReaderPro(bankDir));
}

void CroReader::LogError(const std::exception& exc, const std::wstring& msg)
{
    FILE* fErr1 = fopen("error1.txt", "w");
    fprintf(fErr1, "\"%s\"\t\"%s\"\n", exc.what(),
            WcharToText(msg).c_str());
    fclose(fErr1);
}

CroReader::CroReader(const std::wstring& bankDir)
{
    m_BankDir = bankDir;
}

CroReader::~CroReader()
{
}

void CroReader::SetOutputPath(const std::wstring& outputDir)
{
    m_OutputDir = outputDir;
}

std::wstring CroReader::GetOutputPath() const
{
    return m_OutputDir;
}

/* CroReaderPro */

CroReaderPro::CroReaderPro(const std::wstring& bankDir)
    : CroReader(bankDir)
{
    m_iLogItem = 0;
    m_bExists = false;
    m_llBankId = 0;
    SetExePath(s_CroReaderProExe);
}

CroReaderPro::~CroReaderPro()
{
}

void CroReaderPro::SetLogItem(int iLogItem)
{
    m_iLogItem = iLogItem;
}

int CroReaderPro::GetLogItem() const
{
    return m_iLogItem;
}

bool CroReaderPro::OutputPathExists() const
{
    return m_bExists;
}

bool CroReaderPro::StartCroReader()
{
    return StartApp(L"\"" + GetBankPath() + L"\\CroBank.dat" + L"\"");
}

bool CroReaderPro::WaitCroReaderInit()
{
    return WaitAppIdle();
}

void CroReaderPro::OnAppWindow(HWND hWnd)
{
    std::string name = GetWindowClass(hWnd);

    if (name == "TApplication")
        m_hApplication = hWnd;
    else if (name == "TMainForm")
    {
        m_hMainForm = hWnd;

        m_hPageControl = GetChild(m_hMainForm, "TPageControl");
        m_hTabMemo = GetChild(m_hPageControl, "TTabSheet", L"Свойства");

        m_hMemo = GetChild(m_hTabMemo, "TMemo");

        m_hTabBase = GetChild(m_hPageControl, "TTabSheet", L"Базы");
        m_hBaseView = GetChild(m_hTabBase, "TTreeView");
    }
    else if (name == "TExportCSVForm")
        m_hExportForm = hWnd;
    else if (name == "TProgressForm")
        m_hProgressForm = hWnd;
}

#include <stdio.h>

#define PRINT_VAR(name) printf("%s\t%p\n", #name, name)

void CroReaderPro::SetupCroReader()
{
    WaitAppIdle(1000);
    MonitorSetup();

    PRINT_VAR(m_hApplication);
    PRINT_VAR(m_hMainForm);

    PRINT_VAR(m_hPageControl);

    PRINT_VAR(m_hTabMemo);
    PRINT_VAR(m_hMemo);

    PRINT_VAR(m_hTabBase);
    PRINT_VAR(m_hBaseView);

    PRINT_VAR(m_hExportForm);
    PRINT_VAR(m_hProgressForm);

    /*EnumAppControls(m_hPageControl,
        [](AppMonitor* app, HWND hWnd) {
            CroReaderPro* pro = dynamic_cast<CroReaderPro*>(app);
            if (app->GetWindowClass(hWnd) == "TTabSheet")
            {
                pro->m_Tabs.insert(std::make_pair(
                    app->GetControlTextStr(hWnd), hWnd));
            }
            return true;
        }
    );

    for (auto it = m_Tabs.begin(); it != m_Tabs.end(); ++it)
        printf("\t%S\t\t%p\n", it->first.c_str(), it->second);*/
}

static unsigned int _JSHash(const std::wstring& str)
{
    unsigned int hash = 1315423911;

    for(std::size_t i = 0; i < str.length(); i++)
    {
        hash ^= ((hash << 5) + str[i] + (hash >> 2));
    }

    return (hash & 0x7FFFFFFF);
}

static unsigned _HasCSVFiles(const std::wstring& path)
{
    auto [files, _] = ListDirectory(path);
    unsigned uCsvFileCount = 0;

    for (auto& file : files)
    {
        if (file.find(L".csv") != std::wstring::npos)
            uCsvFileCount++;
    }

    return uCsvFileCount;
}

void CroReaderPro::LoadBankProperties()
{
    if (m_hMemo)
        m_BankMemo = GetControlTextStr(m_hMemo);
}

bool CroReaderPro::IsBankLoaded() const
{
    if (m_BankMemo.empty())
        return false;
    return true;
}

void CroReaderPro::LoadBankInfo()
{
    /* Свойства */
    size_t pos = 0;
    std::wstring del = L"\r\n";

    LoadBankProperties();
    std::wstring text = m_BankMemo;

    //MessageBoxW(NULL, text.c_str(), L"Memo", MB_OK);

    while ((pos = text.find(del)) != std::wstring::npos)
    {
        std::wstring line = text.substr(0, pos);

        if (!line.rfind(L"Имя банка: ", 0))
            m_BankName = line.substr(11);
        else if (!line.rfind(L"ID банка: ", 0))
            m_llBankId = std::stoll(line.substr(10));

        text.erase(0, del.length());
    }

    unsigned int _jhash = _JSHash(text);
    if (!m_llBankId)
        m_llBankId = _jhash;

    std::wstring subdir;
    size_t _pos = GetBankPath().find_last_of(L"\\/");
    if (_pos != std::wstring::npos)
        subdir = GetBankPath().substr(_pos+1);

    if (m_BankName.empty())
    {
        if (subdir.empty())
            m_BankName = std::to_wstring(m_llBankId);
        else m_BankName = subdir;
        m_BankName = std::to_wstring(m_llBankId) + L"_" + m_BankName;
    }

    std::wstring outputDir = GetOutputPath()
        + L"\\" + subdir;

    CreateDirectoryW(outputDir.c_str(), NULL);
    outputDir += L"\\" + m_BankName;
    CreateDirectoryW(outputDir.c_str(), NULL);
    SetOutputPath(outputDir);

    std::wstring existsFile = GetOutputPath() + L"\\.export";
    std::wstring jsonFile = GetOutputPath() + L"\\bank.json";

    m_bExists = GetFileAttributesW(existsFile.c_str()) != INVALID_FILE_ATTRIBUTES;

    if (!m_bExists)
    {
        unsigned uCsvInputFiles = _HasCSVFiles(GetBankPath());
        unsigned uCsvOutputFiles = _HasCSVFiles(GetOutputPath());

        if (!uCsvInputFiles && !uCsvOutputFiles)
            m_bExists = false;
        else if (uCsvInputFiles || uCsvOutputFiles)
            m_bExists = false;
        else if (!uCsvOutputFiles && uCsvInputFiles)
            m_bExists = false;
        else if (uCsvOutputFiles && !uCsvInputFiles)
            m_bExists = true;
    }

    json bases;

    /* Базы */
    DWORD_PTR dwBase = TV_GetNextItem(m_hBaseView, TVGN_ROOT);
    while (dwBase)
    {
        auto base = TV_GetItem(m_hBaseView, dwBase);

        json columns;
        DWORD dwColumn = TV_GetNextItem(m_hBaseView, TVGN_CHILD, dwBase);
        while (dwColumn)
        {
            auto column = TV_GetItem(m_hBaseView, dwColumn);
            columns.push_back({{"columnName", WcharToText(std::get<0>(column))},
                                {"columnType", std::get<1>(column)}});
            dwColumn = TV_GetNextItem(m_hBaseView, TVGN_NEXT, dwColumn);
        }

        std::wstring baseName = std::get<0>(base);
        std::wstring baseFile = GetOutputPath()
                                + L"\\" + baseName + L".csv";

        m_BaseFiles.push_back(baseFile);
        bases.push_back({{"baseName", WcharToText(baseName)},
                        {"baseColumns", columns},
                        {"baseFile", WcharToText(baseName) + ".csv"}});
        dwBase = TV_GetNextItem(m_hBaseView, TVGN_NEXT, dwBase);
    }

    WriteJSONFile(jsonFile, {
        {"bankName", WcharToText(m_BankName)},
        {"bankMemo", WcharToText(m_BankMemo)},
        {"bankMemoHash", _jhash},
        {"bankId", m_llBankId},
        {"bankSource", WcharToText(GetBankPath())},
        {"bankBases", bases}
    });
}

void CroReaderPro::WriteError(const std::wstring& error)
{
    json bank;
    std::wstring path = GetOutputPath() + L"\\bank.json";

    try {
        bank = ReadJSONFile(path);
    } catch (const std::exception& e) {
        CroReader::LogError(e, path);
    }

    bank["bankError"] = WcharToText(error);
    WriteJSONFile(path, bank);
}

std::wstring CroReaderPro::GetBankName() const
{
    return m_BankName;
}

long long CroReaderPro::GetBankId() const
{
    return m_llBankId;
}

#define MESSAGEBOX_CLASS "#32770"

bool CroReaderPro::HasMessageBox()
{
    if (FindAppWindow(MESSAGEBOX_CLASS))
        return true;
    //if (GetChild(m_hApplication, MESSAGEBOX_CLASS))
    //  return true;
    return false;
}

std::wstring CroReaderPro::GetMessageBoxText()
{
    HWND hMsgBox = FindAppWindow(MESSAGEBOX_CLASS);
    //if (!hMsgBox)
    //  hMsgBox = GetChild(m_hApplication, MESSAGEBOX_CLASS);
    if (!hMsgBox)
        return L"";

    auto pszText = std::make_unique<wchar_t[]>(MAX_PATH);
    HWND hText = GetDlgItem(hMsgBox, 0xFFFF);
    GetWindowTextW(hText, pszText.get(), MAX_PATH);

    return std::wstring(pszText.get());
}

void CroReaderPro::CloseMessageBox()
{
    HWND hMsgBox = FindAppWindow(MESSAGEBOX_CLASS);
    if (!hMsgBox)
        return;
    AppMessage(hMsgBox, WM_COMMAND,
        MAKEWPARAM(IDCANCEL, BN_CLICKED), 0);
}

#define CROREADER_MENU_EXPORTCSV 4

void CroReaderPro::StartExport()
{
    if (!HasExportForm())
    {
        AppPostMessage(m_hMainForm, WM_COMMAND,
                        CROREADER_MENU_EXPORTCSV, 0);
    }
    else if (!HasExport())
    {
        HWND hButton = GetChild(m_hExportForm, "TButton", L"OK");
        int iBtnOk = GetDlgCtrlID(hButton);
        AppPostMessage(m_hExportForm, WM_COMMAND,
                        0, (LPARAM)iBtnOk);
    }
    //AppMessage(m_hExportForm, WM_SHOWWINDOW, (WPARAM)TRUE, 0);
}

bool CroReaderPro::HasExportForm()
{
    printf("HasExportForm %d\n", IsWindowVisible(m_hExportForm));
    return IsWindowVisible(m_hExportForm);
}

bool CroReaderPro::HasExport()
{
    printf("HasExport %d\n", IsWindowVisible(m_hProgressForm));
    return IsWindowVisible(m_hProgressForm);
}

void CroReaderPro::MoveFiles()
{
    auto pFind = std::make_unique<WIN32_FIND_DATAW>();
    std::vector<std::wstring> baseFiles;

    std::wstring findPath = GetBankPath() + L"\\base*.csv";
    HANDLE hFind = FindFirstFileW(findPath.c_str(), pFind.get());
    if (hFind)
    {
        do {
            baseFiles.push_back(pFind->cFileName);
        } while(FindNextFile(hFind, pFind.get()));
        FindClose(hFind);
    }

    std::sort(baseFiles.begin(), baseFiles.end(),
        [](const std::wstring& a, const std::wstring& b) {
            int _a = 0, _b = 0;
            swscanf(a.c_str(), L"base%d.csv", &_a);
            swscanf(b.c_str(), L"base%d.csv", &_b);
            return _a < _b;
        }
    );

    std::vector<std::wstring> newFiles = m_BaseFiles;
    for(std::wstring& baseFile : baseFiles)
    {
        std::wstring oldFile = GetBankPath() + L"\\" + baseFile;
        std::wstring newFile;
        if (!newFiles.empty())
        {
            newFile = newFiles.front();
            newFiles.erase(newFiles.begin());
        }
        else newFile = GetOutputPath() + L"\\" + baseFile;
        MoveFileExW(oldFile.c_str(), newFile.c_str(),
                    MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH);
    }

    std::wstring filesPath = GetBankPath() + L"\\files";
    if (GetFileAttributesW(filesPath.c_str()) != INVALID_FILE_ATTRIBUTES)
    {
        MoveFileExW(filesPath.c_str(),
                    (GetOutputPath() + L"\\files").c_str(),
                    MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH);
    }

    std::wstring existsFile = GetOutputPath() + L"\\.export";
    FILE* fExport = _wfopen(existsFile.c_str(), L"wb");
    fclose(fExport);
}

int CroReaderPro::GetExportProgress()
{
    HWND hProgressBar = GetChild(m_hProgressForm, "TProgressBar");
    if (!hProgressBar)
        return -1;

    DWORD_PTR dwLow, dwHigh, dwPos;
    AppMessage(hProgressBar, PBM_GETRANGE, (WPARAM)TRUE, 0, &dwLow);
    AppMessage(hProgressBar, PBM_GETRANGE, (WPARAM)FALSE, 0, &dwHigh);
    AppMessage(hProgressBar, PBM_GETPOS, 0, 0, &dwPos);

    return (int)(((float)dwPos - dwLow) / ((float)dwHigh - dwLow) * 100);
}
