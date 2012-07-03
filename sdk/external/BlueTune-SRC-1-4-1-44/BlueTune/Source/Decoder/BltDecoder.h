/*****************************************************************
|
|   BlueTune - Sync Layer
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file
 * BLT_Decoder API
 */

#ifndef _BLT_DECODER_H_
#define _BLT_DECODER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "BltDefs.h"
#include "BltTypes.h"
#include "BltStream.h"
#include "BltTime.h"
#include "BltEventListener.h"

/** @defgroup BLT_Decoder BLT_Decoder Class
 * @{
 */

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_DECODER_DEFAULT_OUTPUT_NAME "!default"

#define BLT_DECODER_PUMP_OPTION_NON_BLOCKING 1

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
/**
 * BLT_Decoder object.
 * This is the synchronous client API.
 * A decoder creates a core engine, and manages a chain of media nodes
 * including the input and output nodes. It acts as a media data 'pump'
 * getting data from the input, processing it through all the media nodes
 * in the chain, until it reaches the output. The decoder also encapsulates
 * the core functions, such as the registration of plugin modules, etc...
 */
typedef struct BLT_Decoder BLT_Decoder;

/**
 * Represents the current status of a BLT_Decoder object.
 */
typedef struct {
    BLT_StreamInfo     stream_info; /**< Stream info       */
    BLT_StreamPosition position;    /**< Stream position   */
    BLT_TimeStamp      time_stamp;  /**< Timestamp         */
} BLT_DecoderStatus;

/**
 * Property scopes represent the scope of a property. The scope indicates
 * to what part of the system the property applies.
 */
typedef enum {
    BLT_PROPERTY_SCOPE_CORE,
    BLT_PROPERTY_SCOPE_STREAM,
    BLT_PROPERTY_SCOPE_MODULE
} BLT_PropertyScope;

/*----------------------------------------------------------------------
|   prototypes
+---------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Create a BLT_Decoder object.
 */
BLT_Result BLT_Decoder_Create(BLT_Decoder** decoder);

/**
 * Destroy a BLT_Decoder object.
 */
BLT_Result BLT_Decoder_Destroy(BLT_Decoder* decoder);

/**
 * Register all the builtin plugins modules with a BLT_Decoder object.
 */
BLT_Result BLT_Decoder_RegisterBuiltins(BLT_Decoder* decoder);

/**
 * Register a specific plugin module with a BLT_Decoder object.
 * @param module Pointer to a module object to register.
 */
BLT_Result BLT_Decoder_RegisterModule(BLT_Decoder* decoder,
                                      BLT_Module*  module);

/**
 * Get a list of all registered modules
 */
BLT_Result BLT_Decoder_EnumerateModules(BLT_Decoder* decoder, ATX_List** modules);

/**
 * Load and Register a plugin.
 * @param name Name of a plugin (filename of the plugin or special name)
 * @param search_flags Flags that select how the actual plugin file will
 * be located. It is an OR'ed combination or the BLT_PLUGIN_LOADER_FLAGS_XXX
 * constants defined in BltDynamicPlugins.h
 */
BLT_Result BLT_Decoder_LoadPlugin(BLT_Decoder* decoder, 
                                  const char*  name, 
                                  BLT_Flags    search_flags);

/**
 * Load and Register all plugins located in a directory.
 * @param directory Path of the directory that contains the plugins
 * @param file_extension File extension of the plugin files, or NULL to load
 * all files regardless of extension. The file extension includes the '.' 
 * characters (ex: ".plugin")
 */
BLT_Result BLT_Decoder_LoadPlugins(BLT_Decoder* decoder, 
                                   const char*  directory,
                                   const char*  file_extension);

/**
 * Set a BLT_Decoder object's input by name.
 * @param name Name of the input.
 * @param type Mime-type of the input, if known, or NULL.
 */
BLT_Result BLT_Decoder_SetInput(BLT_Decoder*  decoder, 
                                BLT_CString   name, 
                                BLT_CString   type);

/**
 * Set a BLT_Decoder object's input node.
 * @param node The node that will become the new input.
 * @param type Mime-type of the input, if known, or NULL.
 */
BLT_Result BLT_Decoder_SetInputNode(BLT_Decoder*   decoder, 
                                    BLT_CString    name,
                                    BLT_CString    port,
                                    BLT_MediaNode* node);

/**
 * Get a BLT_Decoder object's input node.
 * @param node Address of a pointer to where the decoder's
 * input node will be returned.
 */
BLT_Result BLT_Decoder_GetInputNode(BLT_Decoder*    decoder, 
                                    BLT_MediaNode** node);

/**
 * Set a BLT_Decoder object's output by name.
 * @param name Name of the output.
 * @param type Mime-type of the output, if known, or NULL.
 */
BLT_Result BLT_Decoder_SetOutput(BLT_Decoder* decoder, 
                                 BLT_CString  name, 
                                 BLT_CString  type);

/**
 * Set a BLT_Decoder object's output node.
 * @param node The node object that will be the decoder's output node.
 * @param type Mime-type of the output, if known, or NULL.
 */
