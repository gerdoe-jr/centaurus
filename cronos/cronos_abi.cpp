#include "cronos_abi.h"
#include "crodata.h"
#include "crofile.h"
#include "croexception.h"
#include "cronos_format.h"
#include "cronos02.h"

/* CronosABIValue */

CronosABIValue::CronosABIValue(cronos_filetype ftype,
    cronos_size size, uint64_t mask)
    : m_ValueFile(ftype),
    m_ValueSize(size),
    m_ValueMask(mask)
{
}

/* Cronos ABI format values */

CronosABIValue cronos_hdr(CRONOS_DAT, CRONOS_HEADER_SIZE);
CronosABIValue cronos_hdr_sig(CRONOS_DAT, 7);
CronosABIValue cronos_hdr_major(CRONOS_DAT, 2);
CronosABIValue cronos_hdr_minor(CRONOS_DAT, 2);
CronosABIValue cronos_hdr_flags(CRONOS_DAT, 2, 0xFFFF);
CronosABIValue cronos_hdr_deflength(CRONOS_DAT, 2, 0xFFFF);
CronosABIValue cronos_hdr_secret(CRONOS_DAT, 8);
CronosABIValue cronos_hdrlite_secret(CRONOS_DAT, 8);
CronosABIValue cronos_hdr_crypt(CRONOS_DAT, 0x200);

CronosABIValue cronos_tad_base(CRONOS_TAD, 0);
CronosABIValue cronos_tad_entry(CRONOS_TAD, 0);
CronosABIValue cronos_tad_offset(CRONOS_TAD, 0);
CronosABIValue cronos_tad_size(CRONOS_TAD, 0);
CronosABIValue cronos_tad_flags(CRONOS_TAD, 0);
CronosABIValue cronos_tad_rz(CRONOS_TAD, 0);

/* CronosABI */

CronosABI* CronosABI::s_pFirst = NULL;
CronosABI* CronosABI::s_pLast = NULL;

CronosABI::CronosABI()
    : m_ABIVersion(INVALID_CRONOS_ABI)
{
    AddFormat();
}

CronosABI::CronosABI(cronos_abi_num num)
    : m_ABIVersion(num)
{
    AddFormat();
}

void CronosABI::AddFormat()
{
    if (!s_pFirst) s_pFirst = this;
    if (s_pLast) s_pLast->m_pNext = this;
    s_pLast = this;
}

bool CronosABI::IsFormat() const
{
    return m_ABIVersion != INVALID_CRONOS_ABI;
}

cronos_version CronosABI::Minor() const
{
    return m_ABIVersion.second;
}

bool CronosABI::IsVersion(cronos_version ver) const
{
    return GetVersion() == ver;
}

bool CronosABI::Is3() const
{
    return IsVersion(3);
}

bool CronosABI::Is4A() const
{
    return GetVersion() >= 4;
}

cronos_rel CronosABI::Offset(const CronosABIValue& value) const
{
    if (!HasFormatValue(value))
        return INVALID_CRONOS_OFFSET;
    return GetFormatOffset(value);
}

const uint8_t* CronosABI::GetPtr(const CroData& data,
    const CronosABIValue& value) const
{
    cronos_rel off = Offset(value);
    if (!data.IsValidOffset(off))
        throw CroABIError(this, value, "value ptr");
    return data.Data(off);
}

void CronosABI::GetData(const CroData& data,
    const CronosABIValue& value, CroData& out) const
{
    if (!HasFormatValue(value))
        throw CroException(data.File(), "ABI no value for CroData");

    cronos_rel rel = Offset(value);
    if (!data.IsValidOffset(data.FileOffset(rel)))
        throw CroException(data.File(), "ABI get data", rel);

    out.Copy(data.Data(rel), GetFormatSize(value));
}

uint64_t CronosABI::GetValue(const CroData& data,
    const CronosABIValue& value) const
{
    if (!HasFormatValue(value))
        throw CroException(data.File(), "ABI no value");

    cronos_rel off = Offset(value);
    if (!data.IsValidOffset(data.FileOffset(off)))
        throw CroException(data.File(), "ABI get", off);

    uint64_t uValue = 0;
    if (value.ValueSize() == 2) uValue = data.Get<uint16_t>(off);
    else if (value.ValueSize() == 4) uValue = data.Get<uint32_t>(off);
    else if (value.ValueSize() == 8) uValue = data.Get<uint64_t>(off);

    return value.MaskValue(uValue);
}

