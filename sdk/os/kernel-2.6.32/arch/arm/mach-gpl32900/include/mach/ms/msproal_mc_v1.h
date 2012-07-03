/*=============================================================================
* Copyright 2003-2007 Sony Corporation
* Confidential Information
*
* VERSION       : Ver. 1.00
*------------------------------------------------------------------------------
* FILENAME      : msproal_mc_v1.h
*
* DESCRIPTION   : I-CON layer - Declaration prototype
=============================================================================*/
#ifndef     __MSPROAL_MC_V1_H
#define     __MSPROAL_MC_V1_H

#include <mach/ms/msproal_config.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (2 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_V1)
#define MSPROAL_MC_DRV_V1_P_READ_NEXTRADATA_LEN 47
extern const unsigned short msproal_mc_drv_v1_p_read_nextradata[];
#endif
#endif

#if (4 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_V1)
#define MSPROAL_MC_DRV_V1_P_READ_NEXTRADATA_LEN 47
extern const unsigned short msproal_mc_drv_v1_p_read_nextradata[];
#endif
#endif

#if (5 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_V1)
#define MSPROAL_MC_DRV_V1_P_READ_NEXTRADATA_LEN 55
extern const unsigned short msproal_mc_drv_v1_p_read_nextradata[];
#endif
#endif

#if (2 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_V1)
#define MSPROAL_MC_DRV_V1_S_READ_NEXTRADATA_LEN 49
extern const unsigned short msproal_mc_drv_v1_s_read_nextradata[];
#endif
#endif

#if (4 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_V1)
#define MSPROAL_MC_DRV_V1_S_READ_NEXTRADATA_LEN 49
extern const unsigned short msproal_mc_drv_v1_s_read_nextradata[];
#endif
#endif

#if (5 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_V1)
#define MSPROAL_MC_DRV_V1_S_READ_NEXTRADATA_LEN 57
extern const unsigned short msproal_mc_drv_v1_s_read_nextradata[];
#endif
#endif

#if (2 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_V1)
#define MSPROAL_MC_SEQ_V1_P_READ_BLOCK_LEN  91
extern const unsigned short msproal_mc_seq_v1_p_read_block[];
#endif
#endif

#if (4 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_V1)
#define MSPROAL_MC_SEQ_V1_P_READ_BLOCK_LEN  91
extern const unsigned short msproal_mc_seq_v1_p_read_block[];
#endif
#endif

#if (5 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_V1)
#define MSPROAL_MC_SEQ_V1_P_READ_BLOCK_LEN  103
extern const unsigned short msproal_mc_seq_v1_p_read_block[];
#endif
#endif

#if (2 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_V1)
#define MSPROAL_MC_SEQ_V1_P_WRITE_BLOCK_LEN 85
extern const unsigned short msproal_mc_seq_v1_p_write_block[];
#endif
#endif

#if (4 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_V1)
#define MSPROAL_MC_SEQ_V1_P_WRITE_BLOCK_LEN 85
extern const unsigned short msproal_mc_seq_v1_p_write_block[];
#endif
#endif

#if (5 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_V1)
#define MSPROAL_MC_SEQ_V1_P_WRITE_BLOCK_LEN 95
extern const unsigned short msproal_mc_seq_v1_p_write_block[];
#endif
#endif

#if (2 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_V1)
#define MSPROAL_MC_SEQ_V1_S_READ_BLOCK_LEN  97
extern const unsigned short msproal_mc_seq_v1_s_read_block[];
#endif
#endif

#if (4 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_V1)
#define MSPROAL_MC_SEQ_V1_S_READ_BLOCK_LEN  97
extern const unsigned short msproal_mc_seq_v1_s_read_block[];
#endif
#endif

#if (5 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_V1)
#define MSPROAL_MC_SEQ_V1_S_READ_BLOCK_LEN  109
extern const unsigned short msproal_mc_seq_v1_s_read_block[];
#endif
#endif

#if (2 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_V1)
#define MSPROAL_MC_SEQ_V1_S_WRITE_BLOCK_LEN 95
extern const unsigned short msproal_mc_seq_v1_s_write_block[];
#endif
#endif

#if (4 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_V1)
#define MSPROAL_MC_SEQ_V1_S_WRITE_BLOCK_LEN 95
extern const unsigned short msproal_mc_seq_v1_s_write_block[];
#endif
#endif

#if (5 == MSPROAL_SUPPORT_IP)
#if (1 == MSPROAL_SUPPORT_V1)
#define MSPROAL_MC_SEQ_V1_S_WRITE_BLOCK_LEN 105
extern const unsigned short msproal_mc_seq_v1_s_write_block[];
#endif
#endif


#ifdef __cplusplus
}
#endif

#endif  /*  __MSPROAL_MC_V1_H   */
