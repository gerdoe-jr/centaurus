#include "centaurus_export.h"
#include "croexception.h"
#include "crofile.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
namespace sc = boost::system;

CCentaurusExport::CCentaurusExport(ICentaurusBank* bank,
    const std::wstring& path)
    : m_pBank(bank), m_ExportPath(path)
{
}

void CCentaurusExport::PrepareDirs()
{
    fs::create_directories(m_ExportPath);
}

void CCentaurusExport::Run()
{
    AcquireBank(m_pBank);
    PrepareDirs();

    ExportCroFile(m_pBank->File(CroStru));
}

std::wstring CCentaurusExport::GetFileName(CroFile* file)
{
    std::wstring filePath = file->GetPath();
    std::size_t fileNamePos = filePath.find_last_of(L'\\');
    return fileNamePos != std::wstring::npos
        ? filePath.substr(fileNamePos + 1)
        : filePath;
}

void CCentaurusExport::ExportCroFile(CroFile* file)
{
    std::wstring outPath = m_ExportPath + L"\\" + GetFileName(file);
    fs::create_directories(outPath);

    const CronosABI* abi = file->ABI();
    file->Reset();

    centaurus_size tableLimit = centaurus->RequestTableLimit();
    file->SetTableLimits((cronos_size)tableLimit);

    cronos_id tad_table_id = 1;
    while (!file->IsEndOfEntries())
    {
        cronos_idx tad_count = file->OptimalEntryCount();
        CroEntryTable* tad = AcquireTable<CroEntryTable>(
            file->LoadEntryTable(tad_table_id, tad_count));
        if (tad->IsEmpty())
        {
            ReleaseTable(tad);
            break;
        }

        cronos_id dat_table_id = tad_table_id;
        while (dat_table_id < tad_table_id + tad_count)
        {
            cronos_idx dat_count = file
                ->OptimalRecordCount(*tad, tad->IdStart());
            CroRecordTable* dat = AcquireTable<CroRecordTable>(
                file->LoadRecordTable(*tad, dat_table_id, dat_count));

            printf("tad_count %" FCroIdx " dat_count %" FCroIdx "\n",
                tad_count, dat_count);

            if (dat->IsEmpty())
            {
                ReleaseTable(dat);
                break;
            }

            printf("IdStart %" FCroId " IdEnd %" FCroId "\n", dat->IdStart(), dat->IdEnd());
            for (cronos_id id = dat->IdStart(); id != dat->IdEnd(); id++)
            {
                CroEntry entry = tad->GetEntry(id);
                if (!entry.IsActive()) continue;

                printf("%" FCroId " entry offset %" FCroOff "\n", id, entry.EntryOffset());
                try {
                    /*CroBlock block = dat->FirstBlock(id);
                    printf("BLOCK %" FCroOff " NEXT %" FCroOff "\n",
                        block.GetStartOffset(), block.BlockNext());*/
                    CroBuffer record = GetRecord(file, entry, dat);
                    centaurus->LogBuffer(record, 1251);
                }
                catch (CroException& ce) {
                    fprintf(stderr, "%" FCroId " record exception: %s\n",
                        entry.Id(), ce.what());
                }
                //CroBuffer record = GetRecord(file, entry, dat);
                //centaurus->LogBuffer(record, 1251);

                /*wchar_t szFileName[MAX_PATH] = { 0 };
                swprintf_s(szFileName, L"%s\\%d.bin", outPath.c_str(),
                    entry.Id());
                FILE* fRec = _wfopen(szFileName, L"wb");
                fwrite(record.GetData(), record.GetSize(), 1, fRec);
                fclose(fRec);*/

                UpdateProgress(100.0f * (float)id
                    / (float)file->EntryCountFileSize());
            }

            dat_table_id += dat->GetEntryCount();
            ReleaseTable(dat);
        }

        tad_table_id += tad->GetEntryCount();
        ReleaseTable(tad);
    }
}

CroBuffer CCentaurusExport::GetRecord(CroFile* file, CroEntry& entry, CroRecordTable* dat)
{
    CroBuffer record;
    const CronosABI* abi = file->ABI();

    if (!entry.HasBlock())
    {
        cronos_rel off = dat->DataOffset(entry.EntryOffset());
        record.Write(dat->Data(off), entry.EntrySize());

        return record;
    }

    // здесь нужно загрузить несколько таблиц
    CroBlock block = dat->FirstBlock(entry.Id());
    cronos_size recordSize = block.BlockSize();

    printf("DAT %" FCroOff "<->%" FCroOff "\n", dat->GetStartOffset(),
        dat->GetEndOffset());
    printf("record offset %" FCroOff "\n", block.RecordOffset());

    cronos_rel dataOff = block.RecordOffset();
    cronos_size dataSize = entry.EntrySize()
        - abi->Size(cronos_first_block_hdr);
    if (!dat->IsValidOffset(dat->FileOffset(dataOff)))
    {
        throw std::runtime_error("first block invalid offset "
            + std::to_string(dat->FileOffset(dataOff)));
    }

    record.Write(dat->Data(dataOff), dataSize);
    recordSize -= dataSize;

    while (dat->NextBlock(block))
    {
        dataOff = block.RecordOffset();
        dataSize = std::min(recordSize, file->GetDefaultBlockSize());
        if (!dat->IsValidOffset(dat->FileOffset(dataOff)))
        {
            throw std::runtime_error("next block invalid offset "
                + std::to_string(dat->FileOffset(dataOff)));
        }

        record.Write(dat->Data(dataOff), dataSize);
        recordSize -= dataSize;
        if (!recordSize) break;
    }

    if (file->IsEncrypted())
    {
        file->Decrypt(record.GetData(), record.GetSize(), entry.Id());
    }

    return record;
}
