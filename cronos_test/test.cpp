#include <stdio.h>
#include <stdlib.h>
#include "crofile.h"
#include "croexception.h"
#include "win32util.h"

#ifdef min
#undef min
#undef max
#endif
#include <algorithm>

static std::wstring testBank = 
    L"K:\\Cronos\\TestBanks\\Test3\\Украина - Клиенты moneyveo 2017";
static std::vector<std::string> known_versions;

void scan_directory(const std::wstring& path)
{
    bool logged = false;
    auto [files, dirs] = ListDirectory(path);

    for (auto& file : files)
    {
        if (file.find(L"CroBank.dat") != std::wstring::npos
                || file.find(L"CroStru.dat") != std::wstring::npos)
        {
            if (!logged)
            {
                 printf("%s\n", WcharToAnsi(path).c_str());
                 logged = true;
            }

            printf("\t%s\t", WcharToAnsi(file).c_str());
            CroFile cro = CroFile(path + L"\\" + file.substr(0, 7));
            crofile_status st = cro.Open();
            if (st != CROFILE_OK)
            {
                printf("[%u,\"%s\"]\n", cro.GetStatus(),
                        cro.GetError().c_str());
                continue;
            }

            char szHdrText[32] = {0};
            cronos_abi_num ver = cro.GetABIVersion();
            snprintf(szHdrText, 32, "%02d.%02d", ver.first, ver.second);
            std::string hdrtext = szHdrText;
            printf("%s\n", hdrtext.c_str());
            if (std::find(known_versions.begin(), known_versions.end(),
                    hdrtext) == known_versions.end())
            {
                known_versions.push_back(hdrtext);
            }

            cro.Close();
        }
    }

    for (auto& dir : dirs)
        scan_directory(path + L"\\" + dir);
}

void dump_buffer(const CroBuffer& buf, unsigned codepage = 0)
{
    const char ascii_lup = 201, ascii_rup = 187,
        ascii_lsp = 199, ascii_rsp = 182,
        ascii_lbt = 200, ascii_rbt = 188,
        ascii_up_cross = 209, ascii_bt_cross = 207,
        ascii_cross = 197,
        ascii_v_sp = 179, ascii_h_sp = 196,
        ascii_v_border = 186, ascii_h_border = 205;

    const cronos_size line = 0x10;

    //code page
    if (!codepage) codepage = GetConsoleOutputCP();

    //start
    putc(ascii_lup, stdout);
    for (cronos_rel i = 0; i < 8; i++) putc(ascii_h_border, stdout);
    putc(ascii_up_cross, stdout);
    for (cronos_rel i = 0; i < line * 3 - 1; i++)
        putc(ascii_h_border, stdout);
    putc(ascii_up_cross, stdout);
    for (cronos_rel i = 0; i < line; i++)
        putc(ascii_h_border, stdout);
    putc(ascii_rup, stdout);

    putc('\n', stdout);

    //header
    putc(ascii_v_border, stdout);
    printf(" offset ");
    putc(ascii_v_sp, stdout);
    for (cronos_rel i = 0; i < line; i++)
        printf(i < line - 1 ? "%02x " : "%02x", i & 0xFF);
    putc(ascii_v_sp, stdout);
    switch (codepage)
    {
    case CP_UTF7: printf(" UTF-7  "); break;
    case CP_UTF8: printf(" UTF-8  "); break;
    default: printf(" ANSI CP #%05d ", codepage);
    }
    putc(ascii_v_border, stdout);

    putc('\n', stdout);

    //split
    putc(ascii_lsp, stdout);
    for (cronos_rel i = 0; i < 8; i++)
        putc(ascii_h_sp, stdout);
    putc(ascii_cross, stdout);
    for (cronos_rel i = 0; i < line * 3 - 1; i++)
        putc(ascii_h_sp, stdout);
    putc(ascii_cross, stdout);
    for (cronos_rel i = 0; i < line; i++)
        putc(ascii_h_sp, stdout);
    putc(ascii_rsp, stdout);

    putc('\n', stdout);

    //hex dump
    for (cronos_size off = 0; off < buf.GetSize(); off += line)
    {
        cronos_size len = std::min(buf.GetSize() - off, line);

        putc(ascii_v_border, stdout);
        printf("%08" FCroOff, off);
        if (len) putc(ascii_v_sp, stdout);
        else break;

        for (cronos_rel i = 0; i < line; i++)
        {
            if (i < len)
            {
                printf(i < line - 1 ? "%02X " : "%02X",
                    buf.GetData()[off + i] & 0xFF);
            }
            else printf(i < line - 1 ? "   " : "  ");
        }

        putc(ascii_v_sp, stdout);

        SetConsoleOutputCP(codepage);
        for (cronos_rel i = 0; i < line; i++)
        {
            if (i < len)
            {
                uint8_t byte = buf.GetData()[off + i];
                putc(byte >= 0x20 ? (char)byte : '.', stdout);
            }
            else putc(' ', stdout);
        }
        SetConsoleOutputCP(866);
        putc(ascii_v_border, stdout);

        putc('\n', stdout);
    }

    //end
    putc(ascii_lbt, stdout);

    for (cronos_rel i = 0; i < 8; i++)
        putc(ascii_h_border, stdout);
    putc(ascii_bt_cross, stdout);

    for (cronos_rel i = 0; i < line * 3 - 1; i++)
        putc(ascii_h_border, stdout);

    putc(ascii_bt_cross, stdout);
    for (cronos_rel i = 0; i < line; i++)
        putc(ascii_h_border, stdout);
    putc(ascii_rbt, stdout);

    putc('\n', stdout);
}

