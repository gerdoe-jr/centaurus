﻿#include "centaurus_export.h"
#include "croexception.h"
#include "crofile.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
namespace sc = boost::system;

CentaurusExport::CentaurusExport(ICentaurusBank* bank,
    const std::wstring& path)
    : m_pBank(bank), m_ExportPath(path)
{
}

void CentaurusExport::PrepareDirs()
{
    fs::create_directories(m_ExportPath);
}

void CentaurusExport::Run()
{
    AcquireBank(m_pBank);
    PrepareDirs();

    ExportCroFile(m_pBank->File(CroStru));
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

        cronos_id dat_start_id = tad->IdStart();
        while (dat_start_id < tad->IdEnd())
        {
            cronos_idx dat_optimal = file
                ->OptimalRecordCount(tad, dat_start_id);
            CroRecordTable* dat = AcquireTable<CroRecordTable>(
                file->LoadRecordTable(tad, dat_start_id, dat_optimal));

            if (dat->IsEmpty())
            {
                ReleaseTable(dat);
                break;
            }

            for (cronos_id id = dat->IdStart(); id != dat->IdEnd(); id++)
            {
                CroEntry entry = tad->GetEntry(id);
                if (!entry.IsActive()) continue;

                printf("%" FCroId " entry offset %" FCroOff "\n", id, entry.EntryOffset());
                try {
                    ExportRecord exp = ReadExportRecord(file, entry);
                    CroBuffer record = ReadFileRecord(file, exp);
                    centaurus->LogBuffer(record, 1251);
                }
                catch (CroException& ce) {
                    fprintf(stderr, "%" FCroId " record exception: %s\n",
                        entry.Id(), ce.what());
                }

                UpdateProgress(100.0f * (float)id
                    / (float)file->EntryCountFileSize());
            }

            dat_start_id = dat->IdEnd();
            ReleaseTable(dat);
        }

        tad_start_id = tad->IdEnd();
        ReleaseTable(tad);
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
    cronos_size dataSize = entry.EntrySize() - block.GetSize();
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

CroBuffer CentaurusExport::GetRecord(CroFile* file, CroEntry& entry, CroRecordTable* dat)
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
