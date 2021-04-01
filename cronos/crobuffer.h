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
    CroBuffer(CroBuffer&&) noexcept;
    ~CroBuffer();

    CroBuffer& operator=(CroBuffer&& other) noexcept
    {
        if(&other == this) return *this;
        Free();

        InitBuffer(other.m_pData, other.m_uSize, other.m_bOwner);

        other.m_bOwner = false;
        return *this;
    }

    CroBuffer& operator=(const CroBuffer& other)
    {
        if (&other == this) return *this;
        Free();

        InitBuffer(other.m_pData, other.m_uSize, false);
        return *this;
    }

    void InitBuffer(uint8_t* data, cronos_size size, bool owner);

    cronos_size GetSize() const;
    bool IsEmpty() const;
    const uint8_t* GetData() const;
    uint8_t* GetData();

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
