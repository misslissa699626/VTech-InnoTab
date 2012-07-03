#ifndef _MOD_INIT_H_
#define _MOD_INIT_H_

#include <typedef.h>

enum {
	MOD_TYPE_CORE,
	MOD_TYPE_DRIVER,
	MOD_TYPE_SERVICE,
	MOD_TYPE_APP,
	MOD_TYPE_MAX
};

typedef UINT32 (*spInitCall_t)(void);
typedef UINT32 (*spExitCall_t)(void);

UINT32 modInitAll(SINT32 type);
UINT32 modExitAll(SINT32 type);


#define coreModInitAll() modInitAll(MOD_TYPE_CORE)
#define coreModExitAll() modExitAll(MOD_TYPE_CORE)
#define driverModInitAll() modInitAll(MOD_TYPE_DRIVER)
#define driverModExitAll() modExitAll(MOD_TYPE_DRIVER)
#define appModInitAll() modInitAll(MOD_TYPE_APP)
#define appModExitAll() modExitAll(MOD_TYPE_APP)

#if 0
#ifndef WIN32 /* MIDE gcc */

typedef struct __spModCall_s__ {
	SINT32 type;
	SP_CHAR *name;
	spInitCall_t init;
	spExitCall_t exit;
} __spModCall_t__;

#define _MODULE_DEF(type, name, init, exit) \
	static const __spModCall_t__ __mod__##init##__ __attribute__((__used__)) \
	__attribute__((__section__(".modinit"))) = { type, (SP_CHAR*)name, init, exit}

#else /* win32 vc2008 */

#define __MOD_SIG_START__ 0x12345678
#define __MOD_SIG_END__   0x87654321

typedef struct __spModCall_s__ {
	UINT32 sigStart;
	SINT32 type;
	SP_CHAR *name;
	spInitCall_t init;
	spExitCall_t exit;
	UINT32 sigEnd;
} __spModCall_t__;

#pragma const_seg(".modinit")
#pragma const_seg()

#define _MODULE_DEF(type, name, init, exit) \
	static const __declspec(allocate(".modinit")) \
	__spModCall_t__ __mod__##init##__ = { __MOD_SIG_START__, type, (SP_CHAR*)name, init, exit, __MOD_SIG_END__}

#endif

#define CORE_MODULE_DEF(name, init, exit) _MODULE_DEF(MOD_TYPE_CORE, name, init, exit)
#define DRIVER_MODULE_DEF(name, init, exit) _MODULE_DEF(MOD_TYPE_DRIVER, name, init, exit)
#define APP_MODULE_DEF(name, init, exit) _MODULE_DEF(MOD_TYPE_APP, name, init, exit)

#define MODULE_DEF(name, init, exit) DRIVER_MODULE_DEF(name, init, exit)


#else
#define CORE_MODULE_DEF(...)
#define DRIVER_MODULE_DEF(...)
#define APP_MODULE_DEF(...)
#define MODULE_DEF(...)
#endif


typedef UINT32 (*spExecCall_t)(__spModCall_t__ *pMod, UINT32 iParam);
UINT32 modExec(SINT32 type, spExecCall_t pFunc, UINT32 iParam);

#endif
