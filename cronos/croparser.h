#ifndef __CROPARSER_H
#define __CROPARSER_H

#include "crofile.h"
#include "crostream.h"
#include "cronos_abi.h"
#include "croexception.h"
#include <stdexcept>
#include <string>

class CroBank;
class CroParser;

class ICroParsee
{
public:
    virtual void Parse(CroParser* parser, CroStream& stream) = 0;
};

class CroParser
{
public:
    CroParser(CroBank* bank, crobank_file file);

    inline CroBank* Bank() { return m_pBank; }
    inline CroFile* File() { return m_pFile; }
    inline const CronosABI* ABI() { return m_pFile->ABI(); }
    inline ICroParsee* Parsee() { return m_pParsee; }

    std::string LoadString(const uint8_t* str, cronos_size len);
    std::wstring LoadWString(const uint8_t* str, cronos_size len);

    virtual void Parse(ICroParsee* parsee, CroStream& stream);

    template<typename T>
    inline T Parse(CroStream& stream)
    {
        T value = T();

        auto* parsee = dynamic_cast<ICroParsee*>(&value);
        if (!parsee) throw CroException(m_pFile, "not a parsee");

        Parse(parsee, stream);
        return value;
    }

    template<typename T>
    inline T Parse(CroBuffer& buffer)
    {
        T value = T();
        CroStream stream = CroStream(buffer);
        
        auto* parsee = dynamic_cast<ICroParsee*>(&value);
        if (!parsee) throw CroException(m_pFile, "not a parsee");

        Parse(parsee, stream);
        return value;
    }
protected:
    virtual void StartParsing(ICroParsee* parsee);
    virtual void EndParsing();
private:
    CroBank* m_pBank;
    CroFile* m_pFile;
    ICroParsee* m_pParsee;
};

#endif