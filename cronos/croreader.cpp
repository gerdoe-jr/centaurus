#include "croreader.h"

/* CroReader */

CroReader::CroReader(CroBank* bank)
    : m_pBank(bank), m_Parser(bank)
{
    m_State = CroRecord_Start;
}

CroReader::~CroReader()
{
}

void CroReader::ReadMap(CroRecordMap* map)
{
    for (cronos_id id = map->IdStart(); id != map->IdEnd(); id++)
    {
        if (!map->HasRecord(id)) continue;

        CroBuffer record = map->LoadRecord(id);
        ReadRecord(id, record);
    }
}

void CroReader::ReadRecord(cronos_id id, CroBuffer& record)
{
    m_Parser.Parse(id, record);
    m_State = CroRecord_Start;
    
    do {
        switch (m_State)
        {
        case CroRecord_Start:
            OnRecord();
            break;
        case CroRecord_End:
            OnRecordEnd();
            break;
        case CroValue_Read:
            OnValue();
            break;
        case CroValue_Next:
            OnValueNext();
            break;
        }
    } while (m_State != CroRecord_End);
    OnRecordEnd();

    m_Parser.Reset();
}

void CroReader::OnRecord()
{
    m_State = m_Parser.ParseValue();
}

void CroReader::OnRecordEnd()
{
}

void CroReader::OnValue()
{
    m_Parser.ReadValue();
    m_State = CroValue_Next;
}

void CroReader::OnValueNext()
{
    m_State = m_Parser.NextValue();
}
