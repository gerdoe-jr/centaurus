#ifndef __CROSTRU_H
#define __CROSTRU_H

#include "croparser.h"
#include "crorecord.h"

#define CROATTR_PREFIX      0x03
#define CROATTR_MIN_SIZE    0x0F

#define CROPROP_PREFIX      0x04

enum croblock_type : uint8_t {
    CROBLOCK_ATTR = CROATTR_PREFIX,
    CROBLOCK_PROP = CROPROP_PREFIX
};

class CroStru : public CroParser
{
public:
    CroStru(CroBank* bank, CroRecordMap* stru);

    inline CroRecordMap* CroStruMap() { return m_pStru; }

    croblock_type GetBlockType(cronos_id blockId);
    bool GetAttrByName(const std::string& name,
        CroBuffer& record, CroStream& stream);
private:
    CroRecordMap* m_pStru;
};

#endif