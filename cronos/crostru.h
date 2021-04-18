#ifndef __CROSTRU_H
#define __CROSTRU_H

#include "croparser.h"
#include "crorecord.h"

class CroStru : public CroParser
{
public:
    CroStru(CroBank* bank, CroRecordMap* stru);

    inline CroRecordMap* CroStruMap() { return m_pStru; }
private:
    CroRecordMap* m_pStru;
};

#endif