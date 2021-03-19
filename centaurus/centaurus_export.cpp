#include "centaurus_export.h"
#include "win32util.h"
#include "croexception.h"
#include "crofile.h"
#include "croattr.h"

#include <json_file.h>
#include <boost/filesystem.hpp>
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
}

ExportBuffer::ExportBuffer(ExportFormat fmt, unsigned columns)
{
    m_Format = fmt;
    m_uIndex = 0;
    m_uColumns = columns;
    m_TextOffset = 0;
    m_Reserve = 0;
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

void ExportBuffer::Flush(FILE* fCsv)
{
    fwrite(GetData(), m_TextOffset, 1, fCsv);
    fflush(fCsv);

    Free();
    m_uIndex = 0;
    m_TextOffset = 0;
}

/* CentaurusExport*/

CentaurusExport::CentaurusExport()
    : m_pBank(NULL), m_ExportFormat(ExportCSV)
{
}

CentaurusExport::~CentaurusExport()
{
    for (auto& [_, out] : m_Export)
        if (out.m_fExport) fclose(out.m_fExport);
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
    m_ExportPath = centaurus->GetExportPath() + L"\\" + m_pBank->BankName();
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
    //auto& out = m_Export[baseIndex];
    auto& [_, out] = m_Export[baseIndex];

    char* data = (char*)record.GetData() + 1;
    char* cursor = data;

    unsigned i, field = 0;
    out.Write(std::to_string(id));
    for (i = 1; i < record.GetSize(); i++)
    {
        if (data[i] == record_sep || i == record.GetSize() - 1)
        {
            size_t length = (ptrdiff_t)&data[i] - (ptrdiff_t)cursor;
            std::string value = m_pBank->String(cursor, length);
            out.Write(value);
            
            cursor = &data[i+1];
            if (++field == base.FieldCount() - 1)
                break;
        }
    }

    for (; field < base.FieldCount() - 1; field++)
        out.Write("");
}

void CentaurusExport::RunTask()
{
    AcquireBank(m_pBank);
    PrepareDirs();

    m_BankJson = {
        {"id", m_pBank->BankId()},
        {"name", WcharToText(m_pBank->BankName())},
        {"attributes", json::object()},
        {"bases", json::array()}
    };

    auto& bankAttrs = m_BankJson["attributes"];
    auto& bankBases = m_BankJson["bases"];
    SyncBankJson();

    try {
        // bankAttributes
        for (unsigned i = 0; i < m_pBank->AttrCount(); i++)
        {
            auto& attr = m_pBank->Attr(i);
            auto& data = attr.GetAttr();
            bankAttrs[attr.GetName()] = m_pBank->String(
                (const char*)data.GetData(), data.GetSize());
        }
        
        // bankBases
        for (unsigned i = 0; i != m_pBank->BaseEnd(); i++)
        {
            if (!m_pBank->IsValidBase(i)) continue;
            auto& base = m_pBank->Base(i);

            json baseFields = json::array();
            for (unsigned j = 0; j < base.FieldCount(); j++)
            {
                auto& field = base.Field(j);
                baseFields.push_back({
                    {"name", field.GetName()},
                    {"type", field.GetType()},
                    {"flags", field.GetFlags()}
                });
            }

            // Open export
            std::wstring exportName = L"base" + std::to_wstring(i) + L".csv";
            std::wstring exportPath = m_ExportPath + L"\\" + exportName;

            json bankBase = {
                {"name", base.GetName()},
                {"fields", baseFields},
                //{"export", WcharToText(exportName)}
            };

            FILE* fExport;
            json baseExport = json::object();
            if (!_wfopen_s(&fExport, exportPath.c_str(), L"wb"))
            {
                m_Export[i] = ExportOutput{
                    .m_fExport = fExport,
                    .m_Buffer = ExportBuffer(ExportCSV, base.FieldCount())
                };

                m_Export[i].m_Buffer.ReserveSize(m_pBank
                    ->File(CroBank)->GetDefaultBlockSize() * 4);
                baseExport["file"] = WcharToText(exportName);
                baseExport["format"] = (int)ExportCSV;
                baseExport["status"] = true;
            }
            else
            {
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
            bankExport["status"] = false;
            bankExport["error"] = {
                {"exception", "CroException" },
                {"what", ce.what()},
                {"file", WcharToText(ce.File()->GetPath())}
            };
        } catch (const std::exception& e) {
            bankExport["status"] = false;
            bankExport["error"] = {
                {"exception", "std::exception" },
                {"what", e.what()}
            };
        }

        // Close export
        m_BankJson["export"] = bankExport;
        for (auto& [_, _out] : m_Export)
            fclose(_out.m_fExport);
        m_Export.clear();
    } catch (CroException& ce) {
        m_BankJson["error"] = {
            {"exception", "CroException" },
            {"what", ce.what()},
            {"file", WcharToText(ce.File()->GetPath())}
        };
    } catch (const std::exception& e) {
        m_BankJson["error"] = {
            {"exception", "std::exception" },
            {"what", e.what()}
        };
    }

    m_BankJson["status"] = !m_BankJson.contains("error");
    SyncBankJson();
}

void CentaurusExport::Export()
{
    centaurus_size tableLimit = centaurus->RequestTableLimit();
    CroFile* file = m_pBank->File(CroBank);
    auto* abi = file->ABI();

    file->Reset();
    file->SetTableLimits(tableLimit);

    cronos_id tad_cur = 1;
    while (!file->IsEndOfEntries() && tad_cur <= file->IdEntryEnd())
    {
        cronos_idx tad_optimal = file->OptimalEntryCount();
        CroEntryTable* tad = AcquireTable<CroEntryTable>(
            file->LoadEntryTable(tad_cur, tad_optimal));
        if (tad->IsEmpty())
        {
            ReleaseTable(tad);
            break;
        }

        for (cronos_id id = tad->IdStart(); id != tad->IdEnd(); id++)
        {
            CroEntry entry = tad->GetEntry(id);
            if (!entry.IsActive()) continue;

            // Read record
            CroBlock block;
            CroData data;
            CroBuffer record;

            cronos_off dataOff, nextOff;
            cronos_size dataSize, recordSize;

            if (entry.HasBlock())
            {
                block = CroBlock(true);
                block.InitData(file, entry.Id(), CRONOS_DAT, entry.EntryOffset(),
                    abi->Size(cronos_first_block_hdr));
                file->Read(block, 1, block.GetSize());

                nextOff = block.BlockNext();
                recordSize = block.BlockSize();

                dataOff = block.GetStartOffset() + block.GetSize();
                dataSize = std::min(recordSize,
                    entry.EntrySize() - block.GetSize());
            }
            else
            {
                nextOff = 0;
                recordSize = entry.EntrySize();

                dataOff = entry.EntryOffset();
                dataSize = recordSize;
            }

            data.InitData(file, id, CRONOS_DAT, dataOff, dataSize);
            file->Read(data, 1, dataSize);

            record.Write(data.GetData(), dataSize);
            recordSize -= dataSize;

            while (nextOff != 0 && recordSize > 0)
            {
                block = CroBlock(false);
                block.InitData(file, entry.Id(), CRONOS_DAT, nextOff,
                    abi->Size(cronos_block_hdr));
                file->Read(block, 1, block.GetSize());

                nextOff = block.BlockNext();

                dataOff = block.GetStartOffset() + block.GetSize();
                dataSize = std::min(recordSize, file->GetDefaultBlockSize());
                data.InitData(file, id, CRONOS_DAT, dataOff, dataSize);
                file->Read(data, 1, dataSize);

                record.Write(data.GetData(), dataSize);
                recordSize -= dataSize;
            }

            // Decrypt & parse record
            if (file->IsEncrypted())
            {
                file->Decrypt(record.GetData(), record.GetSize(), id);
            }

            //centaurus->LogBuffer(record);

            // Export to CSV base file
            SaveExportRecord(record, id);
        }

        tad_cur = tad->IdEnd();
        ReleaseTable(tad);

        //Flush
        for (auto& [_, _out] : m_Export)
            _out.m_Buffer.Flush(_out.m_fExport);
    }
}

ExportRecord CentaurusExport::ReadExportRecord(CroFile* file,
    CroEntry& entry)
{
    const CronosABI* abi = file->ABI();
    ExportRecord record = ExportRecord(entry);

    CroBlock block = CroBlock(true);
    block.InitData(file, entry.Id(), CRONOS_DAT, entry.EntryOffset(),
        abi->Size(cronos_first_block_hdr));
    file->Read(block, 1, block.GetSize());

    cronos_off nextOff = block.BlockNext();
    cronos_size recordSize = block.BlockSize();

    cronos_off dataOff = block.GetStartOffset() + block.GetSize();
    cronos_size dataSize = std::min(recordSize,
        entry.EntrySize() - block.GetSize());
    record.AddBlock(dataOff, dataSize);
    recordSize -= dataSize;

    while (nextOff != 0 && recordSize > 0)
    {
        block = CroBlock(false);
        block.InitData(file, entry.Id(), CRONOS_DAT, nextOff,
            abi->Size(cronos_block_hdr));
        file->Read(block, 1, block.GetSize());

        nextOff = block.BlockNext();

        dataOff = block.GetStartOffset() + block.GetSize();
        dataSize = std::min(recordSize, file->GetDefaultBlockSize());
        record.AddBlock(dataOff, dataSize);
        recordSize -= dataSize;
    }

    return record;
}

CroBuffer CentaurusExport::ReadFileRecord(CroFile* file, ExportRecord& record)
{
    CroBuffer buf;
    for (const auto& block : record.m_Blocks)
    {
        CroData data;
        data.InitData(file, record.m_RecordId, CRONOS_DAT,
            block.m_DataPos, block.m_DataSize);
        file->Read(data, 1, data.GetSize());
        buf.Write(data.GetData(), data.GetSize());
    }

    if (file->IsEncrypted())
    {
        file->Decrypt(buf.GetData(), buf.GetSize(), record.m_RecordId);
    }

    return buf;
}

ICentaurusBank* CentaurusExport::TargetBank()
{
    return m_pBank;
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

void CentaurusExport::ReadRecord(CroFile* file, uint32_t id, CroBuffer& out)
{
    CroEntryTable tad = file->LoadEntryTable(id, 1);
    if (tad.IsEmpty()) throw std::runtime_error("no entry table");

    CroEntry entry = tad.GetEntry(id);
    if (!entry.IsActive()) throw std::runtime_error("not active entry");

    ExportRecord record = ReadExportRecord(file, entry);
    out = ReadFileRecord(file, record);
}

void CentaurusExport::SyncBankJson()
{
    WriteJSONFile(m_ExportPath + L"\\bank.json", m_BankJson);
}
