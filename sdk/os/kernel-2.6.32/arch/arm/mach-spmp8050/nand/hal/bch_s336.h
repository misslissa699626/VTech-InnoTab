
#ifndef __BCH_S336_H__
#define __BCH_S336_H__

#include "hal_base.h"

#define rBCH_S336_CFG					(*(volatile unsigned int *) (BCH_BASE_ADDRESS + 0x00))
#define rBCH_S336_DATA_PTR				(*(volatile unsigned int *) (BCH_BASE_ADDRESS + 0x04))
#define rBCH_S336_PARITY_PTR			(*(volatile unsigned int *) (BCH_BASE_ADDRESS + 0x08))
#define rBCH_S336_INT_STATUS			(*(volatile unsigned int *) (BCH_BASE_ADDRESS + 0x0C))
#define rBCH_S336_SOFT_RESET			(*(volatile unsigned int *) (BCH_BASE_ADDRESS + 0x10))
#define rBCH_S336_INT_MASK				(*(volatile unsigned int *) (BCH_BASE_ADDRESS + 0x14))
#define rBCH_S336_REPORT_STATUS			(*(volatile unsigned int *) (BCH_BASE_ADDRESS + 0x18))
#define rBCH_S336_SECTOR_ERR_REPORT		(*(volatile unsigned int *) (BCH_BASE_ADDRESS + 0x1C))
#define rBCH_S336_SECTOR_FAIL_REPORT	(*(volatile unsigned int *) (BCH_BASE_ADDRESS + 0x20))


//#define	SCU_A_BASE (0x93007000)
//#define rSCU_A_PERI_CLKEN		((volatile unsigned int *) (SCU_A_BASE + 0x04))
#define BCH_ON_OFF(x)			((x) << 13)
#define ARBITOR_ON_OFF(x)		((x) << 20)
#define AAHB_M212_ON_OFF(x)		((x) << 16)	

#define BCH_S336_ENCODE (0)
#define BCH_S336_DECODE (1)

#define BCH_S336_16_BIT (0)
#define BCH_S336_24_BIT (1)

#define BCH_S336_START					1
#define BCH_S336_ENC_DEC(x)				((x) << 4)
#define BCH_S336_CORRECT_MODE(x)		((x) << 8)
#define BCH_S336_SECTOR_NUMBER(x)		((x) << 16)

#define BCH_S336_INT  (1)
#define BCH_S336_BUSY (1 << 4)

#define BCH_S336_FINISH_MASK(x)			(x)
#define BCH_S336_DECODE_FAIL_MASK(x)	((x) << 1)

#define BCH_S336_FINISH (1)
#define BCH_S336_DECODE_FAIL (1 << 4)

void bch_s336_init(void);
void bch_s336_close(void);
int bch_init_intr(void) ;//add by king
int bch_s336_process(unsigned char *data, unsigned char *parity, int data_size, int codec_mode, int correct_mode);

#endif // !__BCH_S336_H__

