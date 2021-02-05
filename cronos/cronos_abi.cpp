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

CronosABIValue cronos_hdr_sig(CRONOS_DAT, 7);
CronosABIValue cronos_hdr_flags(CRONOS_DAT, 2, 0xFFFF);
CronosABIValue cronos_hdr_deflength(CRONOS_DAT, 2, 0xFFFF);
CronosABIValue cronos_hdr_secret(CRONOS_DAT, 8);
CronosABIValue cronos_hdrlite_secret(CRONOS_DAT, 8);

CronosABIValue cronos_dat_crypt(CRONOS_DAT, 0x200);

/* CronosABI */

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

cronos_rel CronosABI::Offset(const CronosABIValue& value) const
{
    if (!HasFormatValue(value))
        return INVALID_CRONOS_OFFSET;
    return GetFormatOffset(value);
}

void CronosABI::GetData(const CroData* data,
    const CronosABIValue& value, CroData& out) const
{
    if (!HasFormatValue(value))
        throw CroException(data->File(), "ABI no value for CroData");

    cronos_rel rel = Offset(value);
    if (!data->IsValidOffset(data->FileOffset(rel)))
        throw CroException(data->File(), "ABI get data", rel);

    out.Copy(data->Data(rel), value.ValueSize());
}

uint64_t CronosABI::GetValue(const CroData* data,
    const CronosABIValue& value) const
{
    if (!HasFormatValue(value))
        throw CroException(data->File(), "ABI no value");

    cronos_rel off = Offset(value);
    if (!data->IsValidOffset(data->FileOffset(off)))
        throw CroException(data->File(), "ABI get", off);

    uint64_t uValue = 0;
    if (value.ValueSize() == 2) uValue = data->Get<uint16_t>(off);
    else if (value.ValueSize() == 4) uValue = data->Get<uint32_t>(off);
    else if (value.ValueSize() == 8) uValue = data->Get<uint64_t>(off);

    return value.MaskValue(uValue);
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

    virtual bool IsCompatible(cronos_abi_num num) const
    {
        return num.first = 1;
    }

    virtual bool IsLite() const
    {
        return false;
    }

    virtual bool HasFormatValue(const CronosABIValue& value) const
    {
        return true;
    }

    virtual cronos_off GetFormatOffset(const CronosABIValue& value) const
    {
        /*switch (value)
        {
        case cronos_hdr_sig:        return 0x00;
        case cronos_hdr_flags:      return 0x0F;
        case cronos_hdr_deflength:  return 0x11;
        }
        return INVALID_CRONOS_OFFSET;

        if (value == cronos_hdr_secret)
            return 0;*/
        return INVALID_CRONOS_OFFSET;
    }
};

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
    
    bool IsCompatible(cronos_abi_num num) const override
    {
        if (!CronosABIGeneric::IsCompatible(num))
            return false;
        return num.second >= 2 && num.second <= 4;
    }

    bool IsLite() const override
    {
        return m_ABIVersion.second == 4;
    }

    bool HasFormatValue(const CronosABIValue& value) const override
    {
        /*if (value == cronos_hdr_secret)
            return !IsLite();
        else if (value == cronos_hdrlite_secret)
            return IsLite();
        else if (value == cronos_crypt_table)
            return m_ABIVersion.second != 2;
        return CronosABIGeneric::HasFormatValue(value);*/

        /*if (value == cronos_hdr_secret)
            return false;*/
    }

    cronos_off GetFormatOffset(const CronosABIValue& value) const override
    {
        /*switch (value)
        {
        case cronos_hdr_secret:     return 0x13;
        case cronos_hdrlite_secret: return 0x33;
        case cronos_crypt_table:    return 0xFC;
        }*/
        return CronosABIGeneric::GetFormatOffset(value);
    }

    void GetData(const CroData* data, const CronosABIValue& value,
        CroData& out) const override
    {
        /*if (value == cronos_crypt_table)
        {
            if (m_ABIVersion.second == 2)
            {
                out.InitBuffer(cronos02_crypt_table,
                    value.ValueSize(), false);
            }
        }
        else */CronosABIGeneric::GetData(data, value, out);
    }
} cronos_abi_v3;