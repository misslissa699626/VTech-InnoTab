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
 * @file diag.h
 * @brief Diagnostic interface
 * @date 2010/06/08
 */

#ifndef _DIAG_H_
#define _DIAG_H_

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Log */

#define DIAG_LVL_EMERG    1    /* system is unusable */
#define DIAG_LVL_ALERT    2    /* action must be taken immediately */
#define DIAG_LVL_ERROR    3    /* error conditions */
#define DIAG_LVL_WARN     4    /* warning conditions */
#define DIAG_LVL_INFO     5    /* informational */
#define DIAG_LVL_DEBUG    6    /* debug-level messages */
#define DIAG_LVL_VERB     7    /* verbose-level messages */

#ifndef DIAG_LEVEL
#define DIAG_LEVEL DIAG_LVL_DEBUG
#endif

#if 0
	#define DIAG_EMERG(...)		diagLog(DIAG_LVL_EMERG, __VA_ARGS__)
	#define DIAG_ALERT(...)		diagLog(DIAG_LVL_ALERT, __VA_ARGS__)
	#define DIAG_ERROR(...)		diagLog(DIAG_LVL_ERROR, __VA_ARGS__)
	#define DIAG_WARN(...)		diagLog(DIAG_LVL_WARN, __VA_ARGS__)
	#define DIAG_INFO(...)		diagLog(DIAG_LVL_INFO, __VA_ARGS__)
	#define DIAG_DEBUG(...)		diagLog(DIAG_LVL_DEBUG, __VA_ARGS__)
	#define DIAG_VERB(...)		diagLog(DIAG_LVL_VERB, __VA_ARGS__)
#else
	#if (DIAG_LEVEL < DIAG_LVL_EMERG) || defined(DIAG_EMERG_OFF)
	#define DIAG_EMERG(...)
	#else
	#define DIAG_EMERG(...)		diagLog(DIAG_LVL_EMERG, __VA_ARGS__)
	#endif

	#if (DIAG_LEVEL < DIAG_LVL_ALERT) || defined(DIAG_ALERT_OFF)
	#define DIAG_ALERT(...)
	#else
	#define DIAG_ALERT(...)		diagLog(DIAG_LVL_ALERT, __VA_ARGS__)
	#endif

	#if (DIAG_LEVEL < DIAG_LVL_ERROR) || defined(DIAG_ERROR_OFF)
	#define DIAG_ERROR(...)
	#else
	#define DIAG_ERROR(...)		diagLog(DIAG_LVL_ERROR, __VA_ARGS__)
	#endif

	#if (DIAG_LEVEL < DIAG_LVL_WARN) || defined(DIAG_WARN_OFF)
	#define DIAG_WARN(...)
	#else
	#define DIAG_WARN(...)		diagLog(DIAG_LVL_WARN, __VA_ARGS__)
	#endif

	#if (DIAG_LEVEL < DIAG_LVL_INFO) || defined(DIAG_INFO_OFF)
	#define DIAG_INFO(...)
	#else
	#define DIAG_INFO(...)		diagLog(DIAG_LVL_INFO, __VA_ARGS__)
	#endif

	#if (DIAG_LEVEL < DIAG_LVL_DEBUG) || defined(DIAG_DEBUG_OFF)
	#define DIAG_DEBUG(...)
	#else
	#define DIAG_DEBUG(...)		diagLog(DIAG_LVL_DEBUG, __VA_ARGS__)
	#endif

	#if (DIAG_LEVEL < DIAG_LVL_VERB) || defined(DIAG_VERB_OFF)
	#define DIAG_VERB(...)
	#else
	#define DIAG_VERB(...)		diagLog(DIAG_LVL_VERB, __VA_ARGS__)
	#endif
#endif

extern int diagLog(int logLevel, const char *fmt, ...);
extern int diagLogSetLevel(int logLevel);


/* Trace */

#if defined(DIAG_TRACE_OFF)
	#define DIAG_TRACE(...)
	#define DIAG_FUNC_ENTER()
	#define DIAG_FUNC_EXIT()
	#define DIAG_TRACE_LINE()
#else
	#define DIAG_TRACE(...)		diagTrace(__FUNCTION__, __LINE__, __VA_ARGS__)
	#define DIAG_FUNC_ENTER()	diagTraceFuncEnter(__FUNCTION__, __LINE__)
	#define DIAG_FUNC_EXIT()	diagTraceFuncExit(__FUNCTION__, __LINE__)
	#define DIAG_TRACE_LINE()	diagTraceLine(__FUNCTION__, __LINE__)
#endif

extern int diagTrace(const char *func, int line, const char *fmt, ...);
extern int diagTraceFuncEnter(const char *func, int line);
extern int diagTraceFuncExit(const char *func, int line);
extern int diagTraceLine(const char *func, int line);

/* Assertion */

#if defined(DIAG_ASSERT_OFF)
	#define DIAG_ASSERT(expr)
	#define DIAG_FAIL(expr)
#else
	#define DIAG_ASSERT(expr)		((expr) ? (void)0 : \
				diagFail(__FILE__, __LINE__, __FUNCTION__, #expr))

	#define DIAG_FAIL(expr)			diagFail(__FILE__, __LINE__, \
				__FUNCTION__, #expr)
#endif

extern int diagFail(const char *file, int line, const char *func, const char *expr);


/* Printf */

#define DIAG_PRINTF(...)		diag_printf(__VA_ARGS__)
#define DIAG_VPRINTF(fmt, ap)	diag_vprintf((fmt), (ap))
#define DIAG_PUTS(x)			diag_puts(x)
#define DIAG_PUTCHAR(x)			diag_putchar(x)

extern int diag_printf(const char *fmt, ...);
extern int diag_vprintf(const char *fmt, va_list ap);
extern int diag_puts(const char *str);
extern int diag_putchar(int ch);



#ifdef __cplusplus
};
#endif

#endif /* _DIAG_H_ */
