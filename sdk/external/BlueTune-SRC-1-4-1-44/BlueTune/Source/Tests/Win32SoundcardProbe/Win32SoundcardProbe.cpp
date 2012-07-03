/*****************************************************************
|
|   BlueTune - Command-Line Player
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <mmreg.h>
#include <ks.h>
#include <ksmedia.h>

#include "BlueTune.h"

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#if !defined(WAVE_FORMAT_DOLBY_AC3_SPDIF)
#define WAVE_FORMAT_DOLBY_AC3_SPDIF 0x0092
#endif

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
static GUID SampleFormats[] = {
    {0x00000001,                  0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}},
    {WAVE_FORMAT_DOLBY_AC3_SPDIF, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}}
};

static unsigned int SampleRates[] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 96000};

/*----------------------------------------------------------------------
|    constants
+---------------------------------------------------------------------*/
static void
ProbeFormat(UINT          device_id,
            unsigned int  channel_count,
            unsigned long sample_rate,
            unsigned int  bits_per_sample,
            unsigned int  channel_mask,
            GUID          format_guid)
{
    /* used for 24 and 32 bits per sample as well as multichannel */
    WAVEFORMATEXTENSIBLE format;

    /* fill in format structure */
    format.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
    format.Format.nChannels            = channel_count;
    format.Format.nSamplesPerSec       = sample_rate;
    format.Format.nBlockAlign          = channel_count * bits_per_sample/8;
    format.Format.nAvgBytesPerSec      = format.Format.nBlockAlign * format.Format.nSamplesPerSec;
    format.Format.wBitsPerSample       = bits_per_sample;
    format.Format.cbSize               = 22;
    format.Samples.wValidBitsPerSample = bits_per_sample;
    if (channel_mask && channel_count > 2) {
        format.dwChannelMask = channel_mask;
    } else {
        switch (channel_count) {
            case 1:
                format.dwChannelMask = KSAUDIO_SPEAKER_MONO;
                break;

            case 2:
                format.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
                break;

            case 3:
                format.dwChannelMask = KSAUDIO_SPEAKER_STEREO |
                                       SPEAKER_FRONT_CENTER;
                break;

            case 4:
                format.dwChannelMask = KSAUDIO_SPEAKER_QUAD;
                break;

            case 6:
                format.dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
                break;

            case 8:
                format.dwChannelMask = KSAUDIO_SPEAKER_7POINT1;
                break;

            default:
                format.dwChannelMask = SPEAKER_ALL;
        }
    }
    format.SubFormat = format_guid; 

    MMRESULT mm_result;
    for (unsigned int retry = 0; retry < 10; retry++) {
        mm_result = waveOutOpen(NULL, 
                                device_id, 
                                (const struct tWAVEFORMATEX*)&format,
                                0, 0, WAVE_ALLOWSYNC | WAVE_FORMAT_QUERY);
        if (mm_result != MMSYSERR_ALLOCATED) break;
        Sleep(500);
    }

    if (mm_result == MMSYSERR_ALLOCATED) {
        printf("Device Busy\n");
        return;
    }
    if (mm_result == MMSYSERR_BADDEVICEID) {
        printf("No Such Device\n");
        return;
    }
    
    if (mm_result == MMSYSERR_NODRIVER) {
        printf("No Driver\n");
        return;
    }
    if (mm_result == WAVERR_BADFORMAT) {
        //printf("Format Not Supported\n");
        return;
    }
    if (mm_result != MMSYSERR_NOERROR) {
        printf ("ERROR %x\n", mm_result);
        return;
    }
    printf("ch=%2d, sr=%6d, bps=%2d, format=%s\n",
           channel_count, sample_rate, bits_per_sample, format_guid.Data1 == 1?"PCM":"AC-3");
}

/*----------------------------------------------------------------------
|    ProbeDevice
+---------------------------------------------------------------------*/
static void
ProbeDevice(unsigned int device_id, bool no_pcm)
{
    printf("+++ Probing Device %d ++++++\n", device_id);

    WAVEOUTCAPS caps;
    MMRESULT result = waveOutGetDevCaps(device_id, &caps, sizeof(WAVEOUTCAPS));
    wprintf(L"Name=%s\n", caps.szPname);
    printf("Manufacturer=%x, Product=%x, Version=%x\n", caps.wMid, caps.wPid, caps.vDriverVersion);
    printf("Channels=%d\n", caps.wChannels);
    printf("Flags: ");
    if (caps.dwSupport & WAVECAPS_VOLUME)         printf("VOLUME ");
    if (caps.dwSupport & WAVECAPS_LRVOLUME)       printf("LRVOLUME ");
    if (caps.dwSupport & WAVECAPS_PITCH)          printf("PITCH ");
    if (caps.dwSupport & WAVECAPS_PLAYBACKRATE)   printf("PLAYBACKRATE ");
    if (caps.dwSupport & WAVECAPS_SAMPLEACCURATE) printf("SAMPLEACCURATE ");
    printf("\n");

    for (unsigned int f=0; f<sizeof(SampleFormats)/sizeof(SampleFormats[0]); f++) {
        if (no_pcm && SampleFormats[f].Data1 == 1) continue;
        for (unsigned int channel_count=1; channel_count<=8; channel_count++) {
            for (unsigned int sr=0; sr<sizeof(SampleRates)/sizeof(SampleRates[0]); sr++) {
                for (unsigned int bits_per_sample=8; bits_per_sample<=32; bits_per_sample += 8) {
                    ProbeFormat(device_id, channel_count, SampleRates[sr], bits_per_sample, 0, SampleFormats[f]);
                }
            }
        }
    }

    printf("\n");
}

/*----------------------------------------------------------------------
|    main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    bool no_pcm = false;
    if (argc >= 2 && !strcmp(argv[1], "--no-pcm")) no_pcm = true;

    printf("------- Win32 Soundcard Probe ------ built " __DATE__ "\n");
    unsigned int num_devs = waveOutGetNumDevs(); 
    printf("Found %d devices\n", num_devs);
    for (unsigned int i=0; i<num_devs; i++) {
        ProbeDevice(i, no_pcm);
    }

}