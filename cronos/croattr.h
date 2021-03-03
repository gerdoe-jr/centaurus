#ifndef __CROATTR_H
#define __CROATTR_H

#include "crostream.h"
#include "crobuffer.h"
#include <string>

class CroAttr
{
public:
    CroAttr();

    const std::string& GetName() const;
    CroBuffer& GetAttr();
    std::string GetString() const;

    void Parse(CroStream& stream);
    inline bool IsEntryId() const { return m_bIsEntryId; }
private:
    std::string m_AttrName;
    CroBuffer m_Attr;
    bool m_bIsEntryId;
};

#endif