/*****************************************************************
|
|   BlueTune - Async Player API Example
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>

#include "BlueTune.h"

/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    BLT_Player* player;
    const char* input_name;
} PlayerState;

/*----------------------------------------------------------------------
|    OnSetInputSucceeded
+---------------------------------------------------------------------*/
static void
OnSetInputSucceeded(PlayerState* self)
{
    printf("Input open, playing...\n");
    BLT_Player_Play(self->player);
}

/*----------------------------------------------------------------------
|    OnSetInputFailed
+---------------------------------------------------------------------*/
static void
OnSetInputFailed(PlayerState* self, BLT_Result result)
{
    printf("Cannot open input (%d)\n", result);
}
 
/*----------------------------------------------------------------------
|    OnEvent
+---------------------------------------------------------------------*/
static void
OnEvent(void* instance, const BLT_Player_Event* event)
{
    (void)instance; /* avoid compiler warnings */
    
    switch (event->type) {
        case BLT_PLAYER_EVENT_TYPE_ACK:
            printf("ACK: command=%d\n", ((BLT_Player_AckEvent*)event)->command);
            if (((BLT_Player_AckEvent*)event)->command == BLT_PLAYER_COMMAND_ID_SET_INPUT) {
                OnSetInputSucceeded(instance);
            }
            break;
            
        case BLT_PLAYER_EVENT_TYPE_NACK:
            printf("NACK: command=%d, result=%d\n", 
                   ((BLT_Player_NackEvent*)event)->command,
                   ((BLT_Player_NackEvent*)event)->result);
            if (((BLT_Player_AckEvent*)event)->command == BLT_PLAYER_COMMAND_ID_SET_INPUT) {
                OnSetInputFailed(instance, ((BLT_Player_NackEvent*)event)->result);
            }
            break;
            
        case BLT_PLAYER_EVENT_TYPE_DECODER_STATE_NOTIFICATION:
            printf("DECODER_STATE_NOTIFICATION: state=");
            switch(((BLT_Player_DecoderStateNotificationEvent*)event)->state) {
                case BLT_PLAYER_DECODER_STATE_PLAYING: printf("PLAYING"); break;
                case BLT_PLAYER_DECODER_STATE_STOPPED: printf("STOPPED"); break;
                case BLT_PLAYER_DECODER_STATE_PAUSED:  printf("PAUSED");  break;
                case BLT_PLAYER_DECODER_STATE_EOS:     printf("EOS");     break;
            }
            printf("\n");
            break;
            
        case BLT_PLAYER_EVENT_TYPE_STREAM_TIMECODE_NOTIFICATION:
            printf("STREAM_TIMECODE_NOTIFICATION: timecode=%02d:%02d:%02d\n", 
                   ((BLT_Player_StreamTimeCodeNotificationEvent*)event)->timecode.h,
                   ((BLT_Player_StreamTimeCodeNotificationEvent*)event)->timecode.m,
                   ((BLT_Player_StreamTimeCodeNotificationEvent*)event)->timecode.s);
            break;

        case BLT_PLAYER_EVENT_TYPE_STREAM_POSITION_NOTIFICATION:
            printf("STREAM_POSITION_NOTIFICATION: position=%d/%d\n", 
                   (int)((BLT_Player_StreamPositionNotificationEvent*)event)->position.offset,
                   (int)((BLT_Player_StreamPositionNotificationEvent*)event)->position.range);
            break;

        case BLT_PLAYER_EVENT_TYPE_STREAM_INFO_NOTIFICATION:
            printf("STREAM_INFO_NOTIFICATION:\n");
            if (((BLT_Player_StreamInfoNotificationEvent*)event)->update_mask & BLT_STREAM_INFO_MASK_NOMINAL_BITRATE) {
                printf("INFO: Nominal Bitrate = %d\n", ((BLT_Player_StreamInfoNotificationEvent*)event)->info.nominal_bitrate);
            }
            if (((BLT_Player_StreamInfoNotificationEvent*)event)->update_mask & BLT_STREAM_INFO_MASK_AVERAGE_BITRATE) {
                printf("  INFO: Average Bitrate = %d\n", ((BLT_Player_StreamInfoNotificationEvent*)event)->info.average_bitrate);
            }
            if (((BLT_Player_StreamInfoNotificationEvent*)event)->update_mask & BLT_STREAM_INFO_MASK_INSTANT_BITRATE) {
                printf("  INFO: Instant Bitrate = %d\n", ((BLT_Player_StreamInfoNotificationEvent*)event)->info.instant_bitrate);
            }
            if (((BLT_Player_StreamInfoNotificationEvent*)event)->update_mask & BLT_STREAM_INFO_MASK_SAMPLE_RATE) {
                printf("  INFO: Sample Rate = %d\n", ((BLT_Player_StreamInfoNotificationEvent*)event)->info.sample_rate);
            }
            if (((BLT_Player_StreamInfoNotificationEvent*)event)->update_mask & BLT_STREAM_INFO_MASK_CHANNEL_COUNT) {
                printf("  INFO: Channels = %d\n", ((BLT_Player_StreamInfoNotificationEvent*)event)->info.channel_count);
            }
            if (((BLT_Player_StreamInfoNotificationEvent*)event)->update_mask & BLT_STREAM_INFO_MASK_SIZE) {
                printf("  INFO: Stream Size = %d\n", (BLT_Int32)((BLT_Player_StreamInfoNotificationEvent*)event)->info.size);
            }
            if (((BLT_Player_StreamInfoNotificationEvent*)event)->update_mask & BLT_STREAM_INFO_MASK_DATA_TYPE) {
                printf("  INFO: Data Type = %s\n", ((BLT_Player_StreamInfoNotificationEvent*)event)->info.data_type?((BLT_Player_StreamInfoNotificationEvent*)event)->info.data_type:"(null)");
            }
            break;

        default:
            printf("EVENT: type=%d\n", (int)event->type);
            break;
    }
}

/*----------------------------------------------------------------------
|    main
+---------------------------------------------------------------------*/
int 
main(int argc, char** argv)
{
    PlayerState state = {NULL, NULL};
    BLT_Result  result;
    
    /* check arguments */
    if (argc != 2) {
        fprintf(stderr, "usage: asyncplayer <input-name>\n");
        return 1;
    }
    state.input_name = argv[1];
    
    printf("=== BlueTune Async Player API Example ===\n");
    printf("Playing %s\n", state.input_name);
    
    /* create the player */
    {
        BLT_Player_EventListener listener = {&state, OnEvent};
        result = BLT_Player_Create(listener, &state.player);
        if (BLT_FAILED(result)) {
            fprintf(stderr, "ERROR: BLT_Player_Create() failed (%d)\n", result);
            return 1;
        }
    }
    
    /* set the input */
    BLT_Player_SetInput(state.player, state.input_name, NULL);
    
    /* wait for messages until we decide to end */
    do {
        result = BLT_Player_PumpMessage(state.player, BLT_TIMEOUT_INFINITE);
    } while (BLT_SUCCEEDED(result));
    
    return 0;
}