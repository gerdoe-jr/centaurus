#include "crostream.h"
#include <stdexcept>
#include <string.h>
#include <string>

CroStream::CroStream()
{
    m_pBuffer = NULL;
    m_Pos = 0;
}

CroStream::CroStream(CroBuffer& buf)
{
    m_pBuffer = &buf;
    m_Pos = 0;
}

bool CroStream::IsOverflowed() const
{
    return m_Pos >= m_pBuffer->GetSize();
}

uint8_t* CroStream::Read(cronos_size size)
{
    if (m_Pos + size > m_pBuffer->GetSize())
    {
        throw std::runtime_error("CroStream overflow (read " 
            + std::to_string(size) + " at pos " 
            + std::to_string(m_Pos)
            + " in buffer " + std::to_string(m_pBuffer->GetSize())
            + " total size"
        + ")");
    }

    uint8_t* data = m_pBuffer->GetData() + m_Pos;
    m_Pos += size;
    return data;
}

void CroStream::Write(const uint8_t* src, cronos_size size)
{
    if (m_Pos + size > m_pBuffer->GetSize())
    {
        throw std::runtime_error("CroStream overflow (write "
            + std::to_string(size) + " at pos "
            + std::to_string(m_Pos)
            + " in buffer " + std::to_string(m_pBuffer->GetSize())
            + " total size"
            + ")");
    }

    memcpy(m_pBuffer->GetData() + m_Pos, src, size);
    m_Pos += size;
}
