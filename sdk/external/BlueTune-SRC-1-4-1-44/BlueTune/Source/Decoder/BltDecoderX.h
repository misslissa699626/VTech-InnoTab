/*****************************************************************
|
|   BlueTune - Decoder (experimental video support)
|
|   (c) 2002-2008 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BLT_DecoderX API
 */

#ifndef _BLT_DecoderX_H_
#define _BLT_DecoderX_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "BltDefs.h"
#include "BltTypes.h"
#include "BltStream.h"
#include "BltTime.h"
#include "BltEventListener.h"
#include "BltDecoder.h"

/** @defgroup BLT_DecoderX BLT_DecoderX Class
 * @{
 */

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct BLT_DecoderX BLT_DecoderX;

/*----------------------------------------------------------------------
|   prototypes
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Create a BLT_DecoderX object.
 */
BLT_Result BLT_DecoderX_Create(BLT_DecoderX** decoder);

/**
 * Destroy a BLT_DecoderX object.
 */
BLT_Result BLT_DecoderX_Destroy(BLT_DecoderX* decoder);

/**
 * Register all the builtin plugins modules with a BLT_DecoderX object.
 */
BLT_Result BLT_DecoderX_RegisterBuiltins(BLT_DecoderX* decoder);

/**
 * Register a specific plugin module with a BLT_DecoderX object.
 * @param module Pointer to a module object to register.
 */
BLT_Result BLT_DecoderX_RegisterModule(BLT_DecoderX* decoder,
                                       BLT_Module*   module);

/**
 * Set a BLT_DecoderX object's input.
 * @param name Name of the input.
 * @param type Mime-type of the input, if known, or NULL.
 */
BLT_Result BLT_DecoderX_SetInput(BLT_DecoderX* decoder, 
                                 BLT_CString   name, 
                                 BLT_CString   type);

/**
 * Set a BLT_DecoderX object's input node.
 * @param node The node that will become the new input.
 * @param type Mime-type of the input, if known, or NULL.
 */
BLT_Result BLT_DecoderX_SetInputNode(BLT_DecoderX*  decoder, 
                                     BLT_CString    name,
                                     BLT_MediaNode* node);

/**
 * Set a BLT_DecoderX object's output.
 * @param name Name of the output.
 * @param type Mime-type of the output, if known, or NULL.
 */
BLT_Result BLT_DecoderX_SetAudioOutput(BLT_DecoderX* decoder, 
                                       BLT_CString   name, 
                                       BLT_CString   type);
BLT_Result BLT_DecoderX_SetVideoOutput(BLT_DecoderX* decoder, 
                                       BLT_CString   name, 
                                       BLT_CString   type);

/**
 * Set a BLT_DecoderX object's output node.
 * @param node The node object that will be the decoder's output node.
 * @param type Mime-type of the output, if known, or NULL.
 */
BLT_Result BLT_DecoderX_SetAudioOutputNode(BLT_DecoderX*  decoder, 
                                           BLT_CString    name,
                                           BLT_MediaNode* node);
BLT_Result BLT_DecoderX_SetVideoOutputNode(BLT_DecoderX*  decoder, 
                                           BLT_CString    name,
                                           BLT_MediaNode* node);


/**
 * Get the audio output node of a BLT_DecoderX.
 * @param output Pointer to a BLT_MediaNode where the pointer to the 
 * decoder's audio output media node will be returned.
 */
BLT_Result BLT_DecoderX_GetAudioOutputNode(BLT_DecoderX*   decoder, 
                                           BLT_MediaNode** output);

/**
 * Get the video output node of a BLT_DecoderX.
 * @param output Pointer to a BLT_MediaNode where the pointer to the 
 * decoder's video output media node node will be returned.
 */
BLT_Result BLT_DecoderX_GetVideoOutputNode(BLT_DecoderX*   decoder, 
                                           BLT_MediaNode** output);

/**
 * Add a node to a BLT_DecoderX object's stream node graph.
 * @param stream 
 * @param where Pointer to the node before which the new node will
 * be added. If this parameter is NULL, the node will be added before
 * the output node.
 * @param name Name of the node to instantiate and add.
 */
