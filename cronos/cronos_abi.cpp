#include "cronos_abi.h"
#include "crodata.h"
#include "crofile.h"
#include "croexception.h"
#include "cronos_format.h"
#include "cronos02.h"

CronosABI::CronosABI()
{
    InstallABI(INVALID_CRONOS_VERSION, INVALID_CRONOS_ABI);
    s_ABIFamily.push_back(this);
}

CronosABI::CronosABI(cronos_version ver)
{
    InstallABI(INVALID_CRONOS_VERSION, INVALID_CRONOS_ABI);
    m_Version = ver;
}

CronosABI::CronosABI(cronos_version ver, cronos_abi_num num)
{
    InstallABI(ver, num);
}

CronosABI* CronosABI::LoadABI(cronos_abi_num num) const
{
    return new CronosABI(GetVersion(), num);
}

bool CronosABI::IsCompatible(cronos_abi_num num) const
{
    if (m_ABIVersion != INVALID_CRONOS_ABI)
        return m_ABIVersion == num;
    return false;
}

bool CronosABI::IsLite() const
{
    return false;
}

cronos_model CronosABI::GetModel() const
{
    return cronos_model_big;
}

cronos_version CronosABI::GetVersion() const
{
    return m_Version;
}

cronos_abi_num CronosABI::GetABIVersion() const
{
    return m_ABIVersion;
}

const struct cronos_abi_value* CronosABI::GetValue(
    cronos_value value) const noexcept
{
    return value < cronos_last ? m_Values.data() + value : NULL;
}

const char* CronosABI::GetValueName(cronos_value value) const noexcept
{
    static const char* s_pValueNames[] = {
        "cronos_hdr",
        "cronos_hdr_sig",
        "cronos_hdr_major",
        "cronos_hdr_minor",
        "cronos_hdr_flags",
        "cronos_hdr_deflength",
        "cronos_hdr_secret",
        "cronos_hdrlite_secret",
        "cronos_hdr_crypt",

        "cronos_tad_entry",
        "cronos_tad_offset",
        "cronos_tad_size",
        "cronos_tad_flags",
        "cronos_tad_rz",

        "cronos_first_block_hdr",
        "cronos_first_block_next",
        "cronos_first_block_size",
        "cronos_first_block_data",
        "cronos_block_hdr",
        "cronos_block_next",
        "cronos_block_data"
    };

    return value < cronos_last
        ? s_pValueNames[value] : "cronos_invalid";
}

void CronosABI::InstallABI(cronos_version ver, cronos_abi_num num) noexcept
{
    m_Version = ver;
    m_ABIVersion = num;
    if (ver == INVALID_CRONOS_VERSION || num == INVALID_CRONOS_ABI)
        return;
    m_Values.reserve(cronos_value_count);
}

std::vector<CronosABI*> CronosABI::s_ABIFamily;
std::vector<std::unique_ptr<CronosABI>> CronosABI::s_ABI;

CronosABI* CronosABI::GetABI(cronos_abi_num num)
{
    for (const auto& abi : s_ABI)
        if (abi->GetABIVersion() == num) return abi.get();

    for (const auto& fmt : s_ABIFamily)
    {
        if (fmt->IsCompatible(num))
            return s_ABI.emplace_back(fmt->LoadABI(num)).get();
    }

    return NULL;
}

CronosABI* CronosABI::GenericABI()
{
    return GetABI(INVALID_CRONOS_ABI);
}

/* Cronos generic ABI */

class CronosABI_Generic : public CronosABI
{
public:
    CronosABI_Generic() : CronosABI()
    {
    }

    CronosABI_Generic(cronos_version ver, cronos_abi_num num)
        : CronosABI(ver, num)
    {
        InstallABI(ver, num);
    }

    virtual CronosABI* LoadABI(cronos_abi_num num) const
    {
        return new CronosABI_Generic(INVALID_CRONOS_VERSION, num);
    }

    virtual bool IsCompatible(cronos_abi_num num) const
    {
        return num == INVALID_CRONOS_ABI;
    }

    virtual void InstallABI(cronos_version ver, cronos_abi_num num) noexcept
    {
        /* header */
        install_value(CRONOS_DAT, cronos_value_struct, 0x00, 4096, 0);
        install_value(CRONOS_DAT, cronos_value_data, 0x00, 7, 0);
        install_value(CRONOS_DAT, cronos_value_data, 0x0A, 2, 0);
        install_value(CRONOS_DAT, cronos_value_data, 0x0D, 2, 0);
        install_value(CRONOS_DAT, cronos_value_uint16, 0x0F, 2, 0xFFFF);
        install_value(CRONOS_DAT, cronos_value_uint16, 0x11, 2, 0xFFFF);
    }
} cronos_abi_generic;

/* Cronos 3x ABI */

class CronosABI_V3 : public CronosABI_Generic
{
public:
    CronosABI_V3() : CronosABI_Generic()
    {
    }

