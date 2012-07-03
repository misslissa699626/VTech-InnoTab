#include <linux/kernel.h>       /* printk() */
#include <mach/regs-iis.h>

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#define IISTX_MONO    0x400000
#define IISRX_MONO    0x200000

/* TODO: why IISRX_ISCR pointer to 0x0 ? */
#define IISRX_ISCR_TMP 0xFC81D000
#define IISRX_ISDR_TMP 0xFC81D004
#define IISRX_ISSR_TMP 0xFC81D008

#define IISTX_ISCR_TMP 0xFC812000
#define IISTX_ISDR_TMP 0xFC812004
#define IISTX_ISSR_TMP 0xFC812008

#ifndef HAL_READ_UINT32
#define HAL_READ_UINT32( _register_, _value_ ) \
        ((_value_) = *((volatile unsigned int *)(_register_)))
#endif

#ifndef HAL_WRITE_UINT32
#define HAL_WRITE_UINT32( _register_, _value_ ) \
        (*((volatile unsigned int *)(_register_)) = (_value_))
#endif


/**************************************************************************
 *                            F U N C T I O N                             *
 **************************************************************************/
void
halI2sTxRegDump(
	void
)
{
	register unsigned int val;

	HAL_READ_UINT32(IISTX_ISCR_TMP, val);
	printk("TXCR: 0x%x\n", val);
}

void
halI2sRxRegDump(
	void
)
{
	register unsigned int val;

	HAL_READ_UINT32(IISRX_ISCR_TMP, val);
	printk("RXCR: 0x%x\n", val);
}

unsigned int
halI2sTxChlGet(
	void
)
{
	register unsigned int val;

	HAL_READ_UINT32(IISTX_ISCR_TMP, val);
	val = val & (IISTX_MONO);

	switch (val) {
	case 0:
		return 1;

	case 1:
		return 0;

	default:
		return val;
	}

}

void
halI2sTxChlSet(
	int ch
)
{
	register unsigned int val;

	HAL_READ_UINT32(IISTX_ISCR_TMP, val);

	if (ch == 1) {
		val = val | (IISTX_MONO);
	}
	else {
		val = val & (~IISTX_MONO);
	}

	HAL_WRITE_UINT32(IISTX_ISCR_TMP, val);
}

void
halI2sRxChlSet(
	int ch
)
{
	register unsigned int val;

	HAL_READ_UINT32(IISRX_ISCR_TMP, val);

	if (ch == 1) {
		val = val | (IISRX_MONO);
	}
	else {
		val = val & (~IISRX_MONO);
	}

	HAL_WRITE_UINT32(IISRX_ISCR_TMP, val);
}

void
halI2sTxFrameOrderSet(
	int order
)
{
	register unsigned int val;

	HAL_READ_UINT32(IISTX_ISCR_TMP, val);

	if (order == 0) {
		val = val | (IISTX_SENDMODE_LSB);
	}
	else {
		val = val & (~IISTX_SENDMODE_LSB);
	}

	HAL_WRITE_UINT32(IISTX_ISCR_TMP, val);
}

void
halI2sRxFrameOrderSet(
	int order
)
{
	register unsigned int val;

	HAL_READ_UINT32(IISRX_ISCR_TMP, val);

	if (order == 0) {
		val = val | (IISRX_SENDMODE_LSB);
	}
	else {
		val = val & (~IISRX_SENDMODE_LSB);
	}

	HAL_WRITE_UINT32(IISRX_ISCR_TMP, val);
}

void
halI2sTxFormatSizeSet(
	int size
)
{
	register unsigned int val;

	HAL_READ_UINT32(IISTX_ISCR_TMP, val);
	val = val & (~IISTX_FSMODE_MASK);

	switch (size) {

	case 0x0:
		val |= IISTX_FSMODE_16;
		break;

	case 0x1:
		val |= IISTX_FSMODE_24;
		break;

	case 0x10:
		val |= IISTX_FSMODE_32;
		break;
	}

	HAL_WRITE_UINT32(IISTX_ISCR_TMP, val);
}


