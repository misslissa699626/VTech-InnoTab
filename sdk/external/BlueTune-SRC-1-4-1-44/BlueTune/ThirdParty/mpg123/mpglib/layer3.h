#ifndef _LAYER3_H_
#define _LAYER3_H_

void init_layer3(int);
int do_layer3_sideinfo(PMPSTR mp);
int do_layer3(PMPSTR mp,unsigned char *pcm_sample,int *pcm_point,
              int (*synth_1to1_mono_ptr)(PMPSTR,real *,unsigned char *,int *),
              int (*synth_1to1_ptr)(PMPSTR,real *,int,unsigned char *, int *));

#endif /* _LAYER3_H_ */


