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

        try {
            CroBuffer record = map->LoadRecord(id);
            ReadRecord(id, record);
        }
        catch (const std::exception& e) {
            fprintf(stderr, "CroRedaer record %"
                FCroId ": %s\n", id, e.what());
        }
    }
}

void CroReader::ReadRecord(cronos_id id, CroBuffer& record)
{
    m_State = CroRecord_Start;
    
    do {
        switch (m_State)
        {
        case CroRecord_Start:
            m_Parser.Parse(id, record);
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
}

void CroReader::OnRecord()
{
    m_State = CroValue_Read;
}

void CroReader::OnRecordEnd()
{
    m_Parser.Reset();
}

void CroReader::OnValue()
{
    m_State = m_Parser.ParseValue();
    if (m_State == CroValue_Multi)
        m_State = CroValue_Next;
}

void CroReader::OnValueNext()
{
    m_State = CroValue_Read;
}
