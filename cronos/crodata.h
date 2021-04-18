#ifndef __CRODATA_H
#define __CRODATA_H

#include "crotype.h"
#include "crobuffer.h"
#include "croentity.h"
#include "cronos_abi.h"

typedef enum {
    CRODATA_OFFSET_INVALID,
    CRODATA_OFFSET_OK,
    CRODATA_OFFSET_BEHIND,
    CRODATA_OFFSET_AHEAD
} crodata_offset_type;

class CroData : public CroBuffer, public CroEntity
{
public:
    CroData();
    CroData(CroFile* file, cronos_id id, cronos_filetype ftype,
            cronos_off off, cronos_size size);
    CroData(const CroData& table, cronos_id id,
            cronos_rel off, cronos_size size);
    CroData(CroFile* file, cronos_id id,
        const uint8_t* data, cronos_size size);
    
    static CroData CopyData(const CroData& data);

    void SetOffset(cronos_off off);
    void SetOffset(cronos_off off, cronos_filetype ftype);
    void InitData(CroFile* file, cronos_id id, cronos_filetype ftype,
            cronos_off off, cronos_size size);
    void InitMemory(CroFile* file, cronos_id id,
        const cronos_abi_value* value);

    cronos_filetype GetFileType() const;
    cronos_pos GetStartOffset() const;
    cronos_pos GetEndOffset() const;

    virtual bool CheckValue(cronos_pos value) const;

    crodata_offset_type CheckOffset(cronos_pos off) const;
    bool IsValidOffset(cronos_pos off) const;
    bool OffsetBehind(cronos_pos off) const;
    bool OffsetAhead(cronos_pos off) const;

    cronos_off DataOffset(cronos_pos off) const;
    cronos_off FileOffset(cronos_rel off) const;

    const uint8_t* Data(cronos_rel off) const;
    uint8_t* Data(cronos_rel off);

    CroData Value(cronos_value value);
    CroData CopyValue(cronos_value value);

    template<typename T>
    inline T Get(cronos_rel off) const
    {
        return *(T*)Data(off);
    }

    inline const uint8_t* Data(cronos_value value) const
    {
        return Data(ABI()->GetValue(value)->m_Offset);
    }

    uint8_t* Data(cronos_value value)
    {
        return Data(ABI()->GetValue(value)->m_Offset);
    }

    template<typename T>
    inline T Get(cronos_value value) const
    {
        const auto* i = ABI()->GetValue(value);

        uint64_t dataValue = 0;
        switch (i->m_ValueType)
        {
        case cronos_value_uint16:
            dataValue = *(uint16_t*)Data(i->m_Offset); break;
        case cronos_value_uint32:
            dataValue = *(uint32_t*)Data(i->m_Offset); break;
        case cronos_value_uint64:
            dataValue = *(uint64_t*)Data(i->m_Offset); break;
        default: dataValue = *(T*)Data(i->m_Offset);
        }
        
        return dataValue & i->m_Mask;
    }
private:
    cronos_filetype m_FileType;
    cronos_off m_uOffset;
};

#endif
