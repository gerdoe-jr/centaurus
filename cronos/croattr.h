#ifndef __CROATTR_H
#define __CROATTR_H

#include "croparser.h"
#include <string>

#define CROATTR_PREFIX 0x03
#define CROATTR_REF_PREFIX 0x04

class CroAttr : public ICroParsee
{
public:
    virtual void Parse(CroParser* parser, CroStream& stream);

    inline const std::string& AttrName() const { return m_Name; }
private:
    std::string m_Name;
};

#endif