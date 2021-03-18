#ifndef __CRONOS_ABI_H
#define __CRONOS_ABI_H

#include "crotype.h"
#include <memory>
#include <vector>

class CroFile;

enum cronos_value : unsigned {
    cronos_hdr = 0,
    cronos_hdr_sig,
    cronos_hdr_major,
    cronos_hdr_minor,
    cronos_hdr_flags,
    cronos_hdr_deflength,
    cronos_hdr_secret,
    cronos_hdrlite_secret,
    cronos_hdr_crypt,

    cronos_tad_entry,
    cronos_tad_offset,
    cronos_tad_size,
    cronos_tad_flags,
    cronos_tad_rz,

    cronos_first_block_hdr,
    cronos_first_block_next,
    cronos_first_block_size,
    cronos_first_block_data,
    cronos_block_hdr,
    cronos_block_next,
    cronos_block_data,

    cronos_last
};

#define cronos_value_count cronos_last

enum cronos_value_type {
    cronos_value_struct,
    cronos_value_data,
    cronos_value_uint16,
    cronos_value_uint32,
    cronos_value_uint64
};

struct cronos_abi_value {
    cronos_filetype m_FileType;
    cronos_value_type m_ValueType;
    union {
        cronos_off m_Offset;
        const uint8_t* m_pMem;
    };
    cronos_size m_Size;
    uint64_t m_Mask;
};

enum cronos_model {
    cronos_model_small,
    cronos_model_big
};

class CronosABI
{
public:
    CronosABI();
    CronosABI(cronos_version ver);
    CronosABI(cronos_version ver, cronos_abi_num num);
    virtual ~CronosABI() {}

    virtual CronosABI* LoadABI(cronos_abi_num num) const;

    virtual bool IsCompatible(cronos_abi_num num) const;
    virtual bool IsLite() const;
    virtual cronos_model GetModel() const;

    cronos_version GetVersion() const;
    cronos_abi_num GetABIVersion() const;
    inline cronos_version Minor() const { return m_ABIVersion.second;  }

    const struct cronos_abi_value* GetValue(
        cronos_value value) const noexcept;
    const char* GetValueName(cronos_value value) const noexcept;

    inline cronos_off Offset(cronos_value value) const noexcept
    {
        return GetValue(value)->m_Offset;
    }

    inline cronos_size Size(cronos_value value) const noexcept
    {
        return GetValue(value)->m_Size;
    }
protected:
    virtual void InstallABI(cronos_version ver, cronos_abi_num num) noexcept;

    cronos_version m_Version;
    cronos_abi_num m_ABIVersion;
    std::vector<struct cronos_abi_value> m_Values;

    static std::vector<CronosABI*> s_ABIFamily;
    static std::vector<std::unique_ptr<CronosABI>> s_ABI;
public:
    static CronosABI* GetABI(cronos_abi_num num);
    static CronosABI* GenericABI();
};

#define install_abi_value(value) m_Values.push_back(value)
#define install_value(ftype, vtype, offset, size, mask)            \
    m_Values.push_back({ftype, vtype, offset, size, mask})

#endif