CroBuffer dump_record(CroFile* file, CroRecordTable& dat, CroEntry& entry)
{
    CroBuffer record;
    const CronosABI* abi = file->ABI();

    if (!entry.HasBlock())
    {
        cronos_rel off = dat.DataOffset(entry.EntryOffset());
        record.Write(dat.Data(off), entry.EntrySize());
        printf("%p\n", dat.Data(off));
        printf("%" FCroOff " rel %" FCroOff "\n", entry.EntryOffset(), off);

        printf("\tno block %" FCroOff " record part %" FCroSize "\n",
            entry.EntryOffset(), entry.EntrySize());
        return record;
    }

    CroBlock block = dat.FirstBlock(entry.Id());
    cronos_size recordSize = block.BlockSize();
    
    cronos_rel dataOff = block.GetStartOffset() + block.RecordOffset();
    cronos_size dataSize = entry.EntrySize()
        - abi->Size(cronos_first_block_hdr);
    if (!dat.IsValidOffset(dat.FileOffset(dataOff)))
    {
        printf("\tinvalid offset %" FCroOff "\n", dataOff);
        return record;
    }
    
    printf("\tfirst block %" FCroOff " record part %" FCroSize "\n",
        dat.GetStartOffset() + block.GetStartOffset(), dataSize);
    record.Write(dat.Data(dataOff), dataSize);
    recordSize -= dataSize;

    while (dat.NextBlock(block))
    {
        dataOff = block.GetStartOffset() + block.RecordOffset();
        dataSize = std::min(recordSize, file->GetDefaultBlockSize());
        if (!dat.IsValidOffset(dat.FileOffset(dataOff)))
        {
            printf("\tinvalid offset %" FCroOff "\n", dataOff);
            continue;
        }

        printf("\tnext block %" FCroOff " record part %" FCroSize "\n",
            dat.GetStartOffset() + block.GetStartOffset(), dataSize);
        record.Write(dat.Data(dataOff), dataSize);
        recordSize -= dataSize;

        if (!recordSize)
            break;
    }

    if (file->IsEncrypted())
    {
        file->Decrypt(record.GetData(), record.GetSize(), entry.Id());
    }

    return record;
}

