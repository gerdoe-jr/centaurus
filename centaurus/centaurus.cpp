#include "centaurus.h"
#include "win32util.h"
#include "crofile.h"
#include "cronos_abi.h"
#include <stdexcept>
#include <exception>
#include <memory>
#include <stdio.h>

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
namespace sc = boost::system;

/* Centaurus Bank */

class CCentaurusBank : public ICentaurusBank
{
public:
    bool LoadPath(const std::wstring& path) override
    {
        m_Path = path;

        m_Files[CroStru] = std::make_unique<CroFile>(m_Path + L"\\CroStru");
        m_Files[CroBank] = std::make_unique<CroFile>(m_Path + L"\\CroBank");
        m_Files[CroIndex] = std::make_unique<CroFile>(m_Path + L"\\CroIndex");

        for (unsigned i = 0; i < CroBankFile_Count; i++)
        {
            CroFile* file = File((CroBankFile)i);
            if (!file) continue;

            try {
                crofile_status st = file->Open();
                if (st != CROFILE_OK)
                {
                    throw std::runtime_error("CroFile status "
                        + std::to_string(st));
                }
            } catch (const std::exception& e) {
                fwprintf(stderr, L"CroFile(%s): %S\n",
                    file->GetPath().c_str(), e.what());
                m_Files[(CroBankFile)i] = NULL;
            }
        }

        return File(CroStru) || File(CroBank) || File(CroIndex);
    }

    CroFile* File(CroBankFile type) const override
    {
        return !m_Files[type] ? NULL : m_Files[type].get();
    }

    void ExportHeaders() const override
    {
        sc::error_code ec;
        std::wstring headerPath = m_Path + L"\\include";

        fs::create_directory(headerPath, ec);
        if (ec) throw std::runtime_error("ExportHeaders !create_directory");

        for (unsigned i = 0; i < CroBankFile_Count; i++)
        {
            CroFile* file = File((CroBankFile)i);
            if (!file) continue;

            FILE* fHdr = NULL;
            switch (i)
            {
            case CroStru:
                _wfopen_s(&fHdr, (headerPath + L"\\CroStru.h").c_str(), L"w");
                break;
            case CroBank:
                _wfopen_s(&fHdr, (headerPath + L"\\CroBank.h").c_str(), L"w");
                break;
            case CroIndex:
                _wfopen_s(&fHdr, (headerPath + L"\\CroIndex.h").c_str(), L"w");
                break;
            }

            if (!fHdr) throw std::runtime_error("ExportHeaders !fHdr");
            centaurus->ExportABIHeader(file->ABI(), fHdr);
            fclose(fHdr);
        }
    }
private:
    std::wstring m_Path;
    std::unique_ptr<CroFile> m_Files[CroBankFile_Count];
};

/* Centaurus API */

class CCentaurusAPI : public ICentaurusAPI
{
public:
    void Init() override
    {
        m_fOutput = stdout;
        m_fError = stderr;
    }

    void Exit() override
    {
        if (m_fOutput != stdout) fclose(m_fOutput);
        if (m_fError != stderr) fclose(m_fError);

        for (auto bank : m_Banks) delete bank;
        m_Banks.clear();
    }

    ICentaurusBank* ConnectBank(const std::wstring& path) override
    {
        CCentaurusBank* bank = new CCentaurusBank();
        if (!bank->LoadPath(path))
        {
            fwprintf(m_fError, L"centaurus: failed to open bank %s\n",
                path.c_str());
            delete bank;
            return NULL;
        }

        m_Banks.push_back(bank);
        return bank;
    }

    void DisconnectBank(ICentaurusBank* bank) override
    {
        auto it = std::find(m_Banks.begin(), m_Banks.end(), bank);
        if (it != m_Banks.end())
        {
            CCentaurusBank* theBank = *it;
            delete theBank;

            m_Banks.erase(it);
        }
    }

