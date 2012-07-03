/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2010 by Generalplus Inc.                         *
 *                                                                        *
 *  This software is copyrighted by and is the property of Generalplus    *
 *  Inc. All rights are reserved by Generalplus Inc.                      *
 *  This software may only be used in accordance with the                 *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Generalplus Technology Co., Ltd.                   *
 *                                                                        *
 *  Generalplus Inc. reserves the right to modify this software           *
 *  without notice.                                                       *
 *                                                                        *
 *  Generalplus Inc.                                                      *
 *  3F, No.8, Dusing Rd., Hsinchu Science Park,                           *
 *  Hsinchu City 30078, Taiwan, R.O.C.                                    *
 *                                                                        *
 **************************************************************************/
/**
 * @file    ceva_nop.h
 * @brief   include a ceva nop codec
 * @author  roger hsu
 * @date    2011-04-22
 */
unsigned char cevaNopCodec[] = {
#if 1
//nop_sem0_eC_dint.bin
  0x1f, 0x00, 0xb4, 0xd7, 0xbf, 0x01, 0x00, 0x99, 
  0x00, 0x0a, 0x00, 0x00, 0x80, 0x9c, 0x01, 0x38, 
  0x01, 0xa0, 0xb9, 0x97, 0xa0, 0x3c, 0x59, 0x0a, 
  0xa0, 0x3c, 0xa0, 0x3c, 0xa0, 0x3c, 0xa0, 0x3c, 
  0x01, 0x00, 0xb4, 0x97, 0x00, 0x00, 0x04, 0xf8, 
  0x01, 0xa0, 0xb9, 0x97, 0xa0, 0x3c, 0x80, 0x0a, 
  0x94, 0x97, 0xa0, 0x3c, 0x02, 0x7d, 0xb4, 0x97, 
  0x02, 0x90, 0xb1, 0x97, 0xa0, 0x3c, 0x42, 0x04, 
  0x94, 0x0d, 0x20, 0x9b, 0x20, 0x00, 0x00, 0xf8, 
  0xa0, 0x3c, 0xa0, 0x3c, 0x20, 0xa0, 0xb8, 0x97, 
  0xa1, 0x1f, 0xd4, 0x97, 0xa0, 0x3c, 0xa0, 0x3c, 
  0x10, 0x12, 0x20, 0x9b, 0x20, 0x00, 0x00, 0xf8, 
  0xb9, 0x97, 0xa0, 0x3c, 0x00, 0x0a, 0x69, 0x0a, 
  0xa0, 0x3c, 0xa0, 0x3c, 0xa0, 0x3c, 0x01, 0xa0, 
  0xb6, 0x97, 0x85, 0x35, 0x04, 0xf8, 0xa0, 0x3c, 
  0x02, 0x38, 0x01, 0x00, 0xb6, 0xd7, 0x83, 0x48, 
  0xa0, 0x3c, 0x23, 0xe0, 0xb9, 0x97, 0xa0, 0x3c, 
  0xa0, 0x3c, 0xa0, 0x3c, 0xa0, 0x3c, 0xa0, 0x3c, 
  0x20, 0x9b, 0x20, 0x00, 0x00, 0xf8, 0xa0, 0x3c, 
  0xfa, 0xfb, 0xa0, 0x3c, 0xa0, 0x3c, 0x1f, 0x22, 
  0x00, 0x98, 0x01, 0x00, 0xb4, 0x97, 0x05, 0x00, 
  0x16, 0x04, 0x92, 0x87, 0x00, 0x16, 0x94, 0x06, 
  0x01, 0x0d, 0xd4, 0x89, 0x00, 0x50, 0xea, 0x8b, 
  0x00, 0x50, 0xea, 0x8b, 0x00, 0x50, 0xea, 0x89, 
  0x16, 0x00, 0xd8, 0x85, 0x00, 0x50, 0xea, 0x8b, 
  0x04, 0x00, 0xfa, 0xfb, 0x21, 0x10, 0x90, 0x97, 
  0x05, 0x00, 0xfa, 0xfb, 0x00, 0x00, 0xb4, 0x97, 
  0x07, 0x00, 0xfa, 0xfb, 0x01, 0x00, 0xb4, 0x97, 
  0x07, 0x00, 0xfa, 0xfb, 0x02, 0x00, 0xb4, 0x97, 
  0x00, 0x10, 0xec, 0x8d, 0x83, 0x00, 0xb4, 0x97, 
  0x00, 0xd8, 0x03, 0x48, 0x01, 0x11, 0xec, 0x8d, 
  0x80, 0x3f, 0x00, 0x50, 0xea, 0xc9, 0x13, 0x0b, 
  0x86, 0x00, 0x92, 0xa7, 0x96, 0x18, 0x92, 0xc7, 
  0x10, 0x42, 0xe6, 0x89, 0x00, 0x50, 0xea, 0x89, 
  0x01, 0x15, 0x08, 0x3d, 0x60, 0x0d, 0xef, 0x89, 
  0x6a, 0x8f, 0x01, 0x17, 0x2c, 0xcd, 0x12, 0x20, 
  0x05, 0x15, 0x74, 0xcd, 0x09, 0x3d, 0x02, 0x17, 
  0x00, 0x50, 0xea, 0x8b, 0x2f, 0x4a, 0x13, 0x48, 
  0xea, 0x8b, 0x00, 0x50, 0xea, 0x89, 0x37, 0x4a, 
  0xd4, 0x8d, 0x00, 0x50, 0xd4, 0x8f, 0x00, 0x50, 
  0x20, 0x9b, 0x20, 0x00, 0x00, 0xf8, 0x00, 0x50, 
  0xea, 0x8b, 0x00, 0x50, 0xea, 0x89, 0x9f, 0x27, 
  0xea, 0x8b, 0x00, 0x50, 0xea, 0x89, 0x00, 0x50
#else
//nop_sem0_eC_ndint.bin
  0x1f, 0x00, 0xb4, 0xd7, 0xbf, 0x01, 0x00, 0x99, 
  0xa0, 0x3c, 0x59, 0x0a, 0x00, 0x0a, 0x01, 0x38, 
  0xa0, 0x3c, 0xa0, 0x3c, 0x01, 0xa0, 0xb9, 0x97, 
  0x00, 0x00, 0x04, 0xf8, 0xa0, 0x3c, 0xa0, 0x3c, 
  0xa0, 0x3c, 0x80, 0x0a, 0x01, 0x00, 0xb4, 0x97, 
  0x02, 0x7d, 0xb4, 0x97, 0x01, 0xa0, 0xb9, 0x97, 
  0xa0, 0x3c, 0x42, 0x04, 0x94, 0x97, 0xa0, 0x3c, 
  0x20, 0x00, 0x00, 0xf8, 0x02, 0x90, 0xb1, 0x97, 
  0x20, 0xa0, 0xb8, 0x97, 0x94, 0x0c, 0x20, 0x9b, 
  0xa0, 0x3c, 0xa0, 0x3c, 0xa0, 0x3c, 0xa0, 0x3c, 
  0x20, 0x00, 0x00, 0xf8, 0xa1, 0x1f, 0xd4, 0x97, 
  0x00, 0x0a, 0x69, 0x0a, 0x10, 0x11, 0x20, 0x9b, 
  0xa0, 0x3c, 0x01, 0xa0, 0xb9, 0x97, 0xa0, 0x3c, 
  0x04, 0xf8, 0xa0, 0x3c, 0xa0, 0x3c, 0xa0, 0x3c, 
  0xb6, 0xd7, 0x83, 0x48, 0xb6, 0x97, 0x85, 0x35, 
  0xb9, 0x97, 0xa0, 0x3c, 0x02, 0x38, 0x01, 0x00, 
  0xa0, 0x3c, 0xa0, 0x3c, 0xa0, 0x3c, 0x23, 0xe0, 
  0x00, 0xf8, 0xa0, 0x3c, 0xa0, 0x3c, 0xa0, 0x3c, 
  0xa0, 0x3c, 0x1f, 0x21, 0x20, 0x9b, 0x20, 0x00, 
  0xb4, 0x97, 0x05, 0x00, 0xfa, 0xfb, 0xa0, 0x3c, 
  0x00, 0x16, 0x94, 0x06, 0x00, 0x98, 0x01, 0x00, 
  0x00, 0x50, 0xea, 0x8b, 0x16, 0x04, 0x92, 0x87, 
  0x00, 0x50, 0xea, 0x89, 0x01, 0x0d, 0xd4, 0x89, 
  0x00, 0x50, 0xea, 0x8b, 0x00, 0x50, 0xea, 0x8b, 
  0x21, 0x10, 0x90, 0x97, 0x16, 0x00, 0xd8, 0x85, 
  0x00, 0x00, 0xb4, 0x97, 0x04, 0x00, 0xfa, 0xfb, 
  0x01, 0x00, 0xb4, 0x97, 0x05, 0x00, 0xfa, 0xfb, 
  0x02, 0x00, 0xb4, 0x97, 0x07, 0x00, 0xfa, 0xfb, 
  0x83, 0x00, 0xb4, 0x97, 0x07, 0x00, 0xfa, 0xfb, 
  0x01, 0x11, 0xec, 0x8d, 0x00, 0x10, 0xec, 0x8d, 
  0x92, 0xc7, 0x93, 0x09, 0x00, 0x98, 0x03, 0x48, 
  0xea, 0x89, 0x86, 0x00, 0x92, 0xa7, 0x96, 0x18, 
  0xef, 0x89, 0x10, 0x42, 0xe6, 0x89, 0x00, 0x50, 
  0x12, 0x20, 0x01, 0x15, 0x08, 0x3d, 0x60, 0x0d, 
  0x02, 0x17, 0x6a, 0x8f, 0x01, 0x17, 0x2c, 0xcd, 
  0x13, 0x48, 0x05, 0x15, 0x74, 0xcd, 0x49, 0x3d, 
  0x37, 0x4a, 0x00, 0x50, 0xea, 0x8b, 0x2f, 0x4a, 
  0x00, 0x50, 0xea, 0x8b, 0x00, 0x50, 0xea, 0x89, 
  0x00, 0x50, 0xd4, 0x8d, 0x00, 0x50, 0xd4, 0x8f, 
  0x9f, 0x26, 0x20, 0x9b, 0x20, 0x00, 0x00, 0xf8, 
  0x00, 0x50, 0xea, 0x8b, 0x00, 0x50, 0xea, 0x89, 
  0x00, 0x50, 0xea, 0x8b, 0x00, 0x50, 0xea, 0x89
#endif
};
 