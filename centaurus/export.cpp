#include "export.h"
#include "api.h"
#include "win32util.h"
#include "croexception.h"
#include <crofile.h>
#include <crostru.h>

#include <json_file.h>
#include <win32util.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/trim.hpp>
namespace fs = boost::filesystem;
namespace sc = boost::system;

/* CentaurusExport*/

CentaurusExport::CentaurusExport()
    : CronosAPI("CentaurusExport")
{
    m_ExportFormat = ExportCSV;
}

CentaurusExport::~CentaurusExport()
{
}

std::wstring CentaurusExport::GetFileName(CroFile* file)
{
    std::wstring filePath = file->GetPath();
    std::size_t fileNamePos = filePath.find_last_of(L'\\');
    return fileNamePos != std::wstring::npos
        ? filePath.substr(fileNamePos + 1)
        : filePath;
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

void CentaurusExport::SaveExportRecord(CroBuffer& record, uint32_t id)
{
    const char record_sep = 0x1E;
    CroStream stream = CroStream(record);

    uint8_t baseIndex = stream.Read<uint8_t>();
    if (!m_pBank->IsValidBase(baseIndex))
        return;

    auto& base = m_pBank->Base(baseIndex);
    auto& out = m_Export[baseIndex];

    char* data = (char*)record.GetData() + 1;
    char* cursor = data;

    unsigned i;
    CroFieldIter field = base.StartField();
    
    out->Write(std::to_string(id));
    for (i = 1; i < record.GetSize(); i++)
    {
        if (data[i] == record_sep || i == record.GetSize() - 1)
        {
            size_t length = (ptrdiff_t)&data[i] - (ptrdiff_t)cursor;
            std::string value = Bank()->GetString(
                (const uint8_t*)cursor, length);
            out->Write(value);
            
            cursor = &data[i+1];
            if (++field == base.EndField())
                break;
        }
    }

    for (; field != base.EndField(); field++)
        out->Write("");
}

void CentaurusExport::RunTask()
{
    AcquireBank(m_pBank);
    PrepareDirs();

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
        for (unsigned i = 0; i <= m_pBank->BaseEnd(); i++)
        {
            if (!m_pBank->IsValidBase(i)) continue;
            auto& base = m_pBank->Base(i);

            json baseFields = json::array();
            for (auto it = base.StartField(); it != base.EndField(); it++)
            {
                baseFields.push_back({
                    {"name", it->GetName()},
                    {"type", it->GetType()},
                    {"flags", it->GetFlags()}
                });
            }
            
            // Open export
            std::wstring exportName = L"base" + std::to_wstring(i) + L".csv";
            std::wstring exportPath = JoinFilePath(m_ExportPath, exportName);

            json bankBase = {
                {"name", base.GetName()},
                {"fields", baseFields},
                //{"export", WcharToText(exportName)}
            };

            json baseExport = json::object();
            m_Export[i] = std::make_unique<ExportBuffer>(
                ExportCSV, base.FieldCount());
            auto& _export = m_Export[i];

            try {
                auto croBank = Bank()->File(CROFILE_BANK);
                auto defSize = croBank->GetDefaultBlockSize();

                _export->SetExportFilePath(exportPath);
                _export->ReserveSize(defSize * 4);
                _export->Flush();

                baseExport["file"] = WcharToText(exportName);
                baseExport["format"] = (int)ExportCSV;
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

        // Do export
        json bankExport = json::object();
        try {
            Export();

            bankExport["status"] = true;
        } catch (CroException& ce) {
            centaurus->OnException(ce);
            bankExport["status"] = false;
            bankExport["error"] = {
                {"exception", "CroException" },
                {"what", ce.what()},
                {"file", WcharToText(ce.File()->GetPath())}
            };
        } catch (const std::exception& e) {
            centaurus->OnException(e);
            bankExport["status"] = false;
            bankExport["error"] = {
                {"exception", "std::exception" },
                {"what", e.what()}
            };
        }

        // Close export
        m_BankJson["export"] = bankExport;
        m_Export.clear();
    } catch (CroException& ce) {
        centaurus->OnException(ce);
        m_BankJson["error"] = {
            {"exception", "CroException" },
            {"what", ce.what()},
            {"file", WcharToText(ce.File()->GetPath())}
        };
    } catch (const std::exception& e) {
        centaurus->OnException(e);
        m_BankJson["error"] = {
            {"exception", "std::exception" },
            {"what", e.what()}
        };
    }

    m_BankJson["status"] = !m_BankJson.contains("error");
    SyncBankJson();
}

void CentaurusExport::Release()
{
    CentaurusTask::Release();

    m_Export.clear();
}

void CentaurusExport::Export()
{
    if (m_Export.empty())
    {
        throw std::runtime_error("CentaurusExport bank is not loaded");
    }

    auto file = SetLoaderFile(CROFILE_BANK);
    cronos_size defSize = file->GetDefaultBlockSize();
    uint32_t key = file->GetSecretKey(file->GetSecret());

    
    file->SetupCrypt(key, m_pBank->BankSerial());

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

        for (cronos_id id = bank->IdStart(); id != bank->IdEnd(); id++)
        {
            if (!bank->HasRecord(id)) continue;
            CroBuffer data = bank->LoadRecord(id);

            SaveExportRecord(data, id);

            OnExportRecord(data, id);
        }

        map_id = bank->IdEnd();
        ReleaseMap();
    } while (map_id < file->IdEntryEnd());

    FlushBuffers();
    centaurus->UpdateBankExportIndex(
        m_pBank->BankId(), m_ExportPath);
}

void CentaurusExport::ExportHeaders()
{
    sc::error_code ec;
    std::wstring headerPath = m_ExportPath + L"\\include";

    fs::create_directory(headerPath, ec);
    if (ec) throw std::runtime_error("ExportHeaders !create_directory");

    for (unsigned i = 0; i < CROFILE_COUNT; i++)
    {
        CroFile* file = m_pBank->File((crobank_file)i);
        if (!file) continue;

        FILE* fHdr = NULL;
        switch (i)
        {
        case CROFILE_STRU:
            _wfopen_s(&fHdr, (headerPath + L"\\CroStru.h").c_str(), L"w");
            break;
        case CROFILE_BANK:
            _wfopen_s(&fHdr, (headerPath + L"\\CroBank.h").c_str(), L"w");
            break;
        case CROFILE_INDEX:
            _wfopen_s(&fHdr, (headerPath + L"\\CroIndex.h").c_str(), L"w");
            break;
        }

        if (!fHdr) throw std::runtime_error("ExportHeaders !fHdr");
        centaurus->ExportABIHeader(file->ABI(), fHdr);
        fclose(fHdr);
    }
}

centaurus_size CentaurusExport::GetMemoryUsage()
{
    centaurus_size totalBuffers = 0;
    for (const auto& [_, _export] : m_Export)
        totalBuffers += _export->GetSize();

    return CentaurusTask::GetMemoryUsage() + totalBuffers;
}

void CentaurusExport::OnExportRecord(CroBuffer& record, uint32_t id)
{
    for (auto& [idx, _export] : m_Export)
    {
        if (_export->GetSize() >= m_ExportLimit)
        {
            _export->Flush();
        }
    }
}

void CentaurusExport::FlushBuffers()
{
    for (auto& [_, _export] : m_Export)
        _export->Flush();
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
    WriteJSONFile(m_ExportPath + L"\\bank.json", m_BankJson);
}
