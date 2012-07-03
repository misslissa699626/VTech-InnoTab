#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <linux/errno.h> /* error codes */
#include "mach/typedef.h"
#include "diag.h"

#define MODULE_PATH "/lib/modules"
#define MODULE_MAX_PATH 256
static char module_path[MODULE_MAX_PATH - 32];

static char*
module_get_path(
	char *filename
)
{
	FILE *fd;
	char module_order[48];
	struct utsname uts;
	char *p;

	uname(&uts);
	sprintf(module_order, "%s/%s/modules.order", MODULE_PATH, uts.release);
	fd = fopen(module_order, "r");
	if (!fd) {
		return NULL;
	}

	while (1) {
		if (fgets(module_path, sizeof(module_path), fd) == NULL) {
			fclose(fd);
			return NULL;
		}

		p = strchr(&module_path[0], '.');
		if (p == NULL) {
			fclose(fd);
			return NULL;
		}

		*(p + 3) = 0;
		if (strncmp(p - strlen(filename), filename, strlen(filename)) == 0) {
			fclose(fd);
			return (void*) &module_path;
		}
	}

	fclose(fd);
}

int
module_install(
	char *filename
)
{
	int fd;
	struct stat st;
	unsigned long len;
	char *map;
	int ret;
	char *options;
	char *subpath;
	char module_path[MODULE_MAX_PATH];
	struct utsname uts;

	/* Get options */
	options = strchr(filename, ' ');
	if (options) {
		options = options + 1;
		filename[options - filename - 1] = 0;
		printf("filename : %d,%s\n", strlen(filename), filename);
		printf("options  : %d,%s\n", strlen(options), options);
	}

	printf("Install Module:%s\n", filename);

	if (filename[0] != '/') {
		subpath = module_get_path(filename);
		if (!subpath) {
			DIAG_ERROR("Can not get module subpath\n");
			return -EIO;
		}

		uname(&uts);
		sprintf(module_path, "%s/%s/%s", MODULE_PATH, uts.release, subpath);
		fd = open(module_path, O_RDONLY, 0);
	}
	else {
		fd = open(filename, O_RDONLY, 0);
	}

	if (fd < 0) {
		DIAG_ERROR("cannot open module\n");
		return -EIO;
	}

	fstat(fd, &st);
	len = st.st_size;
	map = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
	if (map == MAP_FAILED) {
		DIAG_ERROR("cannot mmap\n");
		close(fd);
		return -ENOMEM;
	}

	if (options)
		ret = syscall(__NR_init_module, map, len, options);
	else
	ret = syscall(__NR_init_module, map, len, "");
	if (ret != 0) {
		DIAG_ERROR("cannot insert `%s': (%li)\n",filename, ret);
	}

	munmap(map, len);
	close(fd);
	return ret;
}


UINT32
insmod(
	const char *path_fmt,
	...
)
{
	va_list ap;
	char path[128];

	va_start(ap, path_fmt);
	vsnprintf(path, sizeof(path), path_fmt, ap);
	va_end(ap);

	if (module_install(path) != 0)
		return SP_FAIL;
	else
		return SP_OK;
}
