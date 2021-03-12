#ifndef __CROSTREAM_H
#define __CROSTREAM_H

#include "crotype.h"
#include "crobuffer.h"

class CroStream
{
public:
    CroStream(CroBuffer& buf);

    bool IsOverflowed() const;
    uint8_t* Read(cronos_size size);

    template<typename T>
    inline T& Read()
    {
        return *(T*)Read(sizeof(T));
    }

    inline cronos_rel GetPosition() const { return m_Pos; }
    inline void SetPosition(cronos_rel pos) { m_Pos = pos; }
    inline cronos_size Remaining() const
    {
        return IsOverflowed() ? 0 : m_Buffer.GetSize() - GetPosition();
    }
private:
    CroBuffer& m_Buffer;
    cronos_rel m_Pos;
};

#endif