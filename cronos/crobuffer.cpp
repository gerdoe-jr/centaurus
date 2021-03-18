#include "crobuffer.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdexcept>
#include <algorithm>

CroBuffer::CroBuffer()
{
    InitBuffer(NULL, 0, false);
}

CroBuffer::CroBuffer(const uint8_t* data, cronos_size size)
{
    InitBuffer((uint8_t*)data, size, false);
}

CroBuffer::CroBuffer(uint8_t* data, cronos_size size, bool owner)
{
    InitBuffer(data, size, owner);
}

CroBuffer::CroBuffer(const CroBuffer& other)
{
    InitBuffer(NULL, 0, false);

    cronos_size size = other.GetSize();
    if (size)
    {
        Alloc(size);
        memcpy(GetData(), other.GetData(), size);
    }
    m_bOwner = true;
}

CroBuffer::CroBuffer(CroBuffer&& other) noexcept
{
    InitBuffer(other.m_pData, other.m_uSize, other.m_bOwner);

    other.m_bOwner = false;
}

CroBuffer::~CroBuffer()
{
    Free();
}

void CroBuffer::InitBuffer(uint8_t* data,
        cronos_size size, bool owner)
{
    m_pData = data;
    m_uSize = size;
    m_bOwner = owner;
}

cronos_size CroBuffer::GetSize() const
{
    return m_uSize;
}

bool CroBuffer::IsEmpty() const
{
    return !m_pData || m_uSize == 0;
}

const uint8_t* CroBuffer::GetData() const
{
    return m_pData;
}

uint8_t* CroBuffer::GetData()
{
    return m_pData;
}

void CroBuffer::Alloc(cronos_size size)
{
    if (!size) throw std::runtime_error("CroBuffer alloc !size");

    if (IsEmpty()) m_pData = (uint8_t*)malloc(size);
    else
    {
        uint8_t* newData = (uint8_t*)realloc(m_pData, size);
        if (newData) m_pData = newData;
        else Free();
    }

    if (!m_pData)
        throw std::runtime_error("CroBuffer::Alloc !m_pData");
    m_uSize = size;
    m_bOwner = true;
}

void CroBuffer::Copy(const uint8_t* data, cronos_size size)
{
    Free();
    Alloc(size);
    memcpy(m_pData, data, size);
}

void CroBuffer::Write(const uint8_t* data, cronos_size size)
{
    cronos_off off = GetSize();
    Alloc(m_uSize + size);
    memcpy(m_pData + off, data, size);
}

void CroBuffer::Free()
{
    if (m_pData)
    {
        if (m_bOwner) free(m_pData);
        m_pData = NULL;
    }
    m_uSize = 0;
}

