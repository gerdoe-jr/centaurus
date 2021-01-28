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
    cronos_off GetStartOffset() const;
    cronos_off GetEndOffset() const;

    crodata_offset_type CheckOffset(cronos_off off) const;
    bool IsValidOffset(cronos_off off) const;
    bool OffsetBehind(cronos_off off) const;
    bool OffsetAhead(cronos_off off) const;

    cronos_off DataOffset(cronos_off off) const;
    cronos_off FileOffset(cronos_off rel) const;

    const uint8_t* Data(cronos_off rel) const;
    uint8_t* Data(cronos_off rel);

    template<typename T> inline
    T Get(cronos_off rel) const
    {
        return *(T*)Data(rel);
    }
private:
    cronos_filetype m_FileType;
    cronos_off m_uOffset;
};

#endif
