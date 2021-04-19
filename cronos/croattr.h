#ifndef __CROATTR_H
#define __CROATTR_H

#include "croparser.h"
#include <string>

class CroAttr : public ICroParsee
{
public:
    virtual void Parse(CroParser* parser, CroStream& stream);

    inline const std::string& AttrName() const { return m_Name; }
private:
    std::string m_Name;
};

#endif