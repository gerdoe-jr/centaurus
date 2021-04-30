#ifndef __CROREADER_H
#define __CROREADER_H

#include "crobank.h"
#include "crosync.h"

class ICroReader
{
public:
    virtual ~ICroReader() {}
protected:
    virtual void OnRecord() = 0;
    virtual void OnRecordEnd() = 0;

    virtual void OnValue() = 0;
    virtual void OnValueNext() = 0;
};

class CroReader : public ICroReader
{
public:
    CroReader(CroBank* bank);
    virtual ~CroReader();

    void ReadMap(CroRecordMap* map);
    void ReadRecord(cronos_id id, CroBuffer& record);
protected:
    virtual void OnRecord();
    virtual void OnRecordEnd();

    virtual void OnValue();
    virtual void OnValueNext();

    CroBank* m_pBank;
    CroBankParser m_Parser;
    crovalue_parse m_State;
};

#endif