    void ExportABIHeader(const CronosABI* abi,
        FILE* out) const override
    {
        if (!out) out = m_fOutput;

        cronos_abi_num abiVersion = abi->GetABIVersion();
        fprintf(out, "// Cronos %dx %s %s, ABI %02d.%02d\n\n",
            abi->GetVersion(),
            abi->IsLite() ? "Lite" : "Pro",
            abi->GetModel() == cronos_model_big ? "big model" : "small model",
            abiVersion.first, abiVersion.second
        );

        cronos_rel member_off = 0;
        for (unsigned i = 0; i < cronos_last; i++)
        {
            cronos_value value = (cronos_value)i;
            const auto* pValue = abi->GetValue(value);

            const char* valueType;
            switch (pValue->m_ValueType)
            {
            case cronos_value_data: valueType = "uint8_t"; break;
            case cronos_value_struct: valueType = "CroData"; break;
            case cronos_value_uint16: valueType = "uint16_t"; break;
            case cronos_value_uint32: valueType = "uint32_t"; break;
            case cronos_value_uint64: valueType = "uint64_t"; break;
            }

            switch (pValue->m_FileType)
            {
            case CRONOS_MEM:
                fprintf(out,
                    "%s CroData(NULL, cronos_id, (const uint8_t*)%p, %"
                    FCroSize ");\n",
                    abi->GetValueName(value),
                    pValue->m_pMem,
                    pValue->m_Size
                );
                break;
            case CRONOS_TAD:
            case CRONOS_DAT:
                if (pValue->m_ValueType == cronos_value_struct)
                {
                    if (member_off)
                    {
                        fprintf(out, "};\n\n");
                        member_off = 0;
                    }

                    fprintf(out, "struct %s /* %" FCroSize " */ {\n",
                        abi->GetValueName(value),
                        pValue->m_Size
                    );
                }
                else
                {
                    const char* valueName = abi->GetValueName(value) + 7;
                    if (pValue->m_Offset > member_off)
                    {
                        unsigned skip = pValue->m_Offset - member_off;
                        fprintf(out, "\tuint8_t __skip%" FCroOff
                            "[%u];\n\n", member_off, skip);
                        member_off += skip;
                    }

                    if (pValue->m_Offset < member_off)
                    {
                        fprintf(out,
                            "\t/* 0x%" FCroOff " %" FCroSize " & 0x%"
                            PRIX64 " */\n",
                            pValue->m_Offset,
                            pValue->m_Size,
                            pValue->m_Mask
                        );
                    }
                    else if (pValue->m_ValueType == cronos_value_data)
                    {
                        fprintf(out, "\t%s %s[%" FCroSize "];\n",
                            valueType, valueName, pValue->m_Size
                        );
                    }
                    else
                    {
                        fprintf(out, "\t%s %s; /* & 0x%" PRIx64 " */\n",
                            valueType, valueName, pValue->m_Mask
                        );
                    }

                    member_off += pValue->m_Size;
                }

                break;
            }
        }

        if (member_off) fprintf(out, "};\n");
        fprintf(out, "\n");
    }

    void LogBankFiles(ICentaurusBank* bank) const override
    {
        for (unsigned i = 0; i < CroBankFile_Count; i++)
        {
            CroFile* file = bank->File((CroBankFile)i);
            if (!file) continue;

            fprintf(m_fOutput, "CroFile(%s):\n",
                WcharToAnsi(file->GetPath()).c_str()
            );
            
            const CronosABI* abi = file->ABI();
            cronos_abi_num abiVersion = abi->GetABIVersion();
            
            fprintf(m_fOutput,
                "\tCronos %dx %s %s, ABI %02d.%02d\n",
                abi->GetVersion(),
                abi->IsLite() ? "Lite" : "Pro",
                abi->GetModel() == cronos_model_big ? "big model" : "small model",
                abiVersion.first, abiVersion.second
            );
            
            fprintf(m_fOutput,
                "\tTAD FileSize: %" FCroSize "\n"
                "\tDAT FileSize: %" FCroSize "\n",
                file->FileSize(CRONOS_TAD),
                file->FileSize(CRONOS_DAT)
            );
            
            fprintf(m_fOutput,
                "\tEntryCountFileSize: %" FCroIdx "\n",
                file->EntryCountFileSize()
            );
        }
    }
private:
    std::vector<CCentaurusBank*> m_Banks;
    FILE* m_fOutput;
    FILE* m_fError;
};

/* Centaurus ABI */

CENTAURUS_API ICentaurusAPI* centaurus = NULL;

CENTAURUS_API bool InitCentaurusAPI()
{
    ICentaurusAPI* api = new CCentaurusAPI();
    if (!api) return false;

    try {
        api->Init();
    } catch (const std::exception& e) {
        fprintf(stderr, "InitCentaurusAPI: %s\n", e.what());
        delete api;
    }

    centaurus = api;
    return centaurus;
}

CENTAURUS_API void ExitCentaurusAPI()
{
    if (!centaurus) return;
    
    try {
        centaurus->Exit();
    } catch (const std::exception& e) {
        fprintf(stderr, "ExitCentaurusAPI: %s\n", e.what());
    }

    delete centaurus;
    centaurus = NULL;
}
