#include "crostru.h"
#include "crobank.h"
#include "croattr.h"
#include "croprop.h"

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
            cronos_off attrStart = _stream.GetPosition();

            CroAttr attr = Parse<CroAttr>(_stream);
            if (attr.AttrName() == name)
            {
                record.Copy(_record.GetData(), _record.GetSize());
                stream = CroStream(record);
                
                stream.SetPosition(attrStart);
                return true;
            }
        }
    }

    return false;
}

bool CroStru::LoadBankProps()
{
    CroBuffer attr;
    CroStream props;
    if (!GetAttrByName(CROATTR_BANK, attr, props))
    {
        return false;
    }

    while (props.Remaining() > 0)
    {
        CroProp prop = Parse<CroProp>(props);
        CroBuffer& data = prop.Prop();

        if (prop.IsRef())
        {
            cronos_id blockId = prop.RefBlockId();
            croblock_type type = GetBlockType(blockId);
            if (type != CROBLOCK_PROP)
            {
                throw std::runtime_error("ref block is not a prop");
            }

            CroBuffer record = m_pStru->LoadRecord(blockId);
            if (record.IsEmpty())
            {
                throw std::runtime_error("prop ref block is empty");
            }

            CroStream block = CroStream(record);
            block.Read<uint8_t>();

            cronos_size propSize = block.Remaining();
            data.Write(block.Read(propSize), propSize);
        }
        else
        {
            cronos_size propSize = prop.PropSize();
            data.Write(props.Read(propSize), propSize);
        }

        Bank()->ParserStart(this);
        Bank()->OnParseProp(prop);
        Bank()->ParserEnd(this);
    }

    return true;
}
