#include "export.h"
#include "api.h"
#include "win32util.h"
#include "croexception.h"
#include "crofile.h"
#include "croattr.h"

#include <json_file.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/trim.hpp>
namespace fs = boost::filesystem;
namespace sc = boost::system;

#ifdef min
#undef min
#endif

#include <algorithm>

#ifndef WIN32
#include <win32util.h>

static FILE* _wfopen(const wchar_t* path, const wchar_t* mode)
{
    return fopen(WcharToText(path).c_str(), WcharToText(mode).c_str());
}

static int _wfopen_s(FILE** fpFile, const wchar_t* path, const wchar_t* mode)
{
    *fpFile = fopen(WcharToText(path).c_str(), WcharToText(mode).c_str());
    return !!(*fpFile);
}

#endif

/* ExportBuffer */

ExportBuffer::ExportBuffer()
{
    m_Format = ExportCSV;
    m_uIndex = 0;
    m_uColumns = 0;
    m_TextOffset = 0;
    m_Reserve = 0;
    m_uRecordCount = 0;
}

ExportBuffer::ExportBuffer(ExportFormat fmt, unsigned columns)
{
    m_Format = fmt;
    m_uIndex = 0;
    m_uColumns = columns;
    m_TextOffset = 0;
    m_Reserve = 0;
    m_uRecordCount = 0;
}

ExportBuffer::~ExportBuffer()
{
    Flush();
}

void ExportBuffer::SetExportFilePath(const std::wstring& path)
{
    m_ExportFile = path;
}

void ExportBuffer::ReserveSize(cronos_size size)
{
    m_Reserve = size;
}

void ExportBuffer::WriteCSV(const std::string& column)
{
    cronos_size columnSize = column.size();
    cronos_size bufferSize = GetSize();

    if (!bufferSize) Alloc(m_Reserve);
    else if (columnSize + 16 + m_TextOffset >= bufferSize)
    {
        cronos_size newSize = bufferSize + (columnSize + 16 + m_TextOffset);
        Alloc(m_Reserve * (newSize / m_Reserve)
            + (newSize % m_Reserve ? m_Reserve : 0));
    }

    char* pColumn = (char*)GetData() + m_TextOffset;
    char* pCursor = pColumn;

    *pCursor++ = '"';
    for (const auto& c : column)
    {
        if (c == '"') *pCursor++ = '"';
        *pCursor++ = c;
    }
    *pCursor++ = '"';

    if (++m_uIndex == m_uColumns)
    {
        m_uIndex = 0;
        m_uRecordCount++;
        *pCursor++ = '\r';
        *pCursor++ = '\n';
    }
    else *pCursor++ = ',';

    m_TextOffset += (ptrdiff_t)pCursor - (ptrdiff_t)pColumn;
}

void ExportBuffer::WriteJSON(const std::string& column)
{
}

void ExportBuffer::Write(const std::string& column)
{
    if (m_Format == ExportCSV) WriteCSV(column);
    else WriteJSON(column);
}

void ExportBuffer::Flush()
{
    if (IsEmpty()) return;

    unsigned count = m_uRecordCount;
    cronos_size size = GetSize();

    FILE* fOut = _wfopen(m_ExportFile.c_str(), L"ab");
    if (!fOut) throw std::runtime_error("ExportBuffer flush failed");
    fwrite(GetData(), m_TextOffset, 1, fOut);
    fclose(fOut);

    Free();
    m_uIndex = 0;
    m_TextOffset = 0;
    m_uRecordCount = 0;

    printf("[ExportBuffer] %s of %u records -> %s\n", _centaurus
        ->SizeToString(size).c_str(), count, WcharToAnsi(
            m_ExportFile, 866).c_str());
}

/* CentaurusExport*/

CentaurusExport::CentaurusExport()
    : CronosAPI("CentaurusExport")
{
    m_ExportFormat = ExportCSV;
    m_TableLimit = 512 * 1024 * 1024;
    m_BaseLimit = m_TableLimit;
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
    m_ExportPath = centaurus->GetExportPath() + L"\\" + dirName;
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

    unsigned i, field = 0;
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
            if (++field == base.FieldCount() - 1)
                break;
        }
    }

    for (; field < base.FieldCount() - 1; field++)
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
            for (unsigned j = 0; j < base.FieldCount(); j++)
            {
                try {
                    auto& field = base.Field(j);
                    baseFields.push_back({
                        {"name", field.GetName()},
                        {"type", field.GetType()},
                        {"flags", field.GetFlags()}
                    });
                }
                catch (const std::exception& e) {
                    _CrtDbgBreak();
                }
            }

            // Open export
            std::wstring exportName = L"base" + std::to_wstring(i) + L".csv";
            std::wstring exportPath = m_ExportPath + L"\\" + exportName;

            json bankBase = {
                {"name", base.GetName()},
                {"fields", baseFields},
                //{"export", WcharToText(exportName)}
            };

            json baseExport = json::object();
            
            m_Export[i] = std::make_unique<ExportBuffer>
                (ExportCSV, base.FieldCount());
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
        throw std::runtime_error("CentaurusExport bank is not loaded");

    auto file = SetLoaderFile(CROFILE_BANK);
    cronos_size defSize = file->GetDefaultBlockSize();
    
    uint32_t key = file->GetSecretKey(file->GetSecret());
    file->SetupCrypt(key, m_pBank->BankSerial());

    cronos_id map_id = 1;
    cronos_idx burst = m_TableLimit / defSize;
    do {
        burst = std::min((cronos_id)(m_TableLimit / defSize),
            file->EntryCountFileSize());
        auto bank = GetRecordMap(map_id, burst);
        if (bank->IsEmpty())
        {
            ReleaseMap();
            break;
        }

        Log("BURST %" FCroIdx "\n", burst);
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
    // BUG BUG BUG

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
        if (_export->GetSize() >= m_BaseLimit)
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
