/*
 * libmad - MPEG audio decoder library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: minimad.c,v 1.4 2004/01/23 09:41:32 rob Exp $
 */

# include <stdio.h>
#include <stdlib.h>
# include <unistd.h>
# include <sys/stat.h>
# include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <config/sysconfig.h>
#ifdef SYSCONFIG_SIMULATOR
#include <linux/soundcard.h>
#else
#include "mach/audio/soundcard.h"
#endif
# include "mad.h"











/*
 * This is perhaps the simplest example use of the MAD high-level API.
 * Standard input is mapped into memory via mmap(), then the high-level API
 * is invoked with three callbacks: input, output, and error. The output
 * callback converts MAD's high-resolution PCM samples to 16 bits, then
 * writes them to standard output in little-endian, stereo-interleaved
 * format.
 */
void evtSpeakerEn();
int enableSpeaker(int en);
int setVolume(int,int);
static int decode(unsigned char const *, unsigned long,int);

int main(int argc, char *argv[])
{
  struct stat stat;
  void *fdm;
	int vol;
  if (argc != 2)
    return 1;

	vol = atoi(argv[1]);

  if (fstat(STDIN_FILENO, &stat) == -1 ||
      stat.st_size == 0)
    return 2;

  fdm = mmap(0, stat.st_size, PROT_READ, MAP_SHARED, STDIN_FILENO, 0);
  if (fdm == MAP_FAILED)
    return 3;

  decode(fdm, stat.st_size,vol);

  if (munmap(fdm, stat.st_size) == -1)
    return 4;

  return 0;
}

/*
 * This is a private message structure. A generic pointer to this structure
 * is passed to each of the callback functions. Put here any data you need
 * to access from within the callbacks.
 */

struct buffer {
  unsigned char const *start;
  unsigned long length;
};

/*
 * This is the input callback. The purpose of this callback is to (re)fill
 * the stream buffer which is to be decoded. In this example, an entire file
 * has been mapped into memory, so we just call mad_stream_buffer() with the
 * address and length of the mapping. When this callback is called a second
 * time, we are finished decoding.
 */

static
enum mad_flow input(void *data,
		    struct mad_stream *stream)
{
  struct buffer *buffer = data;

  if (!buffer->length)
    return MAD_FLOW_STOP;

  mad_stream_buffer(stream, buffer->start, buffer->length);

  buffer->length = 0;

  return MAD_FLOW_CONTINUE;
}

/*
 * The following utility routine performs simple rounding, clipping, and
 * scaling of MAD's high-resolution samples down to 16 bits. It does not
 * perform any dithering or noise shaping, which would be recommended to
 * obtain any exceptional audio quality. It is therefore not recommended to
 * use this routine if high-quality output is desired.
 */

static inline
signed int scale(mad_fixed_t sample)
{
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}

/*
 * This is the output callback function. It is called after each frame of
 * MPEG audio data has been completely decoded. The purpose of this callback
 * is to output (or play) the decoded PCM audio.
 */

static signed short DO_C( signed short sample )
{
	signed short value = sample;
	signed short b = value >> 8;
	value = (( value & 0xff ) << 8) + b;

	return value;
}

static int fd;
static int fd_mix;
static unsigned long rSampleCount =0;
static unsigned long sSampleCount =0;

static
enum mad_flow output(void *data,
		     struct mad_header const *header,
		     struct mad_pcm *pcm)
{
  unsigned int nchannels, nsamples;
  mad_fixed_t const *left_ch, *right_ch;
	signed short mybuf[16384];
	int count = 0;
	unsigned temp = pcm->samplerate /100;
  /* pcm->samplerate contains the sampling frequency */

  nchannels = pcm->channels;
  nsamples  = pcm->length;
  left_ch   = pcm->samples[0];
  right_ch  = pcm->samples[1];
	

  while (nsamples--) {
    signed int sample;

    /* output sample(s) in 16-bit signed little-endian PCM */

    sample = scale(*left_ch++);
		mybuf[ count++ ] = (sample);

    if (nchannels == 2) {
      sample = scale(*right_ch++);
			mybuf[ count++ ] = (sample);
    }else if (nchannels == 1) {/*duplicate*/
      sample = scale(*left_ch);
			mybuf[ count++ ] = (sample);
    }
    
    sSampleCount++;
 		rSampleCount++;
 		while( sSampleCount  <   (rSampleCount*480)/(temp)  ){
 			
 			sSampleCount++;
	    sample = scale(*left_ch);
			mybuf[ count++ ] = (sample);
	    if (nchannels == 2) {
	      sample = scale(*right_ch);
				mybuf[ count++ ] = (sample);      
	    }else if (nchannels == 1) {/*duplicate*/
      sample = scale(*left_ch);
			mybuf[ count++ ] = (sample);
     }
	    
	    
	    
 		}
 		
    
  }
	/*printf("nchannels=%d, sample count=%d\n", nchannels, count);*/
	 
