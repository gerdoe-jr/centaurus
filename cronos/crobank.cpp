#include "crobank.h"
#include <stdexcept>
#include <win32util.h>

/* CroBank */

CroBank::CroBank(const std::wstring& path)
    : m_Path(path),
    m_TextCodePage(CRONOS_DEFAULT_CODEPAGE)
{
    m_Parser = NULL;

    m_BankFormSaveVer = 0;
    m_BankId = 0;
    m_BankSerial = 0;
    m_BankCustomProt = 0;
    m_BankVersion = 0;
}

CroBank::~CroBank()
{
    Close();
}

bool CroBank::Open()
{
    for (unsigned i = 0; i < CROFILE_COUNT; i++)
    {
        std::wstring path = JoinFilePath(m_Path, FileName((crobank_file)i));
        std::unique_ptr<CroFile> file = std::make_unique<CroFile>(path);
        try {
            file->Open();
        }
        catch (const CroException& exc) {
            OnCronosException(exc);
            file->SetError(CROFILE_ERROR);
        }

        if (!file->IsFailed())
            m_CroFile[i] = std::move(file);
        else m_CroFile[i] = NULL;
    }

    return File(CROFILE_STRU) && File(CROFILE_BANK) && File(CROFILE_INDEX);
}

void CroBank::Close()
{
    for (unsigned i = 0; i < CROFILE_COUNT; i++)
    {
        auto& file = m_CroFile[i];
        if (file)
        {
            file->Close();
            file = NULL;
        }
    }
}

bool CroBank::TryBankPath()
{
    for (unsigned i = 0; i < CROFILE_COUNT; i++)
    {
        std::wstring path = JoinFilePath(m_Path, FileName((crobank_file)i));
        std::unique_ptr<CroFile> file = std::make_unique<CroFile>(path);
        try {
            if (file->Open() != CROFILE_OK)
                return false;
        }
        catch (const CroException& exc) {
            OnCronosException(exc);
            return false;
        }
    }

    return true;
}

void CroBank::SetBankPath(const std::wstring& path)
{
    m_Path = path;
}

const std::wstring& CroBank::GetBankPath() const
{
    return m_Path;
}

void CroBank::SetTextCodePage(unsigned cp)
{
    m_TextCodePage = cp;
}

unsigned CroBank::GetTextCodePage() const
{
    return m_TextCodePage;
}

std::wstring CroBank::GetWString(const uint8_t* str, cronos_size len)
{
    throw std::runtime_error("CroBank::GetWString not implemented");
}

std::string CroBank::GetString(const uint8_t* str, cronos_size len)
{
    return std::string((const char*)str, len);
}

CroFile* CroBank::File(crobank_file file)
{
    return m_CroFile[file].get();
}

const std::wstring& CroBank::FileName(crobank_file file)
{
    static std::wstring _croFileNames[] = {
        L"CroStru",
        L"CroBank",
        L"CroIndex",
    };

    return _croFileNames[file];
}

CroBase* CroBank::GetBaseByIndex(unsigned idx)
{
    for (auto& base : m_Bases)
    {
        if (base.m_BaseIndex == idx)
            return &base;
    }

    return NULL;
}

CroParser* CroBank::Parser()
{
    return m_Parser;
}

void CroBank::ParserStart(CroParser* parser)
{
    m_Parser = parser;
}

void CroBank::ParserEnd(CroParser* parser)
{
    m_Parser = NULL;
}

void CroBank::OnCronosException(const CroException& exc)
{
    throw std::runtime_error("bank cronos exception not implemented");
}

