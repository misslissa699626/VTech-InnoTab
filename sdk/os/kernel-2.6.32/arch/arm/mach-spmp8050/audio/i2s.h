/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

void halI2sTxRegDump(void);
void halI2sRxRegDump(void);
unsigned int halI2sTxChlGet(void);
void halI2sTxChlSet(int ch);
void halI2sRxChlSet(int ch);
void halI2sTxFrameOrderSet(int order);
void halI2sRxFrameOrderSet(int order);
void halI2sTxFormatSizeSet(int size);
void halI2sRxFormatSizeSet(int size);
void halI2sTxEnable(void);
void halI2sTxDisable(void);
void halI2sTxFIFOClear(void);
void halI2sRxEnable(void);
void halI2sRxDisable(void);
void halI2sTxIntEnable(void);
void halI2sTxIntDisable(void);
void halI2sRxIntEnable(void);
void halI2sRxIntDisable(void);
void halI2sRxFIFOClear(void);
