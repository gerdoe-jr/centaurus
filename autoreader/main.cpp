#ifndef UNICODE
#define UNICODE
#endif

#include <Windows.h>
#include <commctrl.h>
#include "worker.h"
#include "scheduler.h"
#include "win32ctrl.h"
#include "win32util.h"
#include "croreader.h"
#include <strsafe.h>
#include <stdlib.h>
#include <stdio.h>
#include <algorithm>
#include <vector>
#include <memory>

#define MAIN_WINDOW L"AutoReaderMain"
#define WIDTH 640
#define HEIGHT 480

LRESULT CALLBACK WndCallback(HWND, UINT, WPARAM, LPARAM);
HWND g_hWnd;

int WINAPI WinMain(HINSTANCE hInst,
                    HINSTANCE hPrevInst,
                    LPSTR lpCmdLine,
                    int nCmdShow)
{
    // инициализировать все методы
    AppMonitor::Init();
    CroReader::Init();

    //Create window
    WNDCLASSEX wndEx;

    ZeroMemory(&wndEx, sizeof(wndEx));
    wndEx.hInstance = hInst;
    wndEx.lpszClassName = MAIN_WINDOW;
    wndEx.lpfnWndProc = WndCallback;
    wndEx.cbSize = sizeof(wndEx);

    wndEx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndEx.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    wndEx.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndEx.hbrBackground = (HBRUSH)COLOR_BACKGROUND;

    RegisterClassEx(&wndEx);
    InitCommonControls();

    RECT rect = {0, 0, WIDTH, HEIGHT};
    AdjustWindowRectEx(
        &rect,
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
        FALSE,
        0
    );

    g_hWnd = CreateWindowEx(0,
        MAIN_WINDOW, L"AutoReader",
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        HWND_DESKTOP, NULL,
        hInst, NULL
    );
    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    ReconnectIO(true);

    //Process messages
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

wchar_t g_szCroReader[MAX_PATH] = {0};
wchar_t g_szPath[MAX_PATH] = {0};
wchar_t g_szOutPath[MAX_PATH] = {0};
unsigned int g_uMaxThreads = 4;
unsigned int g_uDefStackSize = 0;
HWND g_hLog = NULL;
HWND g_hProgressBar = NULL;
HWND g_hStatusLabel = NULL;

std::vector<std::wstring> g_Banks;
HANDLE g_hThread = NULL;
Scheduler* g_pScheduler = NULL;

int AddLog(LPCWSTR lpBank, LPCWSTR lpStatus)
{
    LVITEMW lvItem = { 0 };
    static int _iItem = 0;

    lvItem.mask = LVIF_TEXT;
    lvItem.pszText = _wcsdup(lpBank);
    lvItem.cchTextMax = wcslen(lvItem.pszText);
    lvItem.iItem = _iItem++;
    lvItem.iSubItem = 0;
    //ListView_InsertItem(g_hLog, &lvItem);
    SendMessage(g_hLog, LVM_INSERTITEMW, 0, (LPARAM)&lvItem);

    //ListView_SetItemText(g_hLog, lvItem.iItem, 1, _wcsdup(lpStatus));
    lvItem.iSubItem = 1;
    lvItem.pszText = _wcsdup(lpStatus);
    lvItem.cchTextMax = wcslen(lvItem.pszText);
    SendMessage(g_hLog, LVM_SETITEM, 0, (LPARAM)&lvItem);

    return lvItem.iItem;
}

void UpdateLog(int iItem, LPCWSTR lpStatus)
{
    LVITEMW lvItem = { 0 };

    lvItem.mask = LVIF_TEXT;
    lvItem.pszText = _wcsdup(lpStatus);
    lvItem.cchTextMax = wcslen(lvItem.pszText);
    lvItem.iItem = iItem;
    lvItem.iSubItem = 1;

    SendMessage(g_hLog, LVM_SETITEM, 0, (LPARAM)&lvItem);
}

void SetStatus(const std::wstring& status = L"")
{
    if (status.empty())
        ShowWindow(g_hStatusLabel, SW_HIDE);
    else
    {
        if (!IsWindowVisible(g_hStatusLabel))
            ShowWindow(g_hStatusLabel, SW_SHOW);
        SetWindowText(g_hStatusLabel, status.c_str());
    }
}

void StartProgress(unsigned uLow, unsigned uHigh)
{
    ShowWindow(g_hStatusLabel, SW_HIDE);
    ShowWindow(g_hProgressBar, SW_SHOW);

    SendMessage(g_hProgressBar, PBM_SETRANGE32,
                (WPARAM)uLow, (LPARAM)uHigh);
    SendMessage(g_hProgressBar, PBM_SETSTEP,
                (WPARAM)1, 0);
}

void UpdateProgress()
{
    PostMessage(g_hProgressBar, PBM_STEPIT, 0, 0);
}

void FinishProgress()
{
    ShowWindow(g_hProgressBar, SW_HIDE);
}

typedef unsigned int hwid_t[6];

static void _DoCheck()
{
    static hwid_t _hwids[] = {
        {0x846ee340,0x7039,0x11de,0x9d20,0x806e,0x6f6e6963},
        {0xb062c928,0x26af,0x11eb,0xa2a4,0x806e,0x6f6e6963},
        {0xf96988bf,0x0eda,0x11eb,0x875f,0x806e,0x6f6e6963}
    };
    static unsigned _hwid_count = sizeof(_hwids) / sizeof(hwid_t);
    static int _check_status = 0;

    hwid_t _hwid = { 0 };
    HW_PROFILE_INFOW hwInfo;
    int iErrorCode = 0; 

    switch(_check_status)
    {
    case 0:
        GetCurrentHwProfileW(&hwInfo);
        swscanf_s(hwInfo.szHwProfileGuid,
            L"{%08x-%04x-%04x-%04x-%04x%08x}",
            &_hwid[0], &_hwid[1], &_hwid[2],
            &_hwid[3], &_hwid[4], &_hwid[5]
        );

        _check_status = 2;
        for (unsigned i = 0; i < _hwid_count; i++)
        {
            if(!memcmp(&_hwid[0], &_hwids[i][0], sizeof(hwid_t)))
            {
                _check_status = 1;
                return;
            }
        }
        _DoCheck();
        break;
    case 1: break;
    case 2:
        uintptr_t uErrorCode = (uintptr_t)CreateThread;
        MessageBoxW(g_hWnd,
            (std::wstring(L"Ваша система не соответствует "
                    L"системным требованием для работы с AutoReader\n\n")
                    + std::wstring(L"Код ошибки: ")
                    + std::to_wstring(uErrorCode)
            ).c_str(),
            L"Ошибка",
        MB_ICONHAND);
        ExitProcess((DWORD)uErrorCode);
        break;
    }
}

DWORD WINAPI RunThread(LPVOID);
DWORD WINAPI CheckThread(LPVOID);

LRESULT CALLBACK WndCallback(HWND hWnd, UINT uMsg,
                            WPARAM wParam, LPARAM lParam)
{
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
    static HWND _hExePath;
    const HMENU btn_SetExePath = (HMENU)101;

    static HWND _hPath;
    const HMENU btn_SetPath = (HMENU)102;

    static HWND _hOutPath;
    static HWND _hThreads;

    const HMENU btn_Run = (HMENU)103;
    const HMENU btn_Stop = (HMENU)104;
    const HMENU btn_Check = (HMENU)105;

    if (uMsg == WM_CREATE)
    {
        RECT rcClient, rcWnd;
        GetWindowRect(hWnd, &rcWnd);
        GetClientRect(hWnd, &rcClient);
        int iBorder = (rcWnd.right - rcWnd.left - rcClient.right) / 2;

        _DoCheck();
        CreateWindowW(WC_STATIC,
            L"Путь к CroReader",
            WS_CHILD | WS_VISIBLE,
            0, 0, WIDTH, 20,
            hWnd, (HMENU)0,
            hInst, NULL);
        _hExePath = CreateWindowW(WC_EDIT,
            L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            0, 25, WIDTH - WIDTH/8, 20,
            hWnd, (HMENU)0,
            hInst, NULL);
        CreateWindowW(WC_BUTTON,
            L"Выбрать",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            WIDTH - WIDTH/8 + 5, 25, WIDTH/8 - 5, 20,
            hWnd, btn_SetExePath,
            hInst, NULL);

        CreateWindowW(WC_STATIC,
            L"Путь к банкам кроноса",
            WS_CHILD | WS_VISIBLE,
            0, 50, WIDTH, 20,
            hWnd, (HMENU)0,
            hInst, NULL);
        _hPath = CreateWindowW(WC_EDIT,
            L"K:\\Cronos\\TestBanks",
            //L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            0, 75, WIDTH - WIDTH/8, 20,
            hWnd, (HMENU)0,
            hInst, NULL);
        CreateWindowW(WC_BUTTON,
            L"Выбрать",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            WIDTH - WIDTH/8 + 5, 75, WIDTH/8 - 5, 20,
            hWnd, btn_SetPath,
            hInst, NULL);

        CreateWindowW(WC_STATIC,
            L"Путь экспорта CSV и количество потоков",
            WS_CHILD | WS_VISIBLE,
            0, 100, WIDTH, 20,
            hWnd, (HMENU)0,
            hInst, NULL);
        _hOutPath = CreateWindowW(WC_EDIT,
            L"K:\\Cronos\\Export",
            //L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            0, 125, WIDTH - WIDTH/8, 20,
            hWnd, (HMENU)0,
            hInst, NULL);
        _hThreads = CreateWindowW(WC_EDIT,
            L"4",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            WIDTH - WIDTH/8 + 5, 125, WIDTH/8 - 5, 20,
            hWnd, btn_SetPath,
            hInst, NULL);

        CreateWindowW(WC_BUTTON,
            L"Запустить",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            (WIDTH/4 + 5)*0, 150, WIDTH/4, 20,
            hWnd, btn_Run,
            hInst, NULL);
        CreateWindowW(WC_BUTTON,
            L"Остановить",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            (WIDTH/4 + 5)*1, 150, WIDTH/4, 20,
            hWnd, btn_Stop,
            hInst, NULL);
        CreateWindowW(WC_BUTTON,
            L"Проверить",
            WS_CHILD | WS_VISIBLE | WS_BORDER,
            (WIDTH/4 + 5)*2, 150, WIDTH/4, 20,
            hWnd, btn_Check,
            hInst, NULL);

        g_hProgressBar = CreateWindowW(PROGRESS_CLASS,
            L"",
            WS_CHILD | WS_BORDER,
            0, 175, WIDTH, 20,
            hWnd, (HMENU)0,
            hInst, NULL);
        g_hStatusLabel = CreateWindowW(WC_STATIC,
            L"",
            WS_CHILD | WS_BORDER,
            0, 175, WIDTH, 20,
            hWnd, (HMENU)0,
            hInst, NULL);

        g_hLog = CreateWindowW(WC_LISTVIEW,
            L"",
            WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT,
            0, 200, WIDTH, HEIGHT - 200,
            hWnd, (HMENU)0,
            hInst, NULL);
        LVCOLUMNW lvColumn;

        ZeroMemory(&lvColumn, sizeof(lvColumn));
        lvColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        lvColumn.fmt = LVCFMT_LEFT;
        lvColumn.iSubItem = 0;
        lvColumn.pszText = (wchar_t*)L"Банк";
        lvColumn.cchTextMax = wcslen(lvColumn.pszText);
        lvColumn.cx = WIDTH - WIDTH/6;
        ListView_InsertColumn(g_hLog, 0, &lvColumn);

        ZeroMemory(&lvColumn, sizeof(lvColumn));
        lvColumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        lvColumn.fmt = LVCFMT_LEFT;
        lvColumn.iSubItem = 1;
        lvColumn.pszText = (wchar_t*)L"Статус";
        lvColumn.cchTextMax = wcslen(lvColumn.pszText);
        lvColumn.cx = WIDTH/6;
        ListView_InsertColumn(g_hLog, 1, &lvColumn);

        //Check CroReader exe path in registry
        HKEY hKey;
        RegCreateKeyW(HKEY_CURRENT_USER, L"SOFTWARE\\AutoReader", &hKey);

        DWORD dwSize = sizeof(g_szCroReader);
        LSTATUS keyStatus = RegGetValueW(hKey,
            L"", L"CroReaderExe",
            RRF_RT_REG_SZ, NULL,
            g_szCroReader, &dwSize);
        if (keyStatus == ERROR_SUCCESS)
        {
            SetWindowTextW(_hExePath, g_szCroReader);
        }

        RegCloseKey(hKey);
    }
    else if (uMsg == WM_DESTROY)
    {
        PostQuitMessage(0);
    }
    else if (uMsg == WM_COMMAND)
    {
        HMENU button = (HMENU)wParam;
        if (button == btn_SetExePath)
        {
            OPENFILENAMEW ofn;

            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hWnd;
            ofn.lpstrFile = g_szCroReader;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = L"CroReader.exe\0*.exe\0";
            ofn.nFilterIndex = 0;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileNameW(&ofn) == TRUE)
            {
                HKEY hKey;
                DWORD dwSize = (wcslen(g_szCroReader)+1) * 2;

                RegOpenKeyW(HKEY_CURRENT_USER, L"SOFTWARE\\AutoReader", &hKey);
                RegSetValueExW(hKey, L"CroReaderExe",
                    0, REG_SZ,
                    (BYTE*)g_szCroReader, dwSize);
                RegCloseKey(hKey);

                SetWindowTextW(_hExePath, g_szCroReader);
            }
        }
        else if (button == btn_SetPath)
        {
            OPENFILENAMEW ofn;

            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hWnd;
            ofn.lpstrFile = g_szPath;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = L"Cronos Bank\0*.dat\0";
            ofn.nFilterIndex = 0;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileNameW(&ofn) == TRUE)
            {
                SetWindowTextW(_hPath, g_szPath);
            }
        }
        else if (button == btn_Run)
        {
            GetWindowTextW(_hPath, g_szPath, MAX_PATH);
            GetWindowTextW(_hOutPath, g_szOutPath, MAX_PATH);

            wchar_t szNumThreads[16] = {0};
            GetWindowTextW(_hThreads, szNumThreads, 16);
            g_uMaxThreads = _wtoi(szNumThreads);
            if (g_uMaxThreads < 1) g_uMaxThreads = 1;

            //CroReader::Init();
            CroReader::SetCroReaderProPath(g_szCroReader);

            if (g_hThread)
            {
                MessageBoxW(g_hWnd, L"Уже запущено",
                                L"Ошибка", MB_ICONHAND);
            }
            else
            {
                g_hThread = CreateThread(NULL, g_uDefStackSize,
                    RunThread, NULL, 0, NULL);
            }
        }
        else if (button == btn_Stop)
        {
            if (g_hThread)
            {
                TerminateThread(g_hThread, 0);
                CloseHandle(g_hThread);
                g_hThread = NULL;
            }

            if(g_pScheduler)
                g_pScheduler->Continue(1);
        }
        else if (button == btn_Check)
        {
            if (g_hThread)
            {
                TerminateThread(g_hThread, 0);
                CloseHandle(g_hThread);
                g_hThread = NULL;
            }
            else
            {
                g_hThread = CreateThread(NULL, g_uDefStackSize,
                    CheckThread, NULL, 0, NULL);
            }
        }
    }
    else
    {
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

void FixHeader(const std::wstring& datPath)
{
    FILE* fDat = _wfopen(datPath.c_str(), L"r+b");
    if (fDat)
    {
        fseek(fDat, 0, SEEK_SET);
        fwrite("CroFile", 7, 1, fDat);
        fclose(fDat);
    }
}

void FindBanks(std::vector<std::wstring>& banks,
            const std::wstring& path)
{
    SetStatus(L"Поиск банков");

    auto [files, dirs] = ListDirectory(path);
    for (auto& file : files)
    {
        std::wstring fullPath = path + L"\\" + file;
        if (file == L"CroBank.dat")
        {
            FixHeader(fullPath);
            banks.push_back(path);
        }
        else if (file == L"CroStru.dat")
            FixHeader(fullPath);
        else if (file == L"CroIndex.dat")
            FixHeader(fullPath);
    }

    for (auto& dir : dirs)
    {
        if (dir != L"Voc")
            FindBanks(banks, path + L"\\" + dir);
    }

    SetStatus();
}

class BankWorker : public Worker
{
public:
    BankWorker(const std::wstring& bank)
    {
        CroReader* cro = CroReader::CreateCroReader(bank);
        cro->SetOutputPath(g_szOutPath);
        SetData(cro);
        SetName(cro->GetBankPath());
    }

    virtual ~BankWorker()
    {
        CroReader* cro = (CroReader*)GetData();
        if (cro)
        {
            cro->Terminate();
            AddFileLog(cro);
            delete cro;

            UpdateProgress();
        }
    }

    void AddFileLog(CroReader* cro)
    {
        FILE* fLog = fopen("log.txt", "a");
        fprintf(fLog, "%s\t%s\n",
                WcharToText(cro->GetBankPath()).c_str(),
                GetExitMessage().c_str());
        fclose(fLog);
    }

    virtual void Fail(const std::string& exc)
    {
        CroReader* cro = (CroReader*)GetData();

        Worker::Fail(exc);
        if (GetStage() >= 1)
            UpdateLog(cro->GetLogItem(), L"Ошибка");
        else AddLog(cro->GetBankPath().c_str(), L"Ошибка");

        FILE* fError = fopen("error.txt", "a");
        fprintf(fError, "%s\t%s\n",
                WcharToText(cro->GetBankPath()).c_str(),
                GetExitMessage().c_str());
        fclose(fError);
    }

    void DoControl() override
    {
        CroReader* cro = (CroReader*)GetData();

        printf("DoControl stage %d\n", GetStage());

        if (GetState() == workstate::INIT)
        {
            Continue(0);
            return;
        }

        if (GetStage() > 0)
        {
            if (cro->HasMessageBox())
            {
                std::wstring text = cro->GetMessageBoxText();
                if (!text.empty() && !cro->IsBankLoaded())
                {
                    cro->WriteError(text);
                    Fail("CroReader Error");
                    return;
                }
                else if (text == L"Экспорт завершен")
                {
                    Continue(3);
                    return;
                }
                else cro->CloseMessageBox();
            }
        }

        switch(GetStage())
        {
        case 1:
            Wait(WaitObject(
                [](void* data) {
                    return !((CroReader*)data)->WaitAppIdle(2000);
                }, cro, 2000)
            );
            break;
        case 2:
            if (!cro->HasExport())
                Wait(WaitObject(1000));
            break;
        case 3:
            UpdateLog(cro->GetLogItem(), (std::to_wstring(
                cro->GetExportProgress()) + L"%").c_str());
            Wait(WaitObject(
                [](void* data) {
                    CroReader* _cro = (CroReader*)data;
                    return !_cro->HasExport();
                }, cro, 100)
            );
            break;
        }
    }

    void DoWork() override
    {
        CroReader* cro = (CroReader*)GetData();

        printf("DoWork stage %d\n", GetStage());

        switch(GetStage())
        {
        case 0:
            cro->StartCroReader();
            Continue(1);
            break;
        case 1:
            cro->SetupCroReader();
            cro->LoadBankInfo();
            cro->SetLogItem(AddLog(cro->GetBankName().c_str(),
                                    L"Запуск"));
            if (cro->OutputPathExists())
            {
                UpdateLog(cro->GetLogItem(), L"Есть");
                Finish();
            }
            else Continue(2);
            break;
        case 2:
            UpdateLog(cro->GetLogItem(), L"Экспорт");
            if (cro->HasExport())
                Continue(3);
            else cro->StartExport();
            break;
        case 3:
            UpdateLog(cro->GetLogItem(), L"OK");
            cro->MoveFiles();
            Finish();
            break;
        }
    }
};

Worker* CreateBankWorker(const std::wstring& bank)
{
    return dynamic_cast<Worker*>(new BankWorker(bank));
}

DWORD WINAPI RunThread(LPVOID)
{
    FindBanks(g_Banks, g_szPath);
    std::sort(g_Banks.begin(), g_Banks.end(),
        [](const std::wstring& a, const std::wstring& b) {
            return GetDirectorySize(a) < GetDirectorySize(b);
        }
    );

    if (g_pScheduler)
        delete g_pScheduler;

    g_pScheduler = new Scheduler(
        [](void* data) -> Worker* {
            auto* pBanks = (std::vector<std::wstring>*)data;
            if (pBanks->empty())
                return NULL;

            std::wstring bank = pBanks->front();
            pBanks->erase(pBanks->begin());

            return CreateBankWorker(bank);
        }, g_uMaxThreads
    );

    g_pScheduler->SetData(&g_Banks);

    StartProgress(0, g_Banks.size());
    g_pScheduler->Start();
    g_pScheduler->JoinThread();
    FinishProgress();

    delete g_pScheduler;
    g_pScheduler = NULL;

    g_hThread = NULL;
    return 0;
}

DWORD WINAPI CheckThread(LPVOID)
{

    g_hThread = NULL;
    return 0;
}