void dump_crofile(CroFile* file)
{
    file->Reset();

    cronos_id tad_table_id = 1;
    while (!file->IsEndOfEntries())
    {
        cronos_idx tad_count = file->OptimalEntryCount();
        CroEntryTable tad = file->LoadEntryTable(tad_table_id, tad_count);
        if (tad.IsEmpty()) break;
        
        cronos_id dat_table_id = tad_table_id;
        while (dat_table_id < tad_table_id + tad_count)
        {
            cronos_idx dat_count = std::min(tad_count,
                file->OptimalRecordCount(tad, tad.IdStart()));
            CroRecordTable dat = file->LoadRecordTable(
                tad, tad.Id(), dat_count);
            FILE* fTableDat = fopen("table_dat.bin", "wb");
            fwrite(dat.GetData(), dat.GetSize(), 1, fTableDat);
            fclose(fTableDat);

            for (cronos_id id = dat.IdStart(); id != dat.IdEnd(); id++)
            {
                CroEntry entry = tad.GetEntry(id);
                if (!entry.IsActive())
                    continue;
                CroBlock block = dat.FirstBlock(id);

                printf("%" FCroId " BLOCK %" FCroOff " size %"
                    FCroSize " record %" FCroOff " NEXT %" FCroOff "\n",
                    id, dat.FileOffset(block.GetStartOffset()),
                    block.BlockSize(),
                    dat.FileOffset(block.RecordOffset()),
                    block.BlockNext()
                );

                // dump_record test
                CroBuffer record = dump_record(file, dat, entry);
                dump_buffer(record);
                return;
            }

            printf("DAT entry count %u\n", dat.GetEntryCount());
            break;
            dat_table_id += dat.GetEntryCount();
        }

        tad_table_id += tad.GetEntryCount();
    }

    printf("\n");
}

void log_table(const CroTable& table)
{
    printf(
        "== %s TABLE 0x%" FCroOff " %" FCroSize " ID "
        "%" FCroId "-%" FCroId " COUNT %" FCroIdx "\n",
        table.GetFileType() == CRONOS_TAD ? "TAD" : "DAT",
        table.TableOffset(), table.TableSize(),
        table.IdStart(), table.IdEnd(),
        table.GetEntryCount()
    );
}

