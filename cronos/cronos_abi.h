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

    inline bool operator==(const CronosABIValue& other) const
    {
        return this == &other;
    }
private:
    cronos_filetype m_ValueFile;
    cronos_size m_ValueSize;
    uint64_t m_ValueMask;
};

/* Cronos ABI format values */

extern CronosABIValue cronos_hdr;
extern CronosABIValue cronos_hdr_sig;
extern CronosABIValue cronos_hdr_major;
extern CronosABIValue cronos_hdr_minor;
extern CronosABIValue cronos_hdr_flags;
extern CronosABIValue cronos_hdr_deflength;
extern CronosABIValue cronos_hdr_secret;
extern CronosABIValue cronos_hdrlite_secret;
extern CronosABIValue cronos_hdr_crypt;

extern CronosABIValue cronos_tad_base;
extern CronosABIValue cronos_tad_entry;
extern CronosABIValue cronos_tad_offset;
extern CronosABIValue cronos_tad_size;
extern CronosABIValue cronos_tad_flags;
extern CronosABIValue cronos_tad_rz;

/* CronosABI */

class CronosABI
{
public:
    CronosABI();
    CronosABI(cronos_abi_num num);

    void AddFormat();
    bool IsFormat() const;
    inline const cronos_abi_num& Number() const { return m_ABIVersion; }

    virtual CronosABI* Instance(cronos_abi_num num) const = 0;

    virtual cronos_version GetVersion() const = 0;
    virtual bool IsCompatible(cronos_abi_num num) const = 0;
    virtual bool IsLite() const = 0;
    virtual bool HasFormatValue(const CronosABIValue& value) const = 0;
    virtual cronos_rel GetFormatOffset(const CronosABIValue& value) const = 0;
    virtual cronos_size GetFormatSize(const CronosABIValue& value) const = 0;

    cronos_version Minor() const;
    bool IsVersion(cronos_version ver) const;
    bool Is3() const;
    bool Is4A() const;

    cronos_rel Offset(const CronosABIValue& value) const;
    const uint8_t* GetPtr(const CroData& data,
        const CronosABIValue& value) const;

    virtual void GetData(const CroData& data,
        const CronosABIValue& value, CroData& out) const;
    virtual uint64_t GetValue(const CroData& data,
        const CronosABIValue& value) const;
    virtual CroData ReadData(CroFile* file,
        const CronosABIValue& value) const;
    
    template<typename T>
    inline T Get(const CroData& data,
        const CronosABIValue& value) const
    {
        return (T)GetValue(data, value);
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