#include <stdarg.h>
#include <mach/kernel.h>
#include <mach/diag.h>


static int gLogLevel = DIAG_LVL_VERB;

int
diag_vprintf(
	const char *fmt,
	va_list ap
)
{
	int ret;

	ret = vprintk(fmt, ap);

	return ret;
}
EXPORT_SYMBOL(diag_vprintf);

int
diag_printf(
	const char *fmt,
	...
)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = diag_vprintf(fmt, ap);
	va_end(ap);

	return ret;
}
EXPORT_SYMBOL(diag_printf);

int
diagLogSetLevel(
	int logLevel
)
{
	gLogLevel = logLevel;

	return 0;
}
EXPORT_SYMBOL(diagLogSetLevel);

int
diagLog(
	int logLevel,
	const char *fmt,
	...
)
{
	va_list ap;

	if (logLevel > gLogLevel)
		return 0;
	
	va_start(ap, fmt);
	diag_vprintf(fmt, ap);
	va_end(ap);

	return 0;
}
EXPORT_SYMBOL(diagLog);

int
diagTrace(
	const char *func,
	int line,
	const char *fmt,
	...
)
{
	va_list ap;

	diag_printf("[%s:%d]", func, line);

	va_start(ap, fmt);
	diag_vprintf(fmt, ap);
	va_end(ap);

	return 0;
}
EXPORT_SYMBOL(diagTrace);

int
diagTraceFuncEnter(
	const char *func,
	int line
)
{
	diag_printf("[%s:%d] enter\n", func, line);

	return 0;
}
EXPORT_SYMBOL(diagTraceFuncEnter);

int
diagTraceFuncExit(
	const char *func,
	int line
)
{
	diag_printf("[%s:%d] exit\n", func, line);

	return 0;
}
EXPORT_SYMBOL(diagTraceFuncExit);

int
diagTraceLine(
	const char *func,
	int line
)
{
	diag_printf("[%s:%d] line\n", func, line);

	return 0;
}
EXPORT_SYMBOL(diagTraceLine);

int
diagFail(
	const char *file,
	int line,
	const char *func,
	const char *expr
)
{
	diag_printf("assertion \"%s\" failed: file \"%s\", line %d, function: %s\n",
	   expr, file, line, func);

   return 0;
}
EXPORT_SYMBOL(diagFail);

