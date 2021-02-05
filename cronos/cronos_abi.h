#ifndef __CRONOS_ABI_H
#define __CRONOS_ABI_H

#include "crotype.h"
#include "crodata.h"

class CroFile;

/* CronosABIValue */

class CronosABIValue
{
public:
    CronosABIValue(cronos_filetype ftype, cronos_size size,
        uint64_t mask = (uint64_t)-1);

    inline cronos_filetype ValueFile() const { return m_ValueFile;  }
    inline cronos_size ValueSize() const { return m_ValueSize; }
    inline uint64_t MaskValue(uint64_t value) const
    {
        return value & m_ValueMask;
    }
private:
    cronos_filetype m_ValueFile;
    cronos_size m_ValueSize;
    uint64_t m_ValueMask;
};

/* Cronos ABI format values */

extern CronosABIValue cronos_hdr_sig;
extern CronosABIValue cronos_hdr_flags;
extern CronosABIValue cronos_hdr_deflength;
extern CronosABIValue cronos_hdr_secret;
extern CronosABIValue cronos_hdrlite_secret;

extern CronosABIValue cronos_crypt_table;

/* CronosABI */

class CronosABI
{
public:
    CronosABI();
    CronosABI(cronos_abi_num num);

    void AddFormat();
    bool IsFormat() const;

    virtual bool IsCompatible(cronos_abi_num num) const = 0;
    virtual bool IsLite() const = 0;
    virtual bool HasFormatValue(const CronosABIValue& value) const = 0;
    virtual cronos_rel GetFormatOffset(const CronosABIValue& value) const = 0;

    cronos_rel Offset(const CronosABIValue& value) const;

    virtual void GetData(const CroData* data,
        const CronosABIValue& value, CroData& out) const;
    virtual uint64_t GetValue(const CroData* data,
        const CronosABIValue& value) const;
    
    template<typename T>
    inline T GetValue(const CroData* data,
        const CronosABIValue& value) const
    {
        return (T)GetValue(value);
    }

    static const CronosABI* LoadABI(cronos_abi_num num);
protected:
    cronos_abi_num m_ABIVersion;
private:
    CronosABI* m_pNext;

    static CronosABI* s_pFirst;
    static CronosABI* s_pLast;
};

#endif