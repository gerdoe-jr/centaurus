#include "crostru.h"
#include "croattr.h"

/* CroStru */

CroStru::CroStru(CroBank* bank, CroRecordMap* stru)
    : CroParser(bank, CROFILE_STRU),
    m_pStru(stru)
{
}

croblock_type CroStru::GetBlockType(cronos_id blockId)
{
    if (!m_pStru->HasRecord(blockId))
    {
        throw std::runtime_error("invalid block");
    }

    auto map = m_pStru->PartMap();
    CroRecord& record = map[blockId];
    
    auto& part = record.RecordParts().front();
    CroData block = File()->Read(blockId, CRONOS_DAT,
        part.m_PartOff, part.m_PartSize);
    File()->Decrypt(block, blockId);

    return (croblock_type)block.Get<uint8_t>(0x00);
}

bool CroStru::GetAttrByName(const std::string& name,
    CroBuffer& record, CroStream& stream)
{
    for (cronos_id id = m_pStru->IdStart(); id != m_pStru->IdEnd(); id++)
    {
        if (!m_pStru->HasRecord(id)) continue;
        
        if (GetBlockType(id) == CROBLOCK_ATTR)
        {
            CroBuffer _record = m_pStru->LoadRecord(id);
            CroStream _stream = CroStream(_record);

            _stream.Read<uint8_t>();
            CroAttr attr = Parse<CroAttr>(_stream);

            if (attr.AttrName() == name)
            {
                record.Copy(_record.GetData(), _record.GetSize());
                stream = _stream;
                
                return true;
            }
        }
    }

    return false;
}