	write(fd, mybuf, count*2);

  return MAD_FLOW_CONTINUE;
}

/*
 * This is the error callback function. It is called whenever a decoding
 * error occurs. The error is indicated by stream->error; the list of
 * possible MAD_ERROR_* errors can be found in the mad.h (or stream.h)
 * header file.
 */

static
enum mad_flow error(void *data,
		    struct mad_stream *stream,
		    struct mad_frame *frame)
{
  struct buffer *buffer = data;

  fprintf(stderr, "decoding error 0x%04x (%s) at byte offset %u\n",
	  stream->error, mad_stream_errorstr(stream),
	  stream->this_frame - buffer->start);

  /* return MAD_FLOW_BREAK here to stop decoding (and propagate an error) */

  return MAD_FLOW_CONTINUE;
}

/*
 * This is the function called by main() above to perform all the decoding.
 * It instantiates a decoder object and configures it with the input,
 * output, and error callback functions above. A single call to
 * mad_decoder_run() continues until a callback function returns
 * MAD_FLOW_STOP (to stop decoding) or MAD_FLOW_BREAK (to stop decoding and
 * signal an error).
 */

static
int decode(unsigned char const *start, unsigned long length,int vol)
{
  struct buffer buffer;
  struct mad_decoder decoder;
  int result;
	/* initialize the device */
	int chn, srate;
	fd = open( "/dev/dsp", O_WRONLY );
	chn = 2; /* stereo */
	ioctl( fd, SNDCTL_DSP_CHANNELS, &chn );
	srate = 48000;
	ioctl( fd, SNDCTL_DSP_SPEED, &srate );
	
	
	enableSpeaker(1);
	evtSpeakerEn();	
	setVolume(0,vol);
	setVolume(1,vol);	
	
  /* initialize our private message structure */
  buffer.start  = start;
  buffer.length = length;
  /* configure input, output, and error functions */
  mad_decoder_init(&decoder, &buffer,
		   input, 0 /* header */, 0 /* filter */, output,
		   error, 0 /* message */);
  /* start decoding */
  result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
  /* release the decoder */
  mad_decoder_finish(&decoder);
  return result;
}


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

/* Ioctl for device node definition */
#define GPIO_IOCTL_ID           'G'
#define GPIO_IOCTL_SET_VALUE    _IOW(GPIO_IOCTL_ID, 0, int)
#define GPIO_IOCTL_GET_VALUE    _IOR(GPIO_IOCTL_ID, 1, int)


typedef struct gpio_content_s {
	unsigned int pin_index;     /*!< @brief gpio pin index */
	unsigned int value;         /*!< @brief gpio value */
} gpio_content_t;


#define MK_GPIO_INDEX(channel,func,gid,pin) (((channel)<<24)|((func)<<16)|((gid)<<8)|(pin))
static unsigned char MapTbl_8050[4][32][2] =
{	
	[0] = {
		{0,18},{0,19},{0,20},{0,21},{0,22},{0,22},{0,23},{0,24},
		{0,12},{0,11},{2, 0},{2, 0},{2, 1},{2, 2},{2, 3},{0, 7},
		{0, 0},{0, 0},{0, 0},{0, 0},{2,26},{2,27},{2,28},{2,29},
		{2,30},{2,30},{2,34},{2,34},{2,34},{2,31},{2,32},{0,0}
	},
	[1] = {
		{0, 8},{0, 8},{0, 8},{0, 8},{0, 8},{0, 8},{0, 8},{0, 8},
		{2, 5},{2, 5},{2, 5},{2, 5},{2, 5},{2, 5},{2, 5},{2, 5},
		{3,13},{3,13},{2,33},{2,33},{2,14},{2,14},{2,14},{2,15},
		{2,15},{2,15},{2,16},{2,16},{2,16},{2,17},{2,17},{0,0}
	},
	[2] = {
		{2, 4},{2, 4},{2, 4},{2, 4},{2, 4},{2, 4},{2, 4},{2, 4},
		{0, 9},{0, 9},{0, 9},{0, 9},{0, 9},{0, 9},{0, 9},{0, 9},
		{2, 5},{2, 5},{2, 5},{2, 5},{2, 5},{2, 5},{0, 0},{0, 0},
		{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0}
	},
	[3] = {
		{0, 6},{0, 6},{0, 6},{0, 6},{0, 6},{0, 6},{0, 6},{0, 6},
		{0, 6},{0, 6},{0, 6},{2,25},{2,25},{2,25},{2,25},{2,25},
		{2,25},{2,25},{2,25},{2,25},{2,25},{2,25},{2,25},{2,25},
		{2,25},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0},{0, 0}
	}
};



