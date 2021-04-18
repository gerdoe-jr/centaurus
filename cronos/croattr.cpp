#include "croattr.h"

/* CroAttr */

void CroAttr::Parse(CroParser* parser, CroStream& stream)
{
    uint8_t nameLen = stream.Read<uint8_t>();
    m_Name = parser->LoadString(stream.Read(nameLen), nameLen);
}
