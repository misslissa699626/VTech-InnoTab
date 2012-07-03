/**************************************************************************
 *                                                                        *
 *         Copyright (c) 2006 by Sunplus Technology Co., Ltd.             *
 *                                                                        *
 *  This software is copyrighted by and is the property of Sunplus        *
 *  Technology Co., Ltd. All rights are reserved by Sunplus Technology    *
 *  Co., Ltd. This software may only be used in accordance with the       *
 *  corresponding license agreement. Any unauthorized use, duplication,   *
 *  distribution, or disclosure of this software is expressly forbidden.  *
 *                                                                        *
 *  This Copyright notice MUST not be removed or modified without prior   *
 *  written consent of Sunplus Technology Co., Ltd.                       *
 *                                                                        *
 *  Sunplus Technology Co., Ltd. reserves the right to modify this        *
 *  software without notice.                                              *
 *                                                                        *
 *  Sunplus Technology Co., Ltd.                                          *
 *  19, Innovation First Road, Science-Based Industrial Park,             *
 *  Hsin-Chu, Taiwan, R.O.C.                                              *
 *                                                                        *
 *  Author: Stephen Hung                                                  *
 *                                                                        *
 **************************************************************************/
/**
 * @file common.h
 * @brief Basic datatype definition
 */

#ifndef __COMMON_H__
#define __COMMON_H__

/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#ifndef NULL
#define NULL  ((void *)0)
#endif

#define SUCCESS         0
#define FAIL            1

#define TRUE            1
#define FALSE           0

#define BOOL 	unsigned char

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/

/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
#ifndef __SP_INT_TYPES__
#define __SP_INT_TYPES__
typedef unsigned long long UINT64;
typedef unsigned int       UINT32;
typedef unsigned short     UINT16;
typedef unsigned char      UINT8;

typedef signed long long   SINT64;
typedef signed int         SINT32;
typedef signed short       SINT16;
typedef signed char        SINT8;

typedef long long   INT64;
typedef int             INT32;
typedef short         INT16;
typedef char          INT8;

#endif

#define HAL_READ_UINT8( _register_, _value_ ) \
        ((_value_) = *((volatile UINT8 *)(_register_)))

#define HAL_WRITE_UINT8( _register_, _value_ ) \
        (*((volatile UINT8 *)(_register_)) = (_value_))

#define HAL_READ_UINT16( _register_, _value_ ) \
        ((_value_) = *((volatile UINT16 *)(_register_)))

#define HAL_WRITE_UINT16( _register_, _value_ ) \
        (*((volatile UINT16 *)(_register_)) = (_value_))

#define HAL_READ_UINT32( _register_, _value_ ) \
        ((_value_) = *((volatile UINT32 *)(_register_)))

#define HAL_WRITE_UINT32( _register_, _value_ ) \
        (*((volatile UINT32 *)(_register_)) = (_value_))
        
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/
/*workaround for <stdio.h> in which putchar is a macro*/
#ifndef WIN32
#undef putchar
int putchar(int ch);
#endif

#undef  sio_printf
#define sio_printf   printf
#undef  sio_vprintf
#define sio_vprintf  vsprintf
void usb_printf(const char *format,...);

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/

#endif  /* __COMMON_H__ */

