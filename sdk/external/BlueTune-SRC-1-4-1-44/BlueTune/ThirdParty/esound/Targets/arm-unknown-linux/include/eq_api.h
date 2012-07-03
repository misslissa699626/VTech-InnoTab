
#ifndef __EQ_API_H__
#define __EQ_API_H__


#define EQ_BAND_NUM 5
#define EQ_BLOCK_SIZE 32


int eq_init(void);

// band_gain_level: integer array with 5 elements, legal value range [-12 .. 12]
void set_eq_band(int band_gain_level[]);

// return: processed length
int eq_run(short pcm_out[], short pcm_in[], int length);


#endif // !__EQ_API_H__