int main(int argc, char** argv)
{
    std::wstring bankPath = testBank;
    if (argc >= 2)
    {
        wchar_t szBank[MAX_PATH] = {0};
        MultiByteToWideChar(CP_THREAD_ACP, 0, argv[1], -1,
                szBank, MAX_PATH);
        bankPath = szBank;
    }

    std::wstring fileName = L"CroBank";
    if (argc >= 3)
    {
        wchar_t szFile[64] = {0};
        MultiByteToWideChar(CP_THREAD_ACP, 0, argv[2], -1,
                szFile, 64);
        fileName = szFile;
    }

    if (fileName == L"--scan")
    {
        scan_directory(bankPath);
        printf("\n\n== found version headers:\n");
        for(auto& ver : known_versions)
            printf("\t%s\n", ver.c_str());
        return 0;
    }

    unsigned codepage = 1251;

    CroFile bank(bankPath + L"\\" + fileName);
    crofile_status st;

    for (int i = 3; i < argc; i++)
    {
        if (!strcmp(argv[i], "--set-secret"))
        {
            bank.SetSecret(atoi(argv[++i]), atoi(argv[++i]));
        }
        else if (!strcmp(argv[i], "--codepage"))
        {
            codepage = atoi(argv[++i]);
        }
    }

    if ((st = bank.Open()) != CROFILE_OK)
    {
        fprintf(stderr, "CroFile error %d\n\t%s\n",
                st, bank.GetError().c_str());
        return 1;
    }

    /*printf("bank version %d\n", bank.GetVersion());

    printf("optimal entry count %u\n", bank.OptimalEntryCount());
    CroEntryTable tad = bank.LoadEntryTable(1, bank.OptimalEntryCount());

    cronos_idx count = bank.OptimalRecordCount(tad, 1);
    printf("optimal record count %u\n", count);

    cronos_off start, end;
    bank.RecordTableOffsets(tad, 1, count, start, end);
    printf("%" FCroOff "-%" FCroOff " record table\n", start, end);*/

    for (int i = 3; i < argc; i++)
    {
        bank.Reset();
        
        if (!strcmp(argv[i], "--crypt-table"))
        {
            if (bank.IsEncrypted())
                dump_buffer(bank.GetCryptTable());
        }
        else if (!strcmp(argv[i], "--entry"))
        {
            cronos_id tad_id = i + 1 < argc ? atoi(argv[++i]) : 1;
            cronos_idx tad_count = i + 1 < argc ? atoi(argv[++i]) : 0;

            while (!bank.IsEndOfEntries())
            {
                CroEntryTable tad = bank.LoadEntryTable(tad_id, tad_count > 0
                    ? tad_count : bank.OptimalEntryCount());
                if (tad.IsEmpty()) break;
                log_table(tad);

                for (cronos_id id = tad.IdStart(); id != tad.IdEnd(); id++)
                {
                    CroEntry tad_entry = tad.GetEntry(id);

                    printf("%" FCroId "\t", tad_entry.Id());
                    if (tad_entry.IsActive())
                    {
                        printf("ENTRY\t%016" FCroOff "\t%" FCroSize
                            "\t%" FCroFlags "\n",
                            tad_entry.EntryOffset(),
                            tad_entry.EntrySize(),
                            tad_entry.EntryFlags()
                        );
                    }
                    else printf("INACTIVE\n");
                }

                if (tad_count > 0) break;
                tad_id = tad.IdEnd();
            }
        }
        else if (!strcmp(argv[i], "--secret"))
        {
            const CroData& secret = bank.GetSecret();

            if (bank.IsEncrypted())
            {
                printf("SERIAL\t%" PRIu32 "\n", secret.Get<uint32_t>(0x00));
                printf("KEY\t%" PRIu32 "\n", secret.Get<uint32_t>(0x04));
                dump_buffer(secret);
            }
        }
        else if (!strcmp(argv[i], "--block"))
        {
            cronos_id tad_id = i + 1 < argc ? atoi(argv[++i]) : 1;
            cronos_idx tad_count = i + 1 < argc ? atoi(argv[++i]) : 0;

            while (!bank.IsEndOfEntries())
            {
                CroEntryTable tad = bank.LoadEntryTable(tad_id, tad_count > 0
                    ? tad_count : bank.OptimalEntryCount());
                if (tad.IsEmpty()) break;

                cronos_off dat_start, dat_end;
                cronos_idx dat_count = bank.OptimalRecordCount(tad, tad.Id());
                bank.RecordTableOffsets(tad, tad.Id(), dat_count,
                    dat_start, dat_end);
                CroRecordTable dat = bank.LoadRecordTable(tad,
                    tad_id, dat_count);
                if (dat.IsEmpty()) break;

                log_table(dat);

                for (cronos_id id = tad.IdStart(); id != tad.IdEnd(); id++)
                {
                    CroEntry tad_entry = tad.GetEntry(id);

                    printf("%" FCroId "\t", tad_entry.Id());
                    if (tad_entry.IsActive())
                    {
                        CroBlock dat_entry = dat.FirstBlock(id);
                        if (tad_entry.HasBlock())
                        {
                            printf("BLOCK\t%016" FCroOff "\t%" FCroSize
                                "\t%016" FCroOff "\n",
                                tad_entry.EntryOffset(),
                                dat_entry.BlockSize(),
                                dat_entry.BlockNext()
                            );
                        }
                        else
                        {
                            printf("NOBLOCK\t%016" FCroOff "\t%" FCroSize
                                "\t%016" FCroOff "\n",
                                tad_entry.EntryOffset(),
                                tad_entry.EntrySize(),
                                0ULL
                            );
                        }
                    }
                    else printf("INACTIVE\n");
                }

                if (tad_count > 0) break;
                tad_id = tad.IdEnd();
            }
        }
        else if (!strcmp(argv[i], "--record"))
        {
            cronos_id tad_id = i + 1 < argc ? atoi(argv[++i]) : 1;
            cronos_idx tad_count = i + 1 < argc ? atoi(argv[++i]) : 0;

            CroEntryTable tad = bank.LoadEntryTable(tad_id, tad_count > 0
                ? tad_count : bank.OptimalEntryCount());
            if (tad.IsEmpty()) break;

            cronos_off dat_start, dat_end;
            cronos_idx dat_count = bank.OptimalRecordCount(tad, tad.Id());
            bank.RecordTableOffsets(tad, tad.Id(),
                dat_count, dat_start, dat_end);
            CroRecordTable dat = bank.LoadRecordTable(tad,
                tad_id, dat_count);
            if (dat.IsEmpty()) break;

            CroEntry tad_entry = tad.GetEntry(tad_id);
            if (tad_entry.IsActive())
            {
                CroBuffer record = dump_record(&bank, dat, tad_entry);
                dump_buffer(record, codepage);
            }
        }

        putc('\n', stdout);
    }

    bank.Close();
    return 0;
}
