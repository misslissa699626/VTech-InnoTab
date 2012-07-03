/*=============================================================================
* Copyright 2003-2007 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_mc_pro.h
*
* DESCRIPTION   : I-CON layer - Declaration prototype
=============================================================================*/
#ifndef     __MSPROAL_MC_PRO_H
#define     __MSPROAL_MC_PRO_H

#include <mach/ms/msproal_config.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (2 == MSPROAL_SUPPORT_IP)
#define MSPROAL_MC_SEQ_PRO_P_READ_ATRB_LEN  49
extern const unsigned short msproal_mc_seq_pro_p_read_atrb[];
#endif

#if (4 == MSPROAL_SUPPORT_IP)
#define MSPROAL_MC_SEQ_PRO_P_READ_ATRB_LEN  49
extern const unsigned short msproal_mc_seq_pro_p_read_atrb[];
#endif

#if (5 == MSPROAL_SUPPORT_IP)
#define MSPROAL_MC_SEQ_PRO_P_READ_ATRB_LEN  59
extern const unsigned short msproal_mc_seq_pro_p_read_atrb[];
#endif

#if (2 == MSPROAL_SUPPORT_IP)
#define MSPROAL_MC_SEQ_PRO_P_READ_DATA_LEN  49
extern const unsigned short msproal_mc_seq_pro_p_read_data[];
#endif

#if (4 == MSPROAL_SUPPORT_IP)
#define MSPROAL_MC_SEQ_PRO_P_READ_DATA_LEN  49
extern const unsigned short msproal_mc_seq_pro_p_read_data[];
#endif

#if (5 == MSPROAL_SUPPORT_IP)
#define MSPROAL_MC_SEQ_PRO_P_READ_DATA_LEN  59
extern const unsigned short msproal_mc_seq_pro_p_read_data[];
#endif

#if (2 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_PROHG)
#define MSPROAL_MC_SEQ_PRO_P_READ_2K_DATA_LEN   49
extern const unsigned short msproal_mc_seq_pro_p_read_2k_data[];
#endif
#endif

#if (5 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_PROHG)
#define MSPROAL_MC_SEQ_PRO_P_READ_2K_DATA_LEN   59
extern const unsigned short msproal_mc_seq_pro_p_read_2k_data[];
#endif
#endif

#if (2 == MSPROAL_SUPPORT_IP)
#define MSPROAL_MC_SEQ_PRO_P_WRITE_DATA_LEN 42
extern const unsigned short msproal_mc_seq_pro_p_write_data[];
#endif

#if (4 == MSPROAL_SUPPORT_IP)
#define MSPROAL_MC_SEQ_PRO_P_WRITE_DATA_LEN 42
extern const unsigned short msproal_mc_seq_pro_p_write_data[];
#endif

#if (5 == MSPROAL_SUPPORT_IP)
#define MSPROAL_MC_SEQ_PRO_P_WRITE_DATA_LEN 52
extern const unsigned short msproal_mc_seq_pro_p_write_data[];
#endif

#if (2 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_PROHG)
#define MSPROAL_MC_SEQ_PRO_P_WRITE_2K_DATA_LEN  42
extern const unsigned short msproal_mc_seq_pro_p_write_2k_data[];
#endif
#endif

#if (5 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_PROHG)
#define MSPROAL_MC_SEQ_PRO_P_WRITE_2K_DATA_LEN  52
extern const unsigned short msproal_mc_seq_pro_p_write_2k_data[];
#endif
#endif

#if (( 2 == MSPROAL_SUPPORT_IP ) || (4 == MSPROAL_SUPPORT_IP))
#define MSPROAL_MC_SEQ_PRO_STARTUP_LEN  34
extern const unsigned short msproal_mc_seq_pro_startup[];
#endif

#if (5 == MSPROAL_SUPPORT_IP)
#define MSPROAL_MC_SEQ_PRO_STARTUP_LEN  36
extern const unsigned short msproal_mc_seq_pro_startup[];
#endif

#if (2 == MSPROAL_SUPPORT_IP)
#define MSPROAL_MC_SEQ_PRO_S_READ_ATRB_LEN  55
extern const unsigned short msproal_mc_seq_pro_s_read_atrb[];
#endif

#if (4 == MSPROAL_SUPPORT_IP)
#define MSPROAL_MC_SEQ_PRO_S_READ_ATRB_LEN  55
extern const unsigned short msproal_mc_seq_pro_s_read_atrb[];
#endif

#if (5 == MSPROAL_SUPPORT_IP)
#define MSPROAL_MC_SEQ_PRO_S_READ_ATRB_LEN  65
extern const unsigned short msproal_mc_seq_pro_s_read_atrb[];
#endif

#if (2 == MSPROAL_SUPPORT_IP)
#define MSPROAL_MC_SEQ_PRO_S_READ_DATA_LEN  55
extern const unsigned short msproal_mc_seq_pro_s_read_data[];
#endif

#if (4 == MSPROAL_SUPPORT_IP)
#define MSPROAL_MC_SEQ_PRO_S_READ_DATA_LEN  55
extern const unsigned short msproal_mc_seq_pro_s_read_data[];
#endif

#if (5 == MSPROAL_SUPPORT_IP)
#define MSPROAL_MC_SEQ_PRO_S_READ_DATA_LEN  65
extern const unsigned short msproal_mc_seq_pro_s_read_data[];
#endif

#if (2 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_PROHG)
#define MSPROAL_MC_SEQ_PRO_S_READ_2K_DATA_LEN   55
extern const unsigned short msproal_mc_seq_pro_s_read_2k_data[];
#endif
#endif

#if (5 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_PROHG)
#define MSPROAL_MC_SEQ_PRO_S_READ_2K_DATA_LEN   65
extern const unsigned short msproal_mc_seq_pro_s_read_2k_data[];
#endif
#endif

#if (2 == MSPROAL_SUPPORT_IP)
#define MSPROAL_MC_SEQ_PRO_S_WRITE_DATA_LEN 46
extern const unsigned short msproal_mc_seq_pro_s_write_data[];
#endif

#if (4 == MSPROAL_SUPPORT_IP)
#define MSPROAL_MC_SEQ_PRO_S_WRITE_DATA_LEN 46
extern const unsigned short msproal_mc_seq_pro_s_write_data[];
#endif

#if (5 == MSPROAL_SUPPORT_IP)
#define MSPROAL_MC_SEQ_PRO_S_WRITE_DATA_LEN 56
extern const unsigned short msproal_mc_seq_pro_s_write_data[];
#endif

#if (2 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_PROHG)
#define MSPROAL_MC_SEQ_PRO_S_WRITE_2K_DATA_LEN  46
extern const unsigned short msproal_mc_seq_pro_s_write_2k_data[];
#endif
#endif

#if (5 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_PROHG)
#define MSPROAL_MC_SEQ_PRO_S_WRITE_2K_DATA_LEN  56
extern const unsigned short msproal_mc_seq_pro_s_write_2k_data[];
#endif
#endif


#ifdef __cplusplus
}
#endif

#endif  /*  __MSPROAL_MC_PRO_H  */
