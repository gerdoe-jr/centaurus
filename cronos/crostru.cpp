#include "crostru.h"
#include "crobank.h"
#include "cronos02.h"

/* CroAttr */

void CroAttr::Parse(CroParser* parser, CroStream& stream)
{
    uint8_t nameLen = stream.Read<uint8_t>();
    m_Name = parser->LoadString(stream.Read(nameLen), nameLen);
}

/* CroProp */

CroProp::CroProp()
{
    m_Value = 0;
}

void CroProp::Parse(CroParser* parser, CroStream& stream)
{
    uint8_t nameLen = stream.Read<uint8_t>();
    m_Name = parser->LoadString(stream.Read(nameLen), nameLen);

    m_Value = stream.Read<uint32_t>();
}

const std::string& CroProp::GetName() const
{
    return m_Name;
}

CroBuffer& CroProp::Prop()
{
    return m_Prop;
}

std::string CroProp::GetString() const
{
    return std::string((const char*)m_Prop.GetData(), m_Prop.GetSize());
}

/* CroField */

CroField::CroField()
{
    m_Type = CroType::Ident;
    m_Index = 0;
    m_Flags = 0;
    m_DataIndex = 0;
    m_DataLength = 0;
}

void CroField::Parse(CroParser* parser, CroStream& stream)
{
    uint16_t size = stream.Read<uint16_t>();
    cronos_rel pos = stream.GetPosition();

    m_Type = (CroType)stream.Read<uint16_t>();
    m_Index = stream.Read<uint32_t>();

    uint8_t nameLen = stream.Read<uint8_t>();
    m_Name = parser->LoadString(stream.Read(nameLen), nameLen);

    m_Flags = stream.Read<uint32_t>();
    if (stream.Read<uint8_t>())
    {
        m_DataIndex = stream.Read<uint32_t>();
        m_DataLength = stream.Read<uint32_t>();
    }

    stream.SetPosition(pos + size);
}

const std::string& CroField::GetName() const
{
    return m_Name;
}

CroType CroField::GetType() const
{
    return m_Type;
}

cronos_flags CroField::GetFlags() const
{
    return m_Flags;
}

/* CroBase */

CroBase::CroBase()
{
    m_VocFlags = 0;
    m_BaseVersion = 0;
    m_BitcardId = 0;
    m_LinkedId = 0;
    m_BaseIndex = 0;
    m_Flags = 0;
}

void CroBase::Parse(CroParser* parser, CroStream& base)
{
    m_VocFlags = base.Read<uint16_t>(); // vocflags
    base.Read<uint16_t>(); // unk1
    m_BaseVersion = base.Read<uint16_t>();
    m_BitcardId = base.Read<uint32_t>();
    if (m_BaseVersion == CROBASE_LINKED)
        m_LinkedId = base.Read<uint32_t>();
    m_BaseIndex = base.Read<uint32_t>();

    uint8_t nameLen = base.Read<uint8_t>();
    m_Name = parser->LoadString(base.Read(nameLen), nameLen);

    uint8_t mcLen = base.Read<uint8_t>();
    m_Mnemocode = parser->LoadString(base.Read(mcLen), mcLen);

    m_Flags = base.Read<uint32_t>();
    uint32_t fieldNum = base.Read<uint32_t>();
    for (unsigned i = 0; i < fieldNum; i++)
    {
        CroField field;
        field.Parse(parser, base);
        m_Fields.push_back(field);
    }
}

const std::string& CroBase::GetName() const
{
    return m_Name;
}

CroField* CroBase::GetFieldByIndex(unsigned idx)
{
    for (auto it = StartField(); it != EndField(); it++)
        if (it->m_Index == idx) return &*it;
    return NULL;
}

CroField* CroBase::GetFieldByDataIndex(unsigned idx)
{
    for (auto it = StartField(); it != EndField(); it++)
        if (it->m_DataIndex == idx) return &*it;
    return NULL;
}

/* CroPropNS */

CroPropNS::CroPropNS()
{
    m_Serial = 0;
    m_CustomProt = 0;
}

void CroPropNS::Parse(CroParser* parser, CroStream& stream)
{
    CroFile* stru = parser->File();
    CroData crypt = CroData(stru, INVALID_CRONOS_ID,
        cronos02_crypt_table.m_pMem, cronos02_crypt_table.m_Size);

    CroBuffer in, out;

    cronos_size propSize = stream.Remaining();
    in.Write(stream.Read(propSize), propSize);

    CroStream ns(in);
    ns.Read<uint8_t>();
    uint8_t prefix = ns.Read<uint8_t>();

    out.Write(ns.Read(12), 12);
    stru->Decrypt(out, prefix, &crypt);
    CroStream hdr(out);

    m_Serial = hdr.Read<uint32_t>();
    m_CustomProt = hdr.Read<uint32_t>();

    uint32_t passLen = hdr.Read<uint32_t>();
    if (passLen)
    {
        CroBuffer pass;
        pass.Write(ns.Read(passLen), passLen);
        stru->Decrypt(pass, prefix + 0x0C, &crypt);
        m_SysPass = parser->LoadString(pass.GetData(), passLen);
    }
}

/* CroStruParser */

CroStruParser::CroStruParser(CroBank* bank, CroRecordMap* stru)
    : CroParser(bank, CROFILE_STRU),
    m_pStru(stru)
{
}

croblock_type CroStruParser::GetBlockType(cronos_id blockId)
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

bool CroStruParser::GetAttrByName(const std::string& name,
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

bool CroStruParser::LoadBankProps()
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
