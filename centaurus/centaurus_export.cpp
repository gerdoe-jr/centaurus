#include "centaurus_export.h"
#include "win32util.h"
#include "croexception.h"
#include "crofile.h"
#include "croattr.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
namespace sc = boost::system;

#ifdef min
#undef min
#endif

#include <algorithm>

/* CsvBuffer */

CsvBuffer::CsvBuffer()
{
    m_uIndex = 0;
    m_uColumns = 0;
    m_CsvOffset = 0;
}

CsvBuffer::CsvBuffer(unsigned columns)
{
    m_uIndex = 0;
    m_uColumns = columns;
    m_CsvOffset = 0;
}

void CsvBuffer::Write(const std::string& column)
{
    Alloc(GetSize() + column.size() * 2 + 16);

    char* pColumn = (char*)GetData() + m_CsvOffset;
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

    m_CsvOffset += (ptrdiff_t)pCursor - (ptrdiff_t)pColumn;
}

void CsvBuffer::Flush(FILE* fCsv)
{
    fwrite(GetData(), m_CsvOffset, 1, fCsv);
    fflush(fCsv);

    Free();
    m_uIndex = 0;
    m_CsvOffset = 0;
}

/* CentaurusExport*/

CentaurusExport::CentaurusExport(ICentaurusBank* bank,
    const std::wstring& path)
    : m_pBank(bank), m_ExportPath(path)
{
}

CentaurusExport::~CentaurusExport()
{
    if (!m_Export.empty()) CloseExport();
}

void CentaurusExport::PrepareDirs()
{
    fs::create_directories(m_ExportPath);
}

void CentaurusExport::OpenExport()
{
    std::wstring csvPath = m_ExportPath + L"\\csv";
    fs::create_directories(csvPath);

    for (unsigned i = 0; i <= m_pBank->BaseCount(); i++)
    {
        if (!m_pBank->IsValidBase(i))
            continue;
        
        auto& base = m_pBank->Base(i);
        std::wstring csvFilePath = csvPath + L"\\"
            + AnsiToWchar(base.GetName(), CP_UTF8) + L".csv";

        m_Export[i] = ExportOutput {
            .m_fCsv = _wfopen(csvFilePath.c_str(), L"wb"),
            .m_CsvBuffer = CsvBuffer(base.FieldCount())
        };

        auto& out = m_Export[i];
        for (unsigned j = 0; j < base.FieldCount(); j++)
        {
            auto& field = base.Field(j);
            out.m_CsvBuffer.Write(field.GetName());
        }
        out.m_CsvBuffer.Flush(out.m_fCsv);
    }
}

void CentaurusExport::CloseExport()
{
    for (auto& _export : m_Export)
        fclose(_export.second.m_fCsv);
    m_Export.clear();
}

void CentaurusExport::FlushExport()
{
    for (auto& _export : m_Export)
    {
        _export.second.m_CsvBuffer.Flush(_export.second.m_fCsv);
    }
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

    out.m_CsvBuffer.Write(std::to_string(id));
    
    char* data = (char*)record.GetData() + 1;
    char* cursor = data;

    unsigned i, field = 0;
    for (i = 1; i < record.GetSize(); i++)
    {
        if (data[i] == record_sep || i == record.GetSize() - 1)
        {
            size_t length = (ptrdiff_t)&data[i] - (ptrdiff_t)cursor;
            std::string value = m_pBank->String(cursor, length);
            out.m_CsvBuffer.Write(value);
            
            cursor = &data[i+1];
            if (++field == base.FieldCount() - 1)
                break;
        }
    }

    for (; field < base.FieldCount() - 1; field++)
        out.m_CsvBuffer.Write("");
}

void CentaurusExport::Run()
{
    AcquireBank(m_pBank);
    PrepareDirs();

    m_pBank->ExportStructure(this);

    //ExportCroFile(m_pBank->File(CroStru));
    ExportCroFile(m_pBank->File(CroBank));
    //ExportCroFile(m_pBank->File(CroIndex));
}

std::wstring CentaurusExport::GetFileName(CroFile* file)
{
    std::wstring filePath = file->GetPath();
    std::size_t fileNamePos = filePath.find_last_of(L'\\');
    return fileNamePos != std::wstring::npos
        ? filePath.substr(fileNamePos + 1)
        : filePath;
}

void CentaurusExport::ExportCroFile(CroFile* file)
{
    OpenExport();

    std::wstring outPath = m_ExportPath + L"\\" + GetFileName(file);
    fs::create_directories(outPath);

    const CronosABI* abi = file->ABI();
    file->Reset();

    centaurus_size tableLimit = centaurus->RequestTableLimit();
    file->SetTableLimits((cronos_size)tableLimit);

    cronos_id tad_start_id = 1;
    while (!file->IsEndOfEntries() && tad_start_id < file->IdEntryEnd())
    {
        cronos_idx tad_optimal = file->OptimalEntryCount();

        CroEntryTable* tad = AcquireTable<CroEntryTable>(
            file->LoadEntryTable(tad_start_id, tad_optimal));
        if (tad->IsEmpty())
        {
            ReleaseTable(tad);
            break;
        }

        for (cronos_id id = tad->IdStart(); id != tad->IdEnd(); id++)
        {
            CroEntry entry = tad->GetEntry(id);
            if (!entry.IsActive()) continue;

            try {
                ExportRecord exp = ReadExportRecord(file, entry);
                CroBuffer record = ReadFileRecord(file, exp);
                //centaurus->LogBuffer(record, 1251);
                SaveExportRecord(record, entry.Id());
                //centaurus->LogBuffer(record, 1251);
            }
            catch (CroException& ce) {
                fprintf(stderr, "%" FCroId " cronos exception: %s\n",
                    entry.Id(), ce.what());
            }
            catch (const std::exception& e) {
                fprintf(stderr, "export exception: %s\n", e.what());
            }

            UpdateProgress(100.0f * (float)id
                / (float)file->EntryCountFileSize());
        }

        tad_start_id = tad->IdEnd();
        ReleaseTable(tad);

        FlushExport();
    }

    CloseExport();
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
        record.AddBlock(dataSize, dataOff);
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

void CentaurusExport::ReadRecord(CroFile* file, uint32_t id, CroBuffer& out)
{
    CroEntryTable tad = file->LoadEntryTable(id, 1);
    if (tad.IsEmpty()) throw std::runtime_error("no entry table");

    CroEntry entry = tad.GetEntry(id);
    if (!entry.IsActive()) throw std::runtime_error("not active entry");

    ExportRecord record = ReadExportRecord(file, entry);
    out = ReadFileRecord(file, record);
}