void
halI2sRxFormatSizeSet(
	int size
)
{
	register unsigned int val;

	HAL_READ_UINT32(IISRX_ISCR_TMP, val);
	val = val & (~IISRX_FSMODE_MASK);

	switch (size) {

	case 0:
		val |= IISRX_FSMODE_16;
		break;

	case 1:
		val |= IISRX_FSMODE_24;
		break;

	case 2:
		val |= IISRX_FSMODE_32;
		break;
	}

	HAL_WRITE_UINT32(IISRX_ISCR_TMP, val);
}

void
halI2sTxEnable(
	void
)
{
	register unsigned int val;

	HAL_READ_UINT32(IISTX_ISCR_TMP, val);
	val = val & (~IISTX_SLAVE_MODE);
	HAL_WRITE_UINT32(IISTX_ISCR_TMP, val);

	HAL_READ_UINT32(IISTX_ISCR_TMP, val);
	val = val | (IISTX_ENABLE);
	HAL_WRITE_UINT32(IISTX_ISCR_TMP, val);
}


void
halI2sTxDisable(
	void
)
{
	register unsigned int val;

	HAL_READ_UINT32(IISTX_ISCR_TMP, val);
	val = val & (~IISTX_ENABLE);
	HAL_WRITE_UINT32(IISTX_ISCR_TMP, val);
}


void
halI2sTxFIFOClear(
	void
)
{
	register unsigned int val;

	HAL_READ_UINT32(IISTX_ISCR_TMP, val);
	val = val & (~IISTX_CLRFIFO);
	HAL_WRITE_UINT32(IISTX_ISCR_TMP, val);

	do {
		HAL_READ_UINT32(IISTX_ISSR_TMP, val);
	} while(val & IISTX_ISSR_TMP);
}


void
halI2sRxEnable(
	void
)
{
	register unsigned int val;
	int count = 0;

	do {
		HAL_READ_UINT32(IISRX_ISCR_TMP, val);
		val = val | (IISRX_ENABLE);
		HAL_WRITE_UINT32(IISRX_ISCR_TMP, val);

		HAL_READ_UINT32(IISRX_ISCR_TMP, val);
		printk("I2S Rx Enable count:%3d\n",count);
		count++;
	} while(!(val & IISRX_ENABLE));
}

void
halI2sRxDisable(
	void
)
{
	register unsigned int val;
	HAL_READ_UINT32(IISRX_ISCR_TMP, val);
	val = val & (~IISRX_ENABLE);
	HAL_WRITE_UINT32(IISRX_ISCR_TMP, val);
}


void
halI2sTxIntEnable(
	void
)
{
	register unsigned int val;
	HAL_READ_UINT32(IISTX_ISCR_TMP, val);
	val = val | (IISTX_EN_IRT);
	HAL_WRITE_UINT32(IISTX_ISCR_TMP, val); //Tx interrupt enable
}


void
halI2sTxIntDisable(
	void
)
{
	register unsigned int val;
	HAL_READ_UINT32(IISTX_ISCR_TMP, val);
	val = (val & (~IISTX_EN_IRT))|IISTX_IRT_FLAG;
	HAL_WRITE_UINT32(IISTX_ISCR_TMP, val); //Tx interrupt enable
}


void
halI2sRxIntEnable(
	void
)
{
	register unsigned int val;
	HAL_READ_UINT32(IISRX_ISCR_TMP, val);
	val = val | (IISRX_EN_IRT);
	HAL_WRITE_UINT32(IISRX_ISCR_TMP, val); //Tx interrupt enable
}


void
halI2sRxIntDisable(
	void
)
{
	register unsigned int val;
	HAL_READ_UINT32(IISRX_ISCR_TMP, val);
	val = (val & (~IISRX_EN_IRT))|IISRX_IRT_PEND;
	HAL_WRITE_UINT32(IISRX_ISCR_TMP, val); //Tx interrupt enable
}


void
halI2sRxFIFOClear(
	void
)
{
	register unsigned int val;

	HAL_READ_UINT32(IISRX_ISCR_TMP, val);
	val = val | (IISRX_CLRFIFO);
	HAL_WRITE_UINT32(IISRX_ISCR_TMP, val);

	HAL_READ_UINT32(IISRX_ISCR_TMP, val);
	val = val & (~IISRX_CLRFIFO);
	HAL_WRITE_UINT32(IISRX_ISCR_TMP, val);
}
