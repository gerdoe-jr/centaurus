#ifndef __CROSTREAM_H
#define __CROSTREAM_H

#include "crotype.h"
#include "crobuffer.h"

class CroStream
{
public:
    CroStream(CroBuffer& buf);

    bool IsOverflowed(cronos_size newSize = 0) const;
    uint8_t* Read(cronos_size size);

    template<typename T>
    inline T& Read()
    {
        return *(T*)Read(sizeof(T));
    }
private:
    CroBuffer& m_Buffer;
    cronos_rel m_Pos;
};

#endif