BLT_Result BLT_DecoderX_AddNodeByName(BLT_DecoderX*  decoder, 
                                      const char*    stream,
                                      BLT_MediaNode* where,
                                      BLT_CString    name);

/**
 * Get the audio volume of the output.
 */
BLT_Result BLT_DecoderX_GetVolume(BLT_DecoderX* decoder, float* volume);

/**
 * Set the audio volume of the output.
 */
BLT_Result BLT_DecoderX_SetVolume(BLT_DecoderX* decoder, float volume);


/**
 * Get the ATX_Properties object representing the properties of a
 * BLT_DecoderX object.
 */
BLT_Result BLT_DecoderX_GetProperties(BLT_DecoderX*    decoder,
                                      ATX_Properties** properties);

/**
 * Get the current status of a BLT_DecoderX object.
 * @param status Pointer to a BLT_DecoderXStatus structure where the
 * status will be returned.
 */
BLT_Result BLT_DecoderX_GetStatus(BLT_DecoderX*      decoder,
                                  BLT_DecoderStatus* status);

/**
 * Get the ATX_Properties object representing the properties of a
 * BLT_DecoderX object's stream.
 * @param properties Pointer to a pointer where a pointer to an 
 * APX_Properties object will be returned. The caller can then call
 * methods of the ATX_Properties object to query stream properties.
 */
BLT_Result BLT_DecoderX_GetInputStreamProperties(BLT_DecoderX*    decoder,
                                                 ATX_Properties** properties);
BLT_Result BLT_DecoderX_GetAudioStreamProperties(BLT_DecoderX*    decoder,
                                                 ATX_Properties** properties);
BLT_Result BLT_DecoderX_GetVideoStreamProperties(BLT_DecoderX*    decoder,
                                                 ATX_Properties** properties);

/**
 * Process on media packet through a BLT_DecoderX object's stream.
 */
BLT_Result BLT_DecoderX_PumpPacket(BLT_DecoderX* decoder);

/**
 * Tell a BLT_DecoderX object that decoding is stopped. This allows the
 * media nodes in the decoder's graph to release some resources if they
 * need to. There is no special function to call when decoding resumes,
 * as a call to BLT_DecoderX_PumpPacket will automatically signal all the
 * media nodes to restart if necessary.
 */
BLT_Result BLT_DecoderX_Stop(BLT_DecoderX* decoder);

/**
 * Tell a BLT_DecoderX object that decoding is paused. This allows the
 * media nodes in the decoder's graph to release some resources if they
 * need to. There is no special function to call when decoding resumes,
 * as a call to BLT_DecoderX_PumpPacket will automatically signal all the
 * media nodes to restart if necessary.
 * The difference between pausing and stopping is that pausing can
 * be resumed without any gaps, whereas stopping may release internal
 * resources and flush decoding buffers, so continuing decoder after a 
 * stop may result in an audible gap.
 */
BLT_Result BLT_DecoderX_Pause(BLT_DecoderX* decoder);

/**
 * Seek to a specific time.
 * @param Time to which to seek, in milliseconds.
 */
BLT_Result BLT_DecoderX_SeekToTime(BLT_DecoderX* decoder, BLT_UInt64 time);

/** Seek to a specific position.
 * @param offset Offset between 0 and range
 * @param range Maximum value of offset. The range is an arbitrary
 * scale. For example, if offset=1 and range=2, this means that
 * the decoder should seek to exacly the middle point of the input.
 * Or if offset=25 and range=100, this means that the decoder should
 * seek to the point that is at 25/100 of the total input.
 */
BLT_Result BLT_DecoderX_SeekToPosition(BLT_DecoderX* decoder,
                                       BLT_LargeSize offset,
                                       BLT_LargeSize range);

/**
 * Set a BLT_DecoderX object's event listener. The listener object's
 * notification functions will be called when certain events occur.
 * @param listener Pointer to the BLT_EventListener object that will
 * be notified of events.
 */
BLT_Result BLT_DecoderX_SetEventListener(BLT_DecoderX*      decoder,
                                         BLT_EventListener* listener);
                                               
#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* _BLT_DecoderX_H_ */