BLT_Result BLT_Decoder_SetOutputNode(BLT_Decoder*   decoder, 
                                     BLT_CString    name,
                                     BLT_MediaNode* node);

/**
 * Get a BLT_Decoder object's output node.
 * @param node Address of a pointer to where the decoder's
 * output node will be returned.
 */
BLT_Result BLT_Decoder_GetOutputNode(BLT_Decoder*    decoder, 
                                     BLT_MediaNode** node);


/**
 * Add a node to a BLT_Decoder object's stream node graph.
 * @param where Pointer to the node before which the new node will
 * be added. If this parameter is NULL, the node will be added before
 * the output node.
 * @param name Name of the node to instantiate and add.
 */
BLT_Result BLT_Decoder_AddNodeByName(BLT_Decoder*   decoder, 
                                     BLT_MediaNode* where,
                                     BLT_CString    name);

/**
 * Get the audio volume of the output.
 */
BLT_Result BLT_Decoder_GetVolume(BLT_Decoder* decoder, float* volume);

/**
 * Set the audio volume of the output.
 */
BLT_Result BLT_Decoder_SetVolume(BLT_Decoder* decoder, float volume);

/**
 * Get the ATX_Properties object representing the properties of a
 * BLT_Decoder object.
 */
BLT_Result BLT_Decoder_GetProperties(BLT_Decoder*     decoder,
                                     ATX_Properties** properties);

/**
 * Get the current status of a BLT_Decoder object.
 * @param status Pointer to a BLT_DecoderStatus structure where the
 * status will be returned.
 */
BLT_Result BLT_Decoder_GetStatus(BLT_Decoder*       decoder,
                                 BLT_DecoderStatus* status);

/**
 * Get the ATX_Properties object representing the properties of a
 * BLT_Decoder object's stream.
 * @param properties Pointer to a pointer where a pointer to an 
 * APX_Properties object will be returned. The caller can then call
 * methods of the ATX_Properties object to query stream properties.
 */
BLT_Result BLT_Decoder_GetStreamProperties(BLT_Decoder*     decoder,
                                           ATX_Properties** properties);

/**
 * Process on media packet through a BLT_Decoder object's stream.
 */
BLT_Result BLT_Decoder_PumpPacket(BLT_Decoder* decoder);

/**
 * Process on media packet through a BLT_Decoder object's stream, 
 * with options. 
 * @param options Boolean options flags (combination of 0 or more
 * BLT_DECODER_PUMP_OPTION_XXX flags). If this value is 0, the behaviour
 * is the same as BLT_Decoder_PumpPacket()
 */
BLT_Result BLT_Decoder_PumpPacketWithOptions(BLT_Decoder* decoder, BLT_Flags options);

/**
 * Tell a BLT_Decoder object that decoding is stopped. This allows the
 * media nodes in the decoder's graph to release some resources if they
 * need to. There is no special function to call when decoding resumes,
 * as a call to BLT_Decoder_PumpPacket will automatically signal all the
 * media nodes to restart if necessary.
 */
BLT_Result BLT_Decoder_Stop(BLT_Decoder* decoder);

/**
 * Tell a BLT_Decoder object that decoding is paused. This allows the
 * media nodes in the decoder's graph to release some resources if they
 * need to. There is no special function to call when decoding resumes,
 * as a call to BLT_Decoder_PumpPacket will automatically signal all the
 * media nodes to restart if necessary.
 * The difference between pausing and stopping is that pausing can
 * be resumed without any gaps, whereas stopping may release internal
 * resources and flush decoding buffers, so continuing decoder after a 
 * stop may result in an audible gap.
 */
BLT_Result BLT_Decoder_Pause(BLT_Decoder* decoder);

/**
 * Seek to a specific time.
 * @param Time to which to seek, in milliseconds.
 */
BLT_Result BLT_Decoder_SeekToTime(BLT_Decoder* decoder, BLT_UInt64 time);

/**
 * Drain the output: wait until all buffered audio has been played.
 */
BLT_Result BLT_Decoder_Drain(BLT_Decoder* decoder);

/**
 * Set the output equalizer function
 */
BLT_Result BLT_Decoder_SetEqualizerFunction(BLT_Decoder* decoder, EqualizerConvert function);

/** Seek to a specific position.
 * @param offset Offset between 0 and range
 * @param range Maximum value of offset. The range is an arbitrary
 * scale. For example, if offset=1 and range=2, this means that
 * the decoder should seek to exacly the middle point of the input.
 * Or if offset=25 and range=100, this means that the decoder should
 * seek to the point that is at 25/100 of the total input.
 */
BLT_Result BLT_Decoder_SeekToPosition(BLT_Decoder* decoder,
                                      BLT_UInt64   offset,
                                      BLT_UInt64   range);

/**
 * Set a BLT_Decoder object's event listener. The listener object's
 * notification functions will be called when certain events occur.
 * @param listener Pointer to the BLT_EventListener object that will
 * be notified of events.
 */
BLT_Result BLT_Decoder_SetEventListener(BLT_Decoder*       decoder,
                                        BLT_EventListener* listener);
                                               
#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @} */

#endif /* _BLT_DECODER_H_ */
