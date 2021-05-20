#include "export.h"
#include <win32util.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/trim.hpp>
namespace fs = boost::filesystem;
namespace sc = boost::system;

/* CentaurusExport */

CentaurusExport::CentaurusExport()
    : CronosAPI("CentaurusExport")
{
    m_ExportFormat = ExportCSV;
}

CentaurusExport::~CentaurusExport()
{
}

const std::wstring& CentaurusExport::ExportPath() const
{
    return m_ExportPath;
}

ExportFormat CentaurusExport::GetExportFormat() const
{
    return m_ExportFormat;
}

void CentaurusExport::SetExportFormat(ExportFormat fmt)
{
    m_ExportFormat = fmt;
}

void CentaurusExport::SyncBankJson()
{
    WriteJSONFile(JoinFilePath(m_ExportPath, L"bank.json"), m_BankJson);
}

void CentaurusExport::PrepareDirs()
{
    std::wstring dirName = m_pBank->BankName();
    std::transform(dirName.begin(), dirName.end(), dirName.begin(),
        [](wchar_t sym) {
            const std::wstring prohibit = L"\\/:?\"<>|";
            return prohibit.find(sym) == std::wstring::npos ? sym : L'_';
        }
    );

    boost::algorithm::trim(dirName);
    m_ExportPath = JoinFilePath(centaurus->GetExportPath(), dirName);
    fs::create_directory(m_ExportPath);
}

void CentaurusExport::RunTask()
{
    AcquireBank(m_pBank);
    PrepareDirs();

    switch (m_ExportFormat)
    {
    case ExportCSV:
        m_Export = std::make_unique<CroExportCSV>(m_pBank);
        break;
    case ExportJSON:
    default:
        m_Export = std::make_unique<CroExportRaw>(m_pBank);
    }

    m_BankJson = {
        {"id", m_pBank->BankId()},
        {"name", WcharToText(m_pBank->BankName())},
        {"serial", m_pBank->BankSerial()},
        {"password", WcharToText(m_pBank->BankSysPass())},
        {"version", m_pBank->BankVersion()},
        {"bases", json::array()}
    };

    auto& bankBases = m_BankJson["bases"];
    SyncBankJson();

    try {
        cronos_size baseLimit = m_ExportLimit / m_pBank->BaseCount();

        for (auto it = m_pBank->StartBase(); it != m_pBank->EndBase(); it++)
        {
            json baseFields = json::array();
            for (auto ij = it->StartField(); ij != it->EndField(); ij++)
            {
                baseFields.push_back({
                    {"name", ij->GetName()},
                    {"type", ij->GetType()},
                    {"flags", ij->GetFlags()},
                    {"index", ij->m_DataIndex},
                    {"length", ij->m_DataLength},
                });
            }

            std::wstring exportExt;
            switch (m_ExportFormat)
            {
            case ExportCSV: exportExt = L".csv"; break;
            case ExportJSON: exportExt = L".json"; break;
            }

            std::wstring exportName = L"base"
                + std::to_wstring(it->m_BaseIndex) + exportExt;
            std::wstring exportPath = JoinFilePath(m_ExportPath, exportName);
            
            if (fs::exists(exportPath))
            {
                fs::remove(exportPath);
            }

            json bankBase = {
                {"name", it->GetName()},
                {"index", it->m_BaseIndex},
                {"mnemocode", it->m_Mnemocode},
                {"fields", baseFields},
            };

            json baseExport = json::object();
            try {
                CroSync* baseOut = CreateSyncFile(exportPath, baseLimit);

                m_Export->SetIdentOutput(it->m_BaseIndex, baseOut);
                Log("%03u\t\"%s\" -> %s\n", it->m_BaseIndex, WcharToTerm(
                    TextToWchar(it->GetName())).c_str(),
                    WcharToTerm(exportPath).c_str()
                );

                baseExport["file"] = WcharToText(exportName);
                baseExport["format"] = (unsigned)m_Export->GetExportFormat();
                baseExport["status"] = true;
            }
            catch (const std::exception& e) {
                centaurus->OnException(e);
                baseExport["status"] = false;
                baseExport["error"] = "failed to open export file";
            }

            bankBase["export"] = baseExport;
            bankBases.push_back(bankBase);
        }

        SyncBankJson();

        // !! EXPORT !!
        json bankExport = json::object();
        try {
            Export();

            bankExport["status"] = true;
        }
        catch (CroException& ce) {
            centaurus->OnException(ce);
            bankExport["status"] = false;
            bankExport["error"] = {
                {"exception", "CroException" },
                {"what", ce.what()},
                {"file", WcharToText(ce.File()->GetPath())}
            };
        }
        catch (const std::exception& e) {
            centaurus->OnException(e);
            bankExport["status"] = false;
            bankExport["error"] = {
                {"exception", "std::exception" },
                {"what", e.what()}
            };
        }

        // !! FINISH EXPORT !!
        m_BankJson["export"] = bankExport;
        ReleaseBuffers();
    }
    catch (CroException& ce) {
        centaurus->OnException(ce);
        m_BankJson["error"] = {
            {"exception", "CroException" },
            {"what", ce.what()},
            {"file", WcharToText(ce.File()->GetPath())}
        };
    }
    catch (const std::exception& e) {
        centaurus->OnException(e);
        m_BankJson["error"] = {
            {"exception", "std::exception" },
            {"what", e.what()}
        };
    }

    m_BankJson["status"] = !m_BankJson.contains("error");
    SyncBankJson();

    /*{
        auto file = SetLoaderFile(CROFILE_BANK);
        CroExportList list = CroExportList(m_pBank,
            file->EntryCountFileSize());

        auto bank = GetRecordMap(1, file->EntryCountFileSize());
        list.ReadMap(bank);

        for (auto it = list.ExportStart(); it != list.ExportEnd(); it++)
        {
            Log("RECORD %" FCroId "\n", it->first);

            for (auto& value : it->second)
                LogBuffer(value, 866);
        }

        list.Reset();
    }*/
}

void CentaurusExport::Release()
{
    m_Export = NULL;

    CronosAPI::Release();
}

void CentaurusExport::Export()
{
    if (!m_Export)
    {
        throw std::runtime_error("CroExport not loaded");
    }

    auto file = SetLoaderFile(CROFILE_BANK);
    if (file->IsEncrypted())
    {
        uint32_t key = file->GetSecretKey(file->GetSecret());
        file->SetupCrypt(key, m_pBank->BankSerial());
    }

    cronos_size defSize = file->GetDefaultBlockSize();
    CroReader* reader = m_Export->GetReader();

    cronos_id map_id = 1;
    cronos_idx burst = m_BlockLimit / defSize;
    do {
        burst = std::min((cronos_id)(m_BlockLimit / defSize),
            file->EntryCountFileSize());
        auto bank = GetRecordMap(map_id, burst);
        if (bank->IsEmpty())
        {
            ReleaseMap();
            break;
        }

        try {
            m_Export->Export(bank);

            map_id = bank->IdEnd();
        }
        catch (const std::exception& e) {
            Error("export exception: %s\n", e.what());
        }

        ReleaseMap();
        FlushBuffers();
    } while (map_id < file->IdEntryEnd());

    FlushBuffers();
    centaurus->UpdateBankExportIndex(
        m_pBank->BankId(), m_ExportPath);
}
