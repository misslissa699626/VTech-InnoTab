/*=============================================================================
* Copyright 2002-2007, 2009 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
* Supported IP  : Memory Stick PRO-HG Host Controller IP(smshc_i)
*------------------------------------------------------------------------------
* FILENAME      : msproal_user.h
*
* DESCRIPTION   : MSPROAL_USER layer - Declaration prototype
=============================================================================*/
#ifndef     __MSPROAL_USER_H
#define     __MSPROAL_USER_H

#ifdef __cplusplus
extern "C" {
#endif

#define MSPROAL_DIRECT_ACCESS   1

#define MS_DMA_MODE 0
#define MS_CPU_MODE 1

//COMMAND 
#define CMDCODE_MASK	0x0000000F
#define STPCMD			0x00000040
#define RUNCMD			0x00000080
#define CMDARG_MASK		0x0000FF00
#define EXWRITEREG		0xFFFF0000

//STATUS
#define BUFEMPTY		0x00000001
#define BUFFULL			0x00000002
#define REGBUFEMPTY		0x00000004
#define REGBUFFULL		0x00000008
#define CMDCOM			0x00000010
#define CARDDET			0x00000020
#define TIMEOUT			0x00000040
#define CRCERR			0x00000080
#define BUSY			0x00000100
#define CARDINS			0x00000200
#define MSINT_CED		0x00000400
#define MSINT_ERR		0x00000800
#define MSINT_BREQ		0x00001000
#define MSINT_CMDNK		0x00002000

//INTEN
#define INTEN_BUFEMPTY			0x00000001
#define INTEN_BUFFULL			0x00000002
#define INTEN_REGBUFEMPTY		0x00000004
#define INTEN_REGBUFFULL		0x00000008
#define INTEN_CMDCOM			0x00000010
#define INTEN_CARDDET			0x00000020
#define INTEN_MSINT_CED			0x00000400
#define INTEN_MSINT_ERR			0x00000800
#define INTEN_MSINT_BREQ		0x00001000
#define INTEN_MSINT_CMDNK		0x00002000

//TPC (Transfer Protocol Command)
#define READ_LONG_DATA			0x00000002
#define READ_SHORT_DATA			0x00000003
#define READ_REG				0x00000004
#define GET_INT					0x00000007
#define WRITE_LONG_DATA			0x0000000D
#define WRITE_SHORT_DATA		0x0000000C
#define WRITE_REG				0x0000000B
#define SET_RW_REG_ADRS			0x00000008
#define SET_CMD					0x0000000E
#define EX_SET_CMD				0x00000009

//Command Code MS
#define MS_BLOCK_READ			0xAA
#define MS_BLOCK_WRITE			0x55
#define MS_BLOCK_END			0x33
#define MS_BLOCK_ERASE			0x99
#define MS_FLASH_STOP			0xCC
#define MS_SLEEP				0x5A
#define MS_CLEAR_BUF			0xC3
#define MS_RESET				0x3C

//Command Code MS PRO
#define MSPRO_READ_DATA			0x20
#define MSPRO_WRITE_DATA		0x21
#define MSPRO_READ_ATRB			0x24
#define MSPRO_STOP				0x25
#define MSPRO_ERASE				0x26
#define MSPRO_SET_IBD			0x46
#define MSPRO_GET_IBD			0x47
#define MSPRO_FORMAT			0x10
#define MSPRO_SLEEP				0x11

//INT
#define CED						0x80
#define ERR						0x40
#define BREQ					0x20
#define CMDNK					0x01

/**************************
    Base Address
 **************************/
#define INT_ADRS_BASE           0x80008000  /* INT Controller Address Base  */
#define GPIO3_ADRS_BASE         0x80013000  /* GPIO3 Address Base           */
#define TIMER_ADRS_BASE         0x80048000  /* Timer1 Address Base          */
#define DMAC_ADRS_BASE          0x80200000  /* DMAC Address Base            */
#define ICON_ADRS_BASE          0x80300000  /* ICON Address Base */
#define MSIF_ADRS_BASE          0x80300030  /* MSIF Address Base            */
#define PCOOL_PPORT_ADRS_BASE   0x80300040  /* PPORT Address Base */

/**************************
    Interrupt Controller
 **************************/
#define IEIS(adrs)              ((volatile unsigned long *)((adrs) + 0x0014))
#define IPR0(adrs)              ((volatile unsigned long *)((adrs) + 0x0030))
#define IPR1(adrs)              ((volatile unsigned long *)((adrs) + 0x0034))
#define IPR2(adrs)              ((volatile unsigned long *)((adrs) + 0x0038))
#define IPR3(adrs)              ((volatile unsigned long *)((adrs) + 0x003C))
#define IPR4(adrs)              ((volatile unsigned long *)((adrs) + 0x0040))
#define IPR5(adrs)              ((volatile unsigned long *)((adrs) + 0x0044))
#define IPR6(adrs)              ((volatile unsigned long *)((adrs) + 0x0048))
#define IPR7(adrs)              ((volatile unsigned long *)((adrs) + 0x004C))
#define IPR8(adrs)              ((volatile unsigned long *)((adrs) + 0x0050))
#define IPR9(adrs)              ((volatile unsigned long *)((adrs) + 0x0054))
#define IPR10(adrs)             ((volatile unsigned long *)((adrs) + 0x0058))
#define IPR11(adrs)             ((volatile unsigned long *)((adrs) + 0x005C))
#define IPR12(adrs)             ((volatile unsigned long *)((adrs) + 0x0060))
#define IPR13(adrs)             ((volatile unsigned long *)((adrs) + 0x0064))
#define ICPR(adrs)              ((volatile unsigned long *)((adrs) + 0x0074))

#define IEIS_ENABLE_DMA0        0x00000008
#define IEIS_ENABLE_DMA1        0x00000010
#define IEIS_ENABLE_DMA2        0x00000020
#define IEIS_ENABLE_MSIF        0x00000400
#define IEIS_ENABLE_ICON        0x00000400
#define IEIS_ENABLE_GPIO        0x00000800
#define IEIS_ENABLE_PPORT       0x00000800
#define IPR1_DMA0_PRI11         0x000000B0 /* DMA1 priority 11 */
#define IPR2_DMA1_PRI11         0x0000000B /* DMA1 priority 11 */
#define IPR2_DMA2_PRI11         0x000000B0 /* DMA1 priority 11 */
#define IPR5_MSIF_PRI12         0x0000000C /* MSIF priority 12 */
#define IPR5_ICON_PRI12         0x0000000C /* I-CON priority 12 */
#define IPR5_GPIO_PRI12         0x000000C0 /* pport priority 12 */
#define IPR5_PPORT_PRI12        0x000000C0 /* pport priority 12 */
#define ICPR_DEFAULT_PRI        0x0000000A /* default priority 10 */

/**************************
      Parallel Port
 **************************/
#define PCOOL_PP_DATA(adrs)     ((volatile unsigned short *)((adrs) + 0x0000))
#define PCOOL_PP_CTRL(adrs)     ((volatile unsigned short *)((adrs) + 0x0004))

#define PCOOL_PP_PIEN           0xF000
#define PCOOL_PP_INTEN          0x0100
#define PCOOL_PP_POEN           0x00F0
#define PCOOL_PP_ENABLE_ICON    0x0070

#define PCOOL_PP_P0             0x8000
#define PCOOL_PP_P1             0x4000
#define PCOOL_PP_PWRCTRL        0x0010
#define PCOOL_PP_INS            0x1000

#define PCOOL_DIV_CLK_LOW_SPEED 0x0040
#define PCOOL_DIV_CLK_MASK      0x00C0

/**************************
    GPIO
 **************************/
#define GPIO_ID(adrs)           ((volatile unsigned long *)((adrs) + 0x00))
#define GPIO_DATA(adrs)         ((volatile unsigned long *)((adrs) + 0x04))
#define GPIO_DIR_CTRL(adrs)     ((volatile unsigned long *)((adrs) + 0x08))
#define GPIO_CTRL0(adrs)        ((volatile unsigned long *)((adrs) + 0x0C))
#define GPIO_CTRL1(adrs)        ((volatile unsigned long *)((adrs) + 0x10))
#define GPIO_CTRL2(adrs)        ((volatile unsigned long *)((adrs) + 0x14))
#define GPIO_STTS0(adrs)        ((volatile unsigned long *)((adrs) + 0x18))
#define GPIO_STTS1(adrs)        ((volatile unsigned long *)((adrs) + 0x1C))

#define DATA_CLOCK_SERIAL       0x00000040  /* Colck 20MHz                  */
#define DATA_CLOCK_PARALLEL     0x00000000  /* Colck 40MHz                  */
#define DATA_CLOCK_MASK         0x000000C0  /* Colck Mask                   */
#define DATA_MSPWR              0x00000020
#define DATA_INS                0x00000010  /* INS                          */
#define DIR_CTRL_CLOCK          0x000000C0
#define DIR_CTRL_MSPWR          0x00000020
#define CTRL0_INTEN             0x00000001  /* GPIO INT(XINT_GPIO) Enable   */
#define CTRL_INS                0x00000010  /* FEN4 Enable                  */
#define STTS_INS                0x00000010  /* INS Interrupt                */
#define STTS_FSTS               0x00000000  /* XINT_GPIO Clear              */

/**************************
    DMA Controller
 **************************/
#define SADR0(adrs)             ((volatile unsigned long *)((adrs) + 0x10))
#define DADR0(adrs)             ((volatile unsigned long *)((adrs) + 0x14))
#define TCNT0(adrs)             ((volatile unsigned long *)((adrs) + 0x18))
#define DMACS0(adrs)            ((volatile unsigned long *)((adrs) + 0x1C))
#define SADR1(adrs)             ((volatile unsigned long *)((adrs) + 0x20))
#define DADR1(adrs)             ((volatile unsigned long *)((adrs) + 0x24))
#define TCNT1(adrs)             ((volatile unsigned long *)((adrs) + 0x28))
#define DMACS1(adrs)            ((volatile unsigned long *)((adrs) + 0x2C))
#define SADR2(adrs)             ((volatile unsigned long *)((adrs) + 0x30))
#define DADR2(adrs)             ((volatile unsigned long *)((adrs) + 0x34))
#define TCNT2(adrs)             ((volatile unsigned long *)((adrs) + 0x38))
#define DMACS2(adrs)            ((volatile unsigned long *)((adrs) + 0x3C))
#define DMAEN(adrs)             ((volatile unsigned long *)((adrs) + 0x50))

#define DMACS_RS_IRQ0           0x04000000  /* sms2ip - DataRegister    */
#define DMACS_RS_IRQ1           0x05000000  /* I-CON - GDFIFO           */
#define DMACS_RS_IRQ2           0x06000000  /* I-CON - PageBuffer       */
#define DMACS_SAR_INC           0x00200000  /* SAR INCREMENT */
#define DMACS_DAR_INC           0x00080000  /* DAR INCREMENT */
#define DMACS_TSZ_4BYTE         0x00040000  /* Transfer Data Size: 4BYTE */
#define DMACS_TSZ_16BYTE        0x00060000  /* Transfer Data Size: 16BYTE */
#define DMACS_TBM_NORM          0x00000000  /* Transfer Bus Mode: Normal */
#define DMACS_TBM_BURST         0x0000C000  /* Transfer Bus Mode: Burst */
#define DMACS_RFCT              0x00000040  /* Request Flag Clear Timing */
#define DMACS_IF                0x00000002
#define DMACS_CHAE              0x00000001  /* Channel Enable */
#define DMACS_CLEAR             0x00000000
#define DMAEN_DMAE              0x00000001  /* DMA Enable */

#define TUID(adrs)              ((volatile unsigned long *)((adrs) + 0x0000))
#define TUIEN(adrs)             ((volatile unsigned long *)((adrs) + 0x0004))
#define TUINT(adrs)             ((volatile unsigned long *)((adrs) + 0x0008))
#define TUCAP(adrs)             ((volatile unsigned long *)((adrs) + 0x000C))
#define TUCMP(adrs)             ((volatile unsigned long *)((adrs) + 0x0010))
#define TUD(adrs)               ((volatile unsigned long *)((adrs) + 0x0014))
#define TUCTL(adrs)             ((volatile unsigned long *)((adrs) + 0x0018))

/****************************
    Timer Interrupt Enable Register
*****************************/
#define TUIEN_IENCAP            0x00000001
#define TUIEN_IENCMP            0x00000004

/****************************
    Timer Control Register
*****************************/
#define TUCTL_TUSL              0x00000001
#define TUCTL_TUCN              0x00000002
#define TUCTL_TUST              0x00000004
#define TUCTL_CAEN              0x00000080

/************************************
    Timer Interrupt Request Register
*************************************/
#define TUINT_ICLCMP            0x00000040
#define TUINT_ICLCAP            0x00000010
#define TUINT_ISTCMP            0x00000004
#define TUINT_ISTCAP            0x00000001

#define ALL_CLEAR               0x00000000

/* The number of counts calculated from timer value(ms) */
#define COUNT_NUMBER(time)      (unsigned long)(((time) * 25000UL))

/******************************************************************************
    Timeout Value
******************************************************************************/
/*--    COMMON                      --*/
#define MSPROAL_TIMEOUT_RDY                     1
#define MSPROAL_TIMEOUT_RST                     1
#define MSPROAL_TIMEOUT_DMA                     10
#define MSPROAL_TIMEOUT_READ_REG                5
#define MSPROAL_TIMEOUT_WRITE_REG               10
#define MSPROAL_TIMEOUT_SET_RW_REG_ADRS         1
#define MSPROAL_TIMEOUT_READ_PAGE_DATA          5
#define MSPROAL_TIMEOUT_READ_QUAD_LONG_DATA     10
#define MSPROAL_TIMEOUT_WRITE_PAGE_DATA         10
#define MSPROAL_TIMEOUT_WRITE_QUAD_LONG_DATA    20
#define MSPROAL_TIMEOUT_READ_MG_STTS_REG        1
#define MSPROAL_TIMEOUT_READ_MGD_REG            2
#define MSPROAL_TIMEOUT_WRITE_MGD_REG           2
#define MSPROAL_TIMEOUT_GET_INT                 1
#define MSPROAL_TIMEOUT_SET_CMD                 1
#define MSPROAL_TIMEOUT_EX_SET_CMD              1
#define MSPROAL_TIMEOUT_READ_SHORT_DATA         1
#define MSPROAL_TIMEOUT_WRITE_SHORT_DATA        1
#define MSPROAL_TIMEOUT_READ_IO_DATA            10
#define MSPROAL_TIMEOUT_WRITE_IO_DATA           10
/*--    MS V1.X                     --*/
#define MS_TIMEOUT_BLOCK_ERASE      100
#define MS_TIMEOUT_BLOCK_READ       5
#define MS_TIMEOUT_BLOCK_WRITE      10
#define MS_TIMEOUT_FLASH_STOP       5
#define MS_TIMEOUT_SLEEP            1
#define MS_TIMEOUT_CLEAR_BUF        1
#define MS_TIMEOUT_MS_COMMAND       10
#define MS_TIMEOUT_MG_COMMAND       10
/*--    MS PRO                      --*/
#define MS2_TIMEOUT_WAKEUP          500
#define MS2_TIMEOUT_READ_DATA       1000
#define MS2_TIMEOUT_WRITE_DATA      2000
#define MS2_TIMEOUT_READ_ATRB       1000
#define MS2_TIMEOUT_ERASE           1000
#define MS2_TIMEOUT_READ_2K_DATA    1000
#define MS2_TIMEOUT_WRITE_2K_DATA   2000
#define MS2_TIMEOUT_SLEEP           1000
#define MS2_TIMEOUT_CHG_POWER_CLS   1000
#define MS2_TIMEOUT_STOP            3000
#define MS2_TIMEOUT_FORMAT          3000
/*--    I-CON                       --*/
#define MSPROAL_TIMEOUT_ICON        20000

/******************************************************************************
    Declaration prototype
******************************************************************************/
extern void msproal_user_start_timer(SINT);
extern SINT msproal_user_check_timer(void);
extern void msproal_user_end_timer(void);
extern SINT msproal_user_get_base_adrs(ULONG *);
extern SINT msproal_user_check_stick(void);
extern void msproal_user_extract_int(MSIFHNDL *);
extern SINT msproal_user_control_power(SINT);
extern void msproal_user_change_clock(SINT);
extern void msproal_user_set_clock(UINT freq);
extern void msproal_user_uninit_system(void);
#if         !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
extern void msproal_user_icon_int(MSIFHNDL *);
#endif  /*  !((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))   */
extern void msproal_user_msif_int(MSIFHNDL *);
extern void msproal_user_check_int(MSIFHNDL *);
extern SINT msproal_user_init_system(void);
#if         ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))
#if         (1 == MSPROAL_SUPPORT_DMA)
extern SINT msproal_user_start_dma(SINT, void *, void *, SINT, SINT);
extern SINT msproal_user_check_dma_int(void);
extern void msproal_user_end_dma(void);
extern void msproal_user_dma_int(MSIFHNDL *);
#endif  /*  (1 == MSPROAL_SUPPORT_DMA)                                  */
#endif  /*  ((1 == MSPROAL_SUPPORT_IP) || (3 == MSPROAL_SUPPORT_IP))    */
#if         ((2 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))
extern SINT msproal_user_start_dma(SINT, void *, void *, SINT, SINT);
extern void msproal_user_end_dma(void);
extern void msproal_user_dma_int(MSIFHNDL *);
#endif  /*  ((2 == MSPROAL_SUPPORT_IP) || (4 == MSPROAL_SUPPORT_IP))    */
extern void msproal_user_set_flg(ULONG);
extern void msproal_user_clear_flg(ULONG);
extern SINT msproal_user_check_flg(ULONG);
extern SINT msproal_user_wait_flg(ULONG, SINT);
extern void msproal_user_wait_time(SINT);
#if         (1 == MSPROAL_DIRECT_ACCESS)
#define msproal_user_write_mem8(adrs, data)     \
            (*(volatile UBYTE *)(adrs) = (UBYTE)(data))
#define msproal_user_write_mem16(adrs, data)    \
            (*(volatile UWORD *)(adrs) = (UWORD)(data))
#define msproal_user_write_mem32(adrs, data)    \
            (*(volatile ULONG *)(adrs) = (ULONG)(data))
#define msproal_user_read_mem16(adrs, data)     \
            (*(UWORD *)(data) = *(volatile UWORD *)(adrs))
#define msproal_user_read_mem32(adrs, data)     \
            (*(ULONG *)(data) = *(volatile ULONG *)(adrs))
#else   /*  (1 == MSPROAL_DIRECT_ACCESS)    */
extern void msproal_user_write_mem32(ULONG, ULONG);
extern void msproal_user_write_mem16(ULONG, UWORD);
extern void msproal_user_read_mem32(ULONG, ULONG *);
extern void msproal_user_read_mem16(ULONG, UWORD *);
extern void msproal_user_write_mem8(ULONG, UBYTE);
#endif  /*  (1 == MSPROAL_DIRECT_ACCESS)    */
extern void msproal_user_get_error_info(ULONG **, UWORD *);
extern void msproal_user_flush_cache(void *, ULONG);
extern void msproal_user_invalidate_cache(void *, ULONG);
extern void msproal_user_virt_to_bus(void *, ULONG *);
extern void msproal_user_bus_to_virt(ULONG , void **);
extern void msproal_user_memset(UBYTE *, UBYTE, SINT);
extern void msproal_user_memcpy(UBYTE *, UBYTE *, SINT);
#if         (1 == MSPROAL_SUPPORT_V1)
extern void msproal_user_sort(UWORD *, SINT);
#endif  /*  (1 == MSPROAL_SUPPORT_V1)   */

#ifdef __cplusplus
}
#endif

#endif  /*  __MSPROAL_USER_H     */
