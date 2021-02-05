#ifndef __CROBUFFER_H
#define __CROBUFFER_H

#include "crotype.h"

class CroBuffer
{
public:
    CroBuffer();
    CroBuffer(const uint8_t* data, cronos_size size);
    CroBuffer(uint8_t* data, cronos_size size, bool owner = true);
    CroBuffer(const CroBuffer&);
    CroBuffer(CroBuffer&&);
    ~CroBuffer();

    CroBuffer& operator=(CroBuffer&& other) noexcept
    {
        if(&other == this) return *this;
        Free();

        InitBuffer(other.m_pData, other.m_uSize, other.m_bOwner);

        other.m_bOwner = false;
        return *this;
    }

    void InitBuffer(uint8_t* data, cronos_size size,
            bool owner = true);

    inline cronos_size GetSize() const { return m_uSize; }
    inline bool IsEmpty() const { return GetSize() == 0; }
    inline const uint8_t* GetData() const { return m_pData; }
    inline uint8_t* GetData() { return m_pData; }

    void Alloc(cronos_size);
    void Copy(const uint8_t* data, cronos_size size);
    void Write(const uint8_t* data, cronos_size size);
    void Free();
private:
    uint8_t* m_pData;
    cronos_size m_uSize;
    bool m_bOwner;
};


#endif
