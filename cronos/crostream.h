#ifndef __CROSTREAM_H
#define __CROSTREAM_H

#include "crotype.h"
#include "crobuffer.h"

class CroStream
{
public:
    CroStream();
    CroStream(CroBuffer& buf);

    bool IsOverflowed() const;
    uint8_t* Read(cronos_size size);

    template<typename T> inline T& Get()
    {
        return *(T*)m_pBuffer->GetData();
    }

    template<typename T> inline T& Read()
    {
        return *(T*)Read(sizeof(T));
    }

    inline cronos_rel GetPosition() const { return m_Pos; }
    inline void SetPosition(cronos_rel pos) { m_Pos = pos; }
    inline cronos_size Remaining() const
    {
        return IsOverflowed() ? 0 : m_pBuffer->GetSize() - GetPosition();
    }
private:
    CroBuffer* m_pBuffer;
    cronos_rel m_Pos;
};

#endif