int isHeadphone()
{
	gpio_content_t cmd = {0};
	unsigned int channel = 0;
	unsigned int func = 0;
	unsigned int gid = 0;
	unsigned int pin = 0;

	int fp =0;
	int ret = 0;
	
	/*Get channel*/
	channel = 0;
	pin = 13;

		
	func = MapTbl_8050[channel][pin][0];
	gid = MapTbl_8050[channel][pin][1];
	

	cmd.pin_index = MK_GPIO_INDEX(channel, func, gid, pin);
	
	fp = open("/dev/gpio", O_RDWR);
	if(fp < 0){
		printf("Can't open node /dev/gpio !\n");
		return 0;
	}
	
	ret = ioctl(fp, GPIO_IOCTL_GET_VALUE, &cmd);
	close(fp);
	
	if(cmd.value)
		return 0;
	else
		return 1;
}

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>

/* Dump / Fill format */
#define MEM_FORMAT_BYTE		(sizeof(unsigned char))
#define MEM_FORMAT_WORD		(sizeof(unsigned short))
#define MEM_FORMAT_DWORD		(sizeof(unsigned int))



static unsigned int
safe8bitWrite(
	unsigned int addr,
	unsigned char data
)
{
	*((volatile unsigned char *)addr) = data;
	return 1;
}

static unsigned int
safe16bitWrite(
	unsigned int addr,
	unsigned short data
)
{
	if ( addr & 0x1 ) {
		return 0;
	}

	*((volatile unsigned short *)addr) = data;
	return 1;
}

static unsigned int
safe32bitWrite(
	unsigned int addr,
	unsigned int data
)
{
	if ( addr & 0x3 ) {
		return 0;
	}
	
	*((volatile unsigned int *)addr) = data;
	return 1;
}


static int 
gp_mem_fill(
	unsigned int addr, 
	unsigned int len, 
	unsigned int format,
	unsigned int value
)
{
	unsigned int fillAddr = addr;
	unsigned int i = 0;
	unsigned int bus_error = 0;
	int ret = 0;

	if(len == 0)
		len = 1;

	while( i < len){
		switch ( format ) {
		case MEM_FORMAT_BYTE:
			bus_error = !safe8bitWrite(fillAddr, value);
			break;
		case MEM_FORMAT_WORD:
			bus_error = !safe16bitWrite(fillAddr, value);
			break;
		case MEM_FORMAT_DWORD:
			bus_error = !safe32bitWrite(fillAddr, value);
			break;
		}
		if ( bus_error ) {
			//MEM_PRINT("bus error (or alignment) error at 0x%08x, operation terminated\n", fillAddr);
			return 1;
		}
		//DEBUG0("fillAddr = 0x%08x\n", fillAddr);
		fillAddr += format;
		i++;
	}

	return 0;
}


