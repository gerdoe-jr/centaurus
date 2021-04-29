#ifndef __CROREADER_H
#define __CROREADER_H

#include "crobank.h"
#include "crosync.h"

class CroReader
{
public:
    CroReader(CroBank* bank);

    void ReadMap(CroRecordMap* map);
    bool ReadRecord(cronos_id id, CroBuffer& record);
protected:
    virtual void OnRecord();
    virtual void OnValue();

    CroBank* m_pBank;
    CroBankParser m_Parser;
};

#endif