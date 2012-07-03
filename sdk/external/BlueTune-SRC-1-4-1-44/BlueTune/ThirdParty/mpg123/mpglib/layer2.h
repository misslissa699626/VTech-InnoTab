#ifndef _LAYER2_H_
#define _LAYER2_H_

struct al_table2 
{
  short bits;
  short d;
};

void init_layer2(void);
int  do_layer2( PMPSTR mp,unsigned char *pcm_sample,int *pcm_point);

#endif /* _LAYER2_H_ */


