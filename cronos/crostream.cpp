#include "crostream.h"
#include <stdexcept>
#include <string>

CroStream::CroStream(CroBuffer& buf)
    : m_Buffer(buf), m_Pos(0)
{
}

bool CroStream::IsOverflowed() const
{
    return m_Pos >= m_Buffer.GetSize();
}

uint8_t* CroStream::Read(cronos_size size)
{
    if (m_Pos + size > m_Buffer.GetSize())
    {
        throw std::runtime_error("CroStream overflow (read " 
            + std::to_string(size) + " at pos " 
            + std::to_string(m_Pos)
            + " in buffer " + std::to_string(m_Buffer.GetSize())
            + " total size"
        + ")");
    }
    uint8_t* data = m_Buffer.GetData() + m_Pos;
    m_Pos += size;
    return data;
}
