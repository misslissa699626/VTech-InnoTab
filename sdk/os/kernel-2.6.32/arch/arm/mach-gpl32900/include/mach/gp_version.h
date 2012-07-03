
#ifndef __ARCH_GP_VERSION_H
#define __ARCH_GP_VERSION_H

#ifndef _GPL329XXA_VER
#define _GPL329XXA_VER
typedef enum {
    GPL329XXA_VER_A=1,
    GPL329XXA_VER_B=2,
    GPL329XXA_VER_C=3
} GPL329XXA_VER;
#endif

GPL329XXA_VER gp_version_get(void);


#endif  //__ARCH_GP_VERSION_H

