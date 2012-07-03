#ifndef _TABINIT_H_
#define _TABINIT_H_

#include "mpg123.h"

extern real decwin[512+32];
extern real *pnts[5];
void make_decode_tables(long scale);

#endif /* _TABINIT_H_ */


