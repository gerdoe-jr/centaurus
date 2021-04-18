#ifndef __CROATTR_H
#define __CROATTR_H

#include "crobank.h"
#include "crostream.h"
#include <string>

#define CROATTR_PREFIX 0x03
#define CROATTR_REF_PREFIX 0x04

class CroAttr
{
public:
    CroAttr();

    void Parse(ICroBank* bank, CroStream& stream);
private:
    std::string m_Name;
};

#endif