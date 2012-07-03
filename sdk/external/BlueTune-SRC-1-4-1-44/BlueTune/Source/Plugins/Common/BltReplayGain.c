/*****************************************************************
|
|   ReplayGain common functions
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "BltReplayGain.h"

/*----------------------------------------------------------------------
|   BLT_ReplayGain_SetStreamProperties
+---------------------------------------------------------------------*/
BLT_Result
BLT_ReplayGain_SetStreamProperties(BLT_Stream*           stream,
                                   float                 track_gain,
                                   BLT_ReplayGainSetMode track_gain_mode,
                                   float                 album_gain,
                                   BLT_ReplayGainSetMode album_gain_mode)
{
    /* quick check */
    if (stream == NULL) return BLT_FAILURE;

    /* set the stream properties */
    if (track_gain_mode != BLT_REPLAY_GAIN_SET_MODE_IGNORE || 
        album_gain_mode != BLT_REPLAY_GAIN_SET_MODE_IGNORE) {
        ATX_Properties* properties;

        /* get a reference to the stream properties */
        if (BLT_SUCCEEDED(BLT_Stream_GetProperties(stream, &properties))) {
            ATX_PropertyValue property_value;
            property_value.type = ATX_PROPERTY_VALUE_TYPE_INTEGER;
            switch (track_gain_mode) {
                case BLT_REPLAY_GAIN_SET_MODE_UPDATE:
                    property_value.data.integer = (ATX_Int32)(track_gain*100.0f);
                    ATX_Properties_SetProperty(properties,
                        BLT_REPLAY_GAIN_TRACK_GAIN_VALUE,
                        &property_value);
                    break;

                case BLT_REPLAY_GAIN_SET_MODE_REMOVE:
                    ATX_Properties_SetProperty(properties,
                        BLT_REPLAY_GAIN_TRACK_GAIN_VALUE,
                        NULL);
                    break;
                    
                case BLT_REPLAY_GAIN_SET_MODE_IGNORE:
                    break;
            }
            switch (track_gain_mode) {
                case BLT_REPLAY_GAIN_SET_MODE_UPDATE:
                    property_value.data.integer = (ATX_Int32)(album_gain*100.0f);
                    ATX_Properties_SetProperty(properties,
                        BLT_REPLAY_GAIN_ALBUM_GAIN_VALUE,
                        &property_value);
                    break;

                case BLT_REPLAY_GAIN_SET_MODE_REMOVE:
                    ATX_Properties_SetProperty(properties,
                        BLT_REPLAY_GAIN_ALBUM_GAIN_VALUE,
                        NULL);
                    break;

                case BLT_REPLAY_GAIN_SET_MODE_IGNORE:
                    break;
            }
        }
    }

    return BLT_SUCCESS;
}
