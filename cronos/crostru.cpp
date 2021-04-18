#include "crostru.h"

/* CroStru */

CroStru::CroStru(CroBank* bank, CroRecordMap* stru)
    : CroParser(bank, CroBank::Stru),
    m_pStru(stru)
{
}
