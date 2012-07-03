/*****************************************************************
|
|      File: DecoderTest.c
|
|      Melo - Simple Decoder Test
|
|      (c) 2004 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "Melo.h"
#include "MloUtils.h"

/*----------------------------------------------------------------------
|       Macros
+---------------------------------------------------------------------*/

/* Keeps all the frames from the stream */
#define  MLO_DECODER_TEST_CONTINUOUS_STREAM

/* Do not display or save anything to get a rough idea of the speed */
#undef   MLO_DECODER_TEST_FAST

/*----------------------------------------------------------------------
|       ShowFrame
+---------------------------------------------------------------------*/
static void
ShowFrame(MLO_FrameData* frame)
{
#if ! defined (MLO_DECODER_TEST_FAST)
   printf("Frame: sf=%lu, ch=%u, len=%u, s=%s, p=%s\n", 
        frame->info.sampling_frequency,
        frame->info.channel_configuration,
        frame->info.frame_length,
        frame->info.standard == MLO_AAC_STANDARD_MPEG2 ? "MPEG2" : "MPEG4",
        frame->info.profile == MLO_AAC_PROFILE_MAIN ? "MAIN" :
        frame->info.profile == MLO_AAC_PROFILE_LC ? "LC" :
        frame->info.profile == MLO_AAC_PROFILE_SSR ? "SSR" : "?");
#endif   /* MLO_DECODER_TEST_FAST */
}

/*----------------------------------------------------------------------
|       main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    FILE*           input;
#if ! defined (MLO_DECODER_TEST_FAST)
    FILE*           outfile_ptr;
#endif   /* MLO_DECODER_TEST_FAST */
    MLO_Decoder*    decoder;
    MLO_AdtsParser* parser;
    MLO_Result      result;

    /* check command line args */
    if (argc != 3) {
        fprintf(stderr, "DecderTest <input_filename> <output_filename>\n");
        return 1;
    }

    /* open the file */
    input = fopen(argv[1], "rb");
    if (input == NULL) {
        fprintf(stderr, "ERROR: cannot open input '%s' (%s)\n", argv[1], strerror(errno));
        return 1;
    }

#if ! defined (MLO_DECODER_TEST_FAST)
    outfile_ptr = fopen (argv [2], "wb");
    if (outfile_ptr == 0)
    {
       fprintf(stderr, "ERROR: cannot open output '%s' (%s)\n", argv[2], strerror(errno));
       return (1);
    }
#endif   /* MLO_DECODER_TEST_FAST */

    /* create a decoder */
    result = MLO_Decoder_Create(&decoder);
   if (MLO_FAILED(result))
   {
      fprintf (stderr, "Cannot create decoder.\n");
      return (1);
   }

    /* create a frame parser */
    result = MLO_AdtsParser_Create(&parser);
   if (MLO_FAILED(result))
   {
      fprintf (stderr, "Cannot create the ADTS parser.\n");
      return (1);
   }

    for (;;) {
        /* read some data */
        unsigned char input_buffer[2048];
        unsigned int offset = 0;
        size_t read = fread(input_buffer, 1, sizeof(input_buffer), input);
        if (read <= 0) break;

        while (offset < read) {
            /* feed as much as we can */
            MLO_Size size = (MLO_Size)(read - offset);
            MLO_Size free_bytes = MLO_AdtsParser_GetBytesFree (parser);
            if (size > free_bytes)
            {
               size = free_bytes;
            }
            result = MLO_AdtsParser_Feed(parser, &input_buffer[offset], &size, 0);
            if (MLO_FAILED(result)) {
                fprintf(stderr, "MLO_AdtsParser_Feed failed (%d)\n", result);
                return 1;
            }
            /* adjust size to account for what was consumed */
            offset += size;

#if ! defined (MLO_DECODER_TEST_CONTINUOUS_STREAM)
            /* find all the frames in what was fed so far */
            do
#endif
            {
               MLO_FrameData frame;
               MLO_SampleBuffer output_buffer;
               output_buffer.samples = NULL;
               output_buffer.size = 0;
               result = MLO_AdtsParser_FindFrame(parser, &frame);
               if (MLO_SUCCEEDED(result))
               {
                  size_t         buf_size;
                  ShowFrame (&frame);
                  result = MLO_Decoder_DecodeFrame(decoder, &frame, &output_buffer);
                  if (MLO_FAILED(result)) {
                      fprintf(stderr, "DecodeFrame failed (%d)\n", result);
                      break;
                  }
                  buf_size = output_buffer.sample_count * output_buffer.format.channel_count * output_buffer.format.bits_per_sample / CHAR_BIT;
#if ! defined (MLO_DECODER_TEST_FAST)
                  {
                     const size_t   written = fwrite (output_buffer.samples, 1, buf_size, outfile_ptr);
                     if (written != buf_size)
                     {
                        fprintf(stderr, "ERROR: cannot open write output file.\n");
                        return (1);
                     }
                  }
#endif   /* MLO_DECODER_TEST_FAST */
               }
            }
#if ! defined (MLO_DECODER_TEST_CONTINUOUS_STREAM)
            while (MLO_SUCCEEDED(result) || result == MLO_ERROR_CORRUPTED_BITSTREAM);
            if (MLO_FAILED (result) && result != MLO_ERROR_NOT_ENOUGH_DATA) {
                fprintf(stderr, "MLO_AdtsParser_FindFrame failed (%d)\n", result);
                return 0;
            }
#endif
        }
    }
    
    /* destroy the decoder */
    MLO_Decoder_Destroy(decoder);

    /* close the files */
    fclose(input);

#if ! defined (MLO_DECODER_TEST_FAST)
    fclose (outfile_ptr);
#endif   /* MLO_DECODER_TEST_FAST */

    return 0;
}
