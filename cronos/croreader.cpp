#include "croreader.h"

/* CroReader */

CroReader::CroReader(CroBank* bank)
    : m_pBank(bank), m_Parser(bank)
{
    
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
    
    OnRecord();
}

void CroReader::OnRecord()
{
    // Распарсить запись
    crovalue_parse parse = m_Parser.ParseValue();

    while (parse != CroRecord_End)
    {
        OnValue();

        parse = m_Parser.NextValue();
    }

    m_Parser.Reset();
}

void CroReader::OnValue()
{
    m_Parser.ReadValue();
}
