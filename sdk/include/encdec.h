#ifndef ENCDEC_H
#define ENCDEC_H


#include "typedef.h"


typedef struct blockEncDec_s {
	const UINT8 *phrase;
	UINT32 phraseIdx;
} blockEncDec_t;


UINT32 encDecInit(blockEncDec_t *pEncDec, const UINT8 *phrase);
UINT32 encDecDoEnc(blockEncDec_t *pEncDec, UINT8 *pData, UINT32 dataLen);
UINT32 encDecDoDec(blockEncDec_t *pEncDec, UINT8 *pData, UINT32 dataLen);


#endif