CroData CronosABI::ReadData(CroFile* file,
    const CronosABIValue& value) const
{
    if (!HasFormatValue(value))
        throw CroException(file, "ABI no value");

    cronos_pos pos = Offset(value);
    if (!file->IsValidOffset(pos, value.ValueFile()))
        throw CroException(file, "ABI read", pos);

    return file->Read(INVALID_CRONOS_ID, 1,
        GetFormatSize(value), value.ValueFile());
}

const CronosABI* CronosABI::LoadABI(cronos_abi_num num)
{
    const CronosABI* abi = s_pFirst;
    while (abi)
    {
        if (abi->IsCompatible(num))
        {
            if (abi->IsFormat()) return abi;
            return abi->Instance(num);
        }
        abi = abi->m_pNext;
    }

    return NULL;
}

/* Generic Cronos ABI */

class CronosABIGeneric : public CronosABI
{
public:
    CronosABIGeneric()
    {
    }
    
    CronosABIGeneric(cronos_abi_num num) : CronosABI(num)
    {
    }

    virtual CronosABI* Instance(cronos_abi_num num) const
    {
        return new CronosABIGeneric(num);
    }

    virtual cronos_version GetVersion() const
    {
        return INVALID_CRONOS_VERSION;
    }

    virtual bool IsCompatible(cronos_abi_num num) const
    {
        return false;
    }

    virtual bool IsLite() const
    {
        return false;
    }

    virtual bool HasFormatValue(const CronosABIValue& value) const
    {
        return false;
    }

    virtual cronos_off GetFormatOffset(const CronosABIValue& value) const
    {
        if (value == cronos_hdr)                return 0x00;
        else if (value == cronos_hdr_sig)       return 0x00;
        else if (value == cronos_hdr_major)     return 0x0A;
        else if (value == cronos_hdr_minor)     return 0x0D;
        else if (value == cronos_hdr_flags)     return 0x0F;
        else if (value == cronos_hdr_deflength) return 0x11;

        return INVALID_CRONOS_OFFSET;
    }

    virtual cronos_size GetFormatSize(const CronosABIValue& value) const
    {
        cronos_size valueSize = value.ValueSize();
        if (!valueSize)
            throw CroABIError(this, value, "format size");
        return valueSize;
    }
} cronos_abi_generic;

/* Cronos 3x ABI */

class CronosABI3 : public CronosABIGeneric
{
public:
    CronosABI3() : CronosABIGeneric()
    {
    }

    CronosABI3(cronos_abi_num num) : CronosABIGeneric(num)
    {
    }

    CronosABI* Instance(cronos_abi_num num) const override
    {
        return new CronosABI3(num);
    }

    cronos_version GetVersion() const override
    {
        return CRONOS_V3;
    }

    bool IsCompatible(cronos_abi_num num) const override
    {
        if (num.first != 1)
            return false;
        return num.second >= 2 && num.second <= 4;
    }

    bool IsLite() const override
    {
        return m_ABIVersion.second == 4;
    }

    bool HasFormatValue(const CronosABIValue& value) const override
    {
        if (value == cronos_hdr_secret) return !IsLite();
        else if (value == cronos_hdrlite_secret) return IsLite();
        else if (value == cronos_hdr_crypt)
            return m_ABIVersion.second != 2;

        return false;
    }

    cronos_off GetFormatOffset(const CronosABIValue& value) const override
    {
        if (value == cronos_hdr_secret)             return 0x13;
        else if (value == cronos_hdrlite_secret)    return 0x33;
        else if (value == cronos_hdr_crypt)         return 0xFC;
        else if (value == cronos_tad_base)          return 0x08;
        else if (value == cronos_tad_offset)        return 0x00;
        else if (value == cronos_tad_size)          return 0x04;
        else if (value == cronos_tad_flags)         return 0x08;

        return CronosABIGeneric::GetFormatOffset(value);
    }

    void GetData(const CroData& data, const CronosABIValue& value,
        CroData& out) const override
    {
        if (value == cronos_hdr_crypt)
        {
            if (m_ABIVersion.second == 2)
            {
                out.InitBuffer(cronos02_crypt_table,
                    value.ValueSize(), false);
            }
        }
        else CronosABIGeneric::GetData(data, value, out);
    }

    uint64_t GetValue(const CroData& data,
        const CronosABIValue& value) const override
    {
        cronos_rel off = Offset(value);
        
        if (value == cronos_tad_offset)
            return TAD_V3_OFFSET(data.Get<uint32_t>(off));
        else if (value == cronos_tad_size)
            return TAD_V3_FSIZE(data.Get<uint32_t>(off));
        else if (value == cronos_tad_flags)
            return data.Get<uint32_t>(off);
        else if (value == cronos_tad_rz)
            return 0;

        return CronosABIGeneric::GetValue(data, value);
    }
} cronos_abi_v3;