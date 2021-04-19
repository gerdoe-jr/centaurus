#include "crostru.h"

/* CroStru */

CroStru::CroStru(CroBank* bank, CroRecordMap* stru)
    : CroParser(bank, CROFILE_STRU),
    m_pStru(stru)
{
}