    CronosABI_V3(cronos_abi_num num)
        : CronosABI_Generic(CRONOS_V3, num)
    {
        InstallABI(CRONOS_V3, num);
    }

    CronosABI* LoadABI(cronos_abi_num num) const override
    {
        return new CronosABI_V3(num);
    }

    bool IsCompatible(cronos_abi_num num) const override
    {
        return num.first == 1 && num.second >= 2 && num.second <= 4;
    }

    bool IsLite() const override
    {
        return Minor() == 4;
    }

    cronos_model GetModel() const override
    {
        return Minor() == 2  ? cronos_model_small : cronos_model_big;
    }

    void InstallABI(cronos_version ver,
        cronos_abi_num num) noexcept override
    {
        /* header */
        install_value(CRONOS_DAT, cronos_value_data, 0x13, 8, 0);
        install_value(CRONOS_DAT, cronos_value_data, 0x33, 8, 0);
        if (GetModel() == cronos_model_small)
            install_abi_value(cronos02_crypt_table);
        else install_value(CRONOS_DAT, cronos_value_data, 0xFC, 512, 0);

        /* TAD */
        install_value(CRONOS_TAD, cronos_value_struct, 0x08, 12, 0);
        install_value(CRONOS_TAD, cronos_value_uint32, 0x00, 4, 0x1FFFFFFF);
        install_value(CRONOS_TAD, cronos_value_uint32, 0x04, 4, 0x7FFFFFFF);
        install_value(CRONOS_TAD, cronos_value_uint32, 0x08, 4, 0xFFFFFFFF);
        install_value(CRONOS_TAD, cronos_value_uint32, 0x04, 4, 0x80000000);

        /* DAT */
        install_value(CRONOS_DAT, cronos_value_struct, 0x00, 8, 0);
        install_value(CRONOS_DAT, cronos_value_uint32, 0x00, 4, 0x1FFFFFFF);
        install_value(CRONOS_DAT, cronos_value_uint32, 0x04, 4, 0x7FFFFFFF);
        install_value(CRONOS_DAT, cronos_value_data, 0x08, 0, 0);
        
        install_value(CRONOS_DAT, cronos_value_struct, 0x00, 4, 0);
        install_value(CRONOS_DAT, cronos_value_uint32, 0x00, 4, 0x1FFFFFFF);
        install_value(CRONOS_DAT, cronos_value_data, 0x04, 0, 0);
    }
} cronos_abi_v3;

/* Cronos 4x ABI */

class CronosABI_V4 : public CronosABI_Generic
{
public:
    CronosABI_V4() : CronosABI_Generic()
    {
    }

    CronosABI_V4(cronos_abi_num num)
        : CronosABI_Generic(CRONOS_V4, num)
    {
        InstallABI(CRONOS_V4, num);
    }

    CronosABI* LoadABI(cronos_abi_num num) const override
    {
        return new CronosABI_V4(num);
    }

    bool IsCompatible(cronos_abi_num num) const override
    {
        return num.first == 1 && num.second >= 11 && num.second <= 19;
    }

    bool IsLite() const override
    {
        return Minor() == 13 || Minor() == 19;
    }

    cronos_model GetModel() const override
    {
        return cronos_model_big;
    }

    void InstallABI(cronos_version ver,
        cronos_abi_num num) noexcept override
    {
        /* header */
        install_value(CRONOS_DAT, cronos_value_data, 0x13, 8, 0);
        install_value(CRONOS_DAT, cronos_value_data, 0x33, 8, 0);
        install_value(CRONOS_DAT, cronos_value_data, 0xF8, 512, 0);

        /* TAD */
        install_value(CRONOS_TAD, cronos_value_struct, 0x10, 16, 0);
        install_value(CRONOS_TAD, cronos_value_uint64,
            0x00, 8, 0x0000FFFFFFFFFFFF);
        install_value(CRONOS_TAD, cronos_value_uint32, 0x08, 4, 0xFFFFFFFF);
        install_value(CRONOS_TAD, cronos_value_uint32, 0x0C, 4, 0xFFFFFFFF);
        install_value(CRONOS_TAD, cronos_value_uint64,
            0x00, 0, 0xFFFF000000000000);

        /* DAT */
        install_value(CRONOS_DAT, cronos_value_struct, 0x00, 12, 0);
        install_value(CRONOS_DAT, cronos_value_uint64,
            0x00, 8, 0x0000FFFFFFFFFFFF);
        install_value(CRONOS_DAT, cronos_value_uint32, 0x08, 4, 0xFFFFFFFF);
        install_value(CRONOS_DAT, cronos_value_data, 0x0C, 0, 0);

        install_value(CRONOS_DAT, cronos_value_struct, 0x00, 8, 0);
        install_value(CRONOS_DAT, cronos_value_uint64,
            0x00, 8, 0x0000FFFFFFFFFFFF);
        install_value(CRONOS_DAT, cronos_value_data, 0x08, 0, 0);
    }
} cronos_abi_v4;