void CroBank::OnParseProp(CroProp& prop)
{
    const std::string& name = prop.GetName();
    CroBuffer& data = prop.Prop();

    if (name == CROPROP_BANK)
    {

    }
    else if (name == CROPROP_BANKFORMSAVEVER)
        m_BankFormSaveVer = (uint32_t)atoll(prop.GetString().c_str());
    else if (name == CROPROP_BANKID)
        m_BankId = (uint32_t)atoll(prop.GetString().c_str());
    else if (name == CROPROP_BANKNAME)
        m_BankName = GetWString(data.GetData(), data.GetSize());
    else if (name.starts_with(CROPROP_BASE_PREFIX))
        m_Bases.emplace_back(Parser()->Parse<CroBase>(data));
    else if (name.starts_with(CROPROP_FORMULA_PREFIX))
        m_Formuls.emplace_back(data);
    else if (name == CROPROP_NS1)
    {
        CroPropNS ns1 = Parser()->Parse<CroPropNS>(data);
        std::string pass = ns1.SysPassword();

        m_BankSerial = ns1.Serial();
        m_BankCustomProt = ns1.CustomProt();
        m_BankSysPass = GetWString((const uint8_t*)
            pass.c_str(), pass.size());
    }
    else if (name == CROPROP_VERSION)
        m_BankVersion = atoi(prop.GetString().c_str());
}

/* CroBankParser */

CroBankParser::CroBankParser(CroBank* bank)
    : CroParser(bank, CROFILE_BANK)
{
    Reset();
}

void CroBankParser::Reset()
{
    m_Id = INVALID_CRONOS_ID;

    m_pBase = NULL;

    m_ValueOff = INVALID_CRONOS_OFFSET;
    m_ValueSize = 0;
}

void CroBankParser::Parse(cronos_id id, CroBuffer& data)
{
    m_Id = id;
    m_pData = &data;
    m_Record = CroStream(*m_pData);

    unsigned baseId = ReadIdent();
    m_pBase = Bank()->GetBaseByIndex(baseId);
    m_FieldIter = m_pBase->StartField();
}

uint8_t* CroBankParser::Value()
{
    return m_pData->GetData() + m_ValueOff;
}

cronos_off CroBankParser::ValueOff()
{
    return m_ValueOff;
}

cronos_size CroBankParser::ValueSize()
{
    return m_ValueSize;
}

CroType CroBankParser::ValueType()
{
    return m_ValueType;
}

crovalue_parse CroBankParser::ParseValue()
{
    if (m_FieldIter == m_pBase->EndField())
        return CroRecord_End;
    auto& field = *m_FieldIter;
    
    m_ValueOff = m_Record.GetPosition();
    m_ValueSize = 0;
    m_ValueType = field.m_Type;

    return CroValue_Read;
}

crovalue_parse CroBankParser::NextValue()
{
    if (!m_Record.Remaining())
        return CroRecord_End;
    
    uint8_t sep = m_Record.Read<uint8_t>();
    switch (sep)
    {
    case CROVALUE_SEP:
        if (++m_FieldIter == m_pBase->EndField())
            return CroRecord_End;
        break;
    }

    return CroValue_Next;
}

void CroBankParser::ReadValue()
{
    m_ValueOff = m_Record.GetPosition();
    
    while (m_Record.Remaining())
    {
        uint8_t value = m_Record.Get<uint8_t>();
        if (value == CROVALUE_SEP)
        {
            break;
        }
        else
        {
            m_Record.Read<uint8_t>();
        }
    }

    m_ValueSize = std::min(m_Record.GetPosition() - m_ValueOff,
        m_pData->GetSize() - m_ValueOff);
}

CroIdent CroBankParser::ReadIdent()
{
    uint8_t prefix = m_Record.Get<uint8_t>();
    if (prefix & 0xF0)
    {
        return (CroIdent)ReadInteger();
    }
    
    return m_Record.Read<uint8_t>();
}

CroInteger CroBankParser::ReadInteger()
{
    switch (m_Record.Get<uint8_t>())
    {
    case 2: return m_Record.Read<int8_t>();
    case 3: return m_Record.Read<int16_t>();
    case 4: return m_Record.Read<int32_t>();
    }

    return 0;
}
