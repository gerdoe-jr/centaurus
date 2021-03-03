﻿#include "crostream.h"
#include <stdexcept>

CroStream::CroStream(CroBuffer& buf)
    : m_Buffer(buf), m_Pos(0)
{
}

bool CroStream::IsOverflowed(cronos_size newSize) const
{
    return m_Pos + newSize > m_Buffer.GetSize();
}

uint8_t* CroStream::Read(cronos_size size)
{
    if (IsOverflowed(size))
        throw std::runtime_error("CroStream overflow");
    uint8_t* data = m_Buffer.GetData() + m_Pos;
    m_Pos += size;
    return data;
}