int enableSpeaker(int en)
{
	unsigned int format = MEM_FORMAT_DWORD;
	unsigned int inputAddr = 0, address = 0;
	unsigned int inputLen = 0, length = 0;
	unsigned int data = 0;
	unsigned int i = 0;
	unsigned int endAddr = 0;
	int ret = 0;

	int fd = 0;
	void *map_base, *virt_addr;
	off_t target;
	unsigned int page_size, mapped_size, offset_in_page, len_byte;
	format = MEM_FORMAT_BYTE;

	

	inputAddr = 0x9301f020;


	/*End address or Length*/
	inputLen = 1;

	/*Fill value*/
	if(en)
		data = 0x9B;/*enable*/
	else
		data = 0x83;
	
	address = inputAddr;
	length = inputLen;
	
	/*Calculate maping base & size*/
	len_byte = length * format;
	mapped_size = page_size = getpagesize();	
	offset_in_page = (unsigned int)address & (page_size - 1);
	i = (offset_in_page + len_byte)/page_size;
	mapped_size = mapped_size*(i+1);
	
	fd = open("/dev/mem", (O_RDWR | O_SYNC));
	if(fd < 0){
		printf("Can not open /dev/mem !\n");
		return 1;
	}
		
	map_base = mmap(NULL,
			mapped_size,
			(PROT_READ | PROT_WRITE),
			MAP_SHARED,
			fd,
			address & ~(off_t)(page_size - 1));
	
	if (map_base == MAP_FAILED){
		printf("Mapping fail!\n");
		ret = 1;
		goto FAIL_MAP;
	}

	/*printf("Memory mapped at address %p, mapped size = %d .\n", map_base, mapped_size);*/
	
	virt_addr = (char*)map_base + offset_in_page;
	
	ret = gp_mem_fill((unsigned int)virt_addr, length, format, data);
	if(ret != 0){
		printf("Fill: bus error (or alignment) error at 0x%08x, operation terminated\n", virt_addr);
	}
	
	munmap(map_base, mapped_size);
	
FAIL_MAP:
	close(fd);
	return ret;
	
}



int setVolume(int output,int vol)
{
	unsigned int format = MEM_FORMAT_DWORD;
	unsigned int inputAddr = 0, address = 0;
	unsigned int inputLen = 0, length = 0;
	unsigned int data = 0;
	unsigned int i = 0;
	unsigned int endAddr = 0;
	int ret = 0;

	int fd = 0;
	void *map_base, *virt_addr;
	off_t target;
	unsigned int page_size, mapped_size, offset_in_page, len_byte;
	format = MEM_FORMAT_WORD;

	
	if(output == 0)/*headphone*/
		inputAddr = 0x9301f038;
	else /*speaker*/
		inputAddr = 0x9301f034;

	/*End address or Length*/
	inputLen = 1;

	if(vol > 63)
			vol = 63;
	if(vol < 0)
			vol = 0;
	
	vol = 63 - vol ;
	data = (vol << 7) | (vol << 2) | 0x03;
	printf("target =[%d] data=[%x]\n",output,data);
	
	address = inputAddr;
	length = inputLen;
	
	/*Calculate maping base & size*/
	len_byte = length * format;
	mapped_size = page_size = getpagesize();	
	offset_in_page = (unsigned int)address & (page_size - 1);
	i = (offset_in_page + len_byte)/page_size;
	mapped_size = mapped_size*(i+1);
	
	fd = open("/dev/mem", (O_RDWR | O_SYNC));
	if(fd < 0){
		printf("Can not open /dev/mem !\n");
		return 1;
	}
		
	map_base = mmap(NULL,
			mapped_size,
			(PROT_READ | PROT_WRITE),
			MAP_SHARED,
			fd,
			address & ~(off_t)(page_size - 1));
	
	if (map_base == MAP_FAILED){
		printf("Mapping fail!\n");
		ret = 1;
		goto FAIL_MAP;
	}

	/*printf("Memory mapped at address %p, mapped size = %d .\n", map_base, mapped_size);*/
	
	virt_addr = (char*)map_base + offset_in_page;
	
	ret = gp_mem_fill((unsigned int)virt_addr, length, format, data);
	if(ret != 0){
		printf("Fill: bus error (or alignment) error at 0x%08x, operation terminated\n", virt_addr);
	}
	
	munmap(map_base, mapped_size);
	
FAIL_MAP:
	close(fd);
	return ret;
	
}



void evtSpeakerEn()
{
	gpio_content_t cmd = {0};
	unsigned int channel = 0;
	unsigned int func = 0;
	unsigned int gid = 0;
	unsigned int pin = 0;

	int fp =0;
	int ret = 0;
	
	/*Get channel*/
	channel = 0;
	pin = 14;

		
	func = MapTbl_8050[channel][pin][0];
	gid = MapTbl_8050[channel][pin][1];
	

	cmd.pin_index = MK_GPIO_INDEX(channel, func, gid, pin);
	cmd.value = 1;
	fp = open("/dev/gpio", O_RDWR);
	if(fp < 0){
		printf("Can't open node /dev/gpio !\n");
		return ;
	}
	
	ret = ioctl(fp, GPIO_IOCTL_SET_VALUE, &cmd);
	close(fp);
	
	return;
}