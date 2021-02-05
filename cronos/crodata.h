#ifndef __CRODATA_H
#define __CRODATA_H

#include "crotype.h"
#include "crobuffer.h"
#include "croentity.h"

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
            cronos_off rel, cronos_size size);

    void InitData(CroFile* file, cronos_id id, cronos_filetype ftype,
            cronos_off off, cronos_size size);

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

    template<typename T> inline
    T Get(cronos_rel off) const
    {
        return *(T*)Data(rel);
    }
private:
    cronos_filetype m_FileType;
    cronos_off m_uOffset;
};

#endif
