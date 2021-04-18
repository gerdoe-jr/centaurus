#include "croparser.h"

/* CroParser */

CroParser::CroParser(CroBank* bank, crobank_file file)
{
    m_pBank = bank;
    m_pFile = Bank()->File(file);
    m_pParsee = NULL;
}

std::string CroParser::LoadString(const uint8_t* str, cronos_size len)
{
    return Bank()->GetString(str, len);
}

std::wstring CroParser::LoadWString(const uint8_t* str, cronos_size len)
{
    return Bank()->GetWString(str, len);
}

void CroParser::Parse(ICroParsee* parsee, CroStream& stream)
{
    StartParsing(parsee);
    try {
        m_pParsee->Parse(this, stream);
    }
    catch (const CroException& ce) {
        EndParsing();
        throw std::runtime_error("parser error: "
            + std::string(ce.what()));
    }
    catch (const std::exception& ex) {
        EndParsing();
        throw std::runtime_error("parser error: "
            + std::string(ex.what()));
    }
    EndParsing();
}

void CroParser::StartParsing(ICroParsee* parsee)
{
    m_pParsee = parsee;
    m_pBank->ParserStart(this);
}

void CroParser::EndParsing()
{
    m_pBank->ParserEnd(this);
    m_pParsee = NULL;
}
