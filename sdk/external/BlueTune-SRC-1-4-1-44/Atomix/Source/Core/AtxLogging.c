/*****************************************************************
|
|   Atomix - Logging Support
|
| Copyright (c) 2002-2010, Axiomatic Systems, LLC.
| All rights reserved.
|
| Redistribution and use in source and binary forms, with or without
| modification, are permitted provided that the following conditions are met:
|     * Redistributions of source code must retain the above copyright
|       notice, this list of conditions and the following disclaimer.
|     * Redistributions in binary form must reproduce the above copyright
|       notice, this list of conditions and the following disclaimer in the
|       documentation and/or other materials provided with the distribution.
|     * Neither the name of Axiomatic Systems nor the
|       names of its contributors may be used to endorse or promote products
|       derived from this software without specific prior written permission.
|
| THIS SOFTWARE IS PROVIDED BY AXIOMATIC SYSTEMS ''AS IS'' AND ANY
| EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
| WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
| DISCLAIMED. IN NO EVENT SHALL AXIOMATIC SYSTEMS BE LIABLE FOR ANY
| DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
| (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
| ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
| (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
| SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
|
 ****************************************************************/
/** @file
* Implementation file for logging
*/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include <stdarg.h>
#include "AtxConfig.h"
#include "AtxConsole.h"
#include "AtxTypes.h"
#include "AtxUtils.h"
#include "AtxResults.h"
#include "AtxLogging.h"
#include "AtxSystem.h"
#include "AtxString.h"
#include "AtxList.h"
#include "AtxDataBuffer.h"
#include "AtxFile.h"
#include "AtxStreams.h"
#include "AtxReferenceable.h"
#include "AtxDestroyable.h"
#include "AtxSockets.h"
#include "AtxDebug.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef struct {
    ATX_String key;
    ATX_String value;
} ATX_LogConfigEntry;

typedef struct {
    ATX_List*            config;
    ATX_List*            loggers;
    ATX_Logger*          root;
    ATX_LogManagerLocker locker;
    ATX_Boolean          initialized;
} ATX_LogManager;

typedef struct {
    ATX_UInt32  outputs;
    ATX_Boolean use_colors;
    ATX_Flags   format_filter;
} ATX_LogConsoleHandler;

typedef struct {
    ATX_OutputStream* stream;
} ATX_LogFileHandler;

typedef struct {
    ATX_String        host;
    ATX_UInt16        port;
    ATX_OutputStream* stream;
    ATX_UInt32        sequence_number;
} ATX_LogTcpHandler;

typedef struct {
    ATX_DatagramSocket* socket;
    ATX_SocketAddress   address;
    ATX_UInt32          sequence_number;
} ATX_LogUdpHandler;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define ATX_LOG_HEAP_BUFFER_INCREMENT 4096
#define ATX_LOG_STACK_BUFFER_MAX_SIZE 512
#define ATX_LOG_HEAP_BUFFER_MAX_SIZE  65536

#if !defined(ATX_CONFIG_LOG_CONFIG_ENV)
#define ATX_CONFIG_LOG_CONFIG_ENV "ATOMIX_LOG_CONFIG"
#endif

#if !defined(ATX_CONFIG_DEFAULT_LOG_CONFIG_SOURCE)
#define ATX_CONFIG_DEFAULT_LOG_CONFIG_SOURCE "file:atomix-logging.properties"
#endif

#if !defined(ATX_CONFIG_DEFAULT_LOG_LEVEL)
#define ATX_CONFIG_DEFAULT_LOG_LEVEL ATX_LOG_LEVEL_OFF
#endif
#define ATX_LOG_ROOT_DEFAULT_HANDLER   "ConsoleHandler"
#if !defined(ATX_CONFIG_DEFAULT_LOG_FILE_HANDLER_FILENAME)
#define ATX_CONFIG_DEFAULT_LOG_FILE_HANDLER_FILENAME "_atomix.log"
#endif

#define ATX_LOG_TCP_HANDLER_DEFAULT_PORT            7723
#define ATX_LOG_TCP_HANDLER_DEFAULT_CONNECT_TIMEOUT 5000 /* 5 seconds */

#define ATX_LOG_UDP_HANDLER_DEFAULT_PORT             7724
#define ATX_LOG_UDP_HANDLER_DEFAULT_RESOLVER_TIMEOUT 10000 /* 10 seconds */

#if defined(_WIN32) || defined(_WIN32_WCE)
#define ATX_LOG_CONSOLE_HANDLER_DEFAULT_COLOR_MODE ATX_FALSE
#else
#define ATX_LOG_CONSOLE_HANDLER_DEFAULT_COLOR_MODE ATX_TRUE
#endif
#define ATX_LOG_CONSOLE_HANDLER_DEFAULT_OUTPUTS   1
#define ATX_LOG_CONSOLE_HANDLER_OUTPUT_TO_DEBUG   1
#define ATX_LOG_CONSOLE_HANDLER_OUTPUT_TO_CONSOLE 2

#define ATX_LOG_FORMAT_FILTER_NO_SOURCE        1
#define ATX_LOG_FORMAT_FILTER_NO_TIMESTAMP     2
#define ATX_LOG_FORMAT_FILTER_NO_FUNCTION_NAME 4

/*----------------------------------------------------------------------
|   globals
+---------------------------------------------------------------------*/
static ATX_LogManager LogManager;

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
#define ATX_LOG_MANAGER_LOCK do {                                                       \
    if (LogManager.locker.iface != NULL) {                                              \
        ATX_Result result = LogManager.locker.iface->Lock(LogManager.locker.instance);  \
        if (ATX_FAILED(result)) return result;                                          \
    }                                                                                   \
} while (0)

#define ATX_LOG_MANAGER_UNLOCK do {                                                     \
    if (LogManager.locker.iface != NULL) {                                              \
        ATX_Result result = LogManager.locker.iface->Unlock(LogManager.locker.instance);\
        if (ATX_FAILED(result)) return result;                                          \
    }                                                                                   \
} while (0)

/*----------------------------------------------------------------------
|   forward references
+---------------------------------------------------------------------*/
static ATX_Logger* ATX_Logger_Create(const char* name);
static ATX_Result ATX_Logger_Destroy(ATX_Logger* self);
static ATX_Result ATX_LogConsoleHandler_Create(const char*     logger_name, 
                                               ATX_LogHandler* handler);
static ATX_Result ATX_LogFileHandler_Create(const char*     logger_name, 
                                            ATX_LogHandler* handler);
static ATX_Result ATX_LogTcpHandler_Create(const char*     logger_name, 
                                           ATX_LogHandler* handler);
static ATX_Result ATX_LogUdpHandler_Create(const char*     logger_name, 
                                           ATX_LogHandler* handler);
static ATX_Result ATX_LogNullHandler_Create(const char*     logger_name, 
                                            ATX_LogHandler* handler);

/*----------------------------------------------------------------------
|   ATX_LogHandler_Create
+---------------------------------------------------------------------*/
static ATX_Result
ATX_LogHandler_Create(const char*     logger_name,
                      const char*     handler_name, 
                      ATX_LogHandler* handler)
{
    if (ATX_StringsEqual(handler_name, "NullHandler")) {
        return ATX_LogNullHandler_Create(logger_name, handler);
    } else if (ATX_StringsEqual(handler_name, "FileHandler")) {
        return ATX_LogFileHandler_Create(logger_name, handler);
    } else if (ATX_StringsEqual(handler_name, "ConsoleHandler")) {
        return ATX_LogConsoleHandler_Create(logger_name, handler);
    } else if (ATX_StringsEqual(handler_name, "TcpHandler")) {
        return ATX_LogTcpHandler_Create(logger_name, handler);
    } else if (ATX_StringsEqual(handler_name, "UdpHandler")) {
        return ATX_LogUdpHandler_Create(logger_name, handler);
    }

    return ATX_ERROR_NO_SUCH_CLASS;
}

/*----------------------------------------------------------------------
|   ATX_Log_GetLogLevel
+---------------------------------------------------------------------*/
int 
ATX_Log_GetLogLevel(const char* name)
{
    if (       ATX_StringsEqual(name, "FATAL")) {
        return ATX_LOG_LEVEL_SEVERE;
    } else if (ATX_StringsEqual(name, "SEVERE")) {
        return ATX_LOG_LEVEL_WARNING;
    } else if (ATX_StringsEqual(name, "WARNING")) {
        return ATX_LOG_LEVEL_WARNING;
    } else if (ATX_StringsEqual(name, "INFO")) {
        return ATX_LOG_LEVEL_INFO;
    } else if (ATX_StringsEqual(name, "FINE")) {
        return ATX_LOG_LEVEL_FINE;
    } else if (ATX_StringsEqual(name, "FINER")) {
        return ATX_LOG_LEVEL_FINER;
    } else if (ATX_StringsEqual(name, "FINEST")) {
        return ATX_LOG_LEVEL_FINEST;
    } else if (ATX_StringsEqual(name, "ALL")) {
        return ATX_LOG_LEVEL_ALL;
    } else if (ATX_StringsEqual(name, "OFF")) {
        return ATX_LOG_LEVEL_OFF;
    } else {
        return -1;
    }
}

/*----------------------------------------------------------------------
|   ATX_Log_GetLogLevelName
+---------------------------------------------------------------------*/
const char*
ATX_Log_GetLogLevelName(int level)
{
    switch (level) {
        case ATX_LOG_LEVEL_FATAL:   return "FATAL";
        case ATX_LOG_LEVEL_SEVERE:  return "SEVERE";
        case ATX_LOG_LEVEL_WARNING: return "WARNING";
        case ATX_LOG_LEVEL_INFO:    return "INFO";
        case ATX_LOG_LEVEL_FINE:    return "FINE";
        case ATX_LOG_LEVEL_FINER:   return "FINER";
        case ATX_LOG_LEVEL_FINEST:  return "FINEST";
        case ATX_LOG_LEVEL_OFF:     return "OFF";
        default:                    return "";
    }
}

/*----------------------------------------------------------------------
|   ATX_Log_GetLogLevelAnsiColor
+---------------------------------------------------------------------*/
static const char*
ATX_Log_GetLogLevelAnsiColor(int level)
{
    switch (level) {
        case ATX_LOG_LEVEL_FATAL:   return "31";
        case ATX_LOG_LEVEL_SEVERE:  return "31";
        case ATX_LOG_LEVEL_WARNING: return "33";
        case ATX_LOG_LEVEL_INFO:    return "32";
        case ATX_LOG_LEVEL_FINE:    return "34";
        case ATX_LOG_LEVEL_FINER:   return "35";
        case ATX_LOG_LEVEL_FINEST:  return "36";
        default:                    return NULL;
    }
}

/*----------------------------------------------------------------------
|   ATX_LogManager_ConfigValueIsBooleanTrue
+---------------------------------------------------------------------*/
static ATX_Boolean
ATX_LogManager_ConfigValueIsBooleanTrue(ATX_String* value)
{
    return 
        ATX_String_Compare(value, "true", ATX_TRUE) == 0 ||
        ATX_String_Compare(value, "yes",  ATX_TRUE) == 0 ||
        ATX_String_Compare(value, "on",   ATX_TRUE) == 0 ||
        ATX_String_Compare(value, "1",    ATX_FALSE) == 0;
}

/*----------------------------------------------------------------------
|   ATX_LogManager_ConfigValueIsBooleanFalse
+---------------------------------------------------------------------*/
static ATX_Boolean
ATX_LogManager_ConfigValueIsBooleanFalse(ATX_String* value)
{
    return 
        ATX_String_Compare(value, "false", ATX_TRUE) == 0  ||
        ATX_String_Compare(value, "no",    ATX_TRUE) == 0  ||
        ATX_String_Compare(value, "off",   ATX_TRUE) == 0  ||
        ATX_String_Compare(value, "0",     ATX_FALSE) == 0;
}

/*----------------------------------------------------------------------
|   ATX_LogManager_GetConfigValue
+---------------------------------------------------------------------*/
static ATX_String*
ATX_LogManager_GetConfigValue(const char* prefix, const char* suffix)
{
    ATX_ListItem* item = ATX_List_GetFirstItem(LogManager.config);
    ATX_Size      prefix_length = prefix?ATX_StringLength(prefix):0;
    ATX_Size      suffix_length = suffix?ATX_StringLength(suffix):0;
    ATX_Size      key_length    = prefix_length+suffix_length;
    while (item) {
        ATX_LogConfigEntry* entry = (ATX_LogConfigEntry*)ATX_ListItem_GetData(item);
        if (ATX_String_GetLength(&entry->key) == key_length &&
            (prefix == NULL || ATX_String_StartsWith(&entry->key, prefix)) &&
            (suffix == NULL || ATX_String_EndsWith(&entry->key, suffix  )) ) {
            return &entry->value;
        }
        item = ATX_ListItem_GetNext(item);
    }

    /* not found */
    return NULL;
}

/*----------------------------------------------------------------------
|   ATX_LogManager_SetConfigValue
+---------------------------------------------------------------------*/
static ATX_Result
ATX_LogManager_SetConfigValue(const char* key, const char* value)
{
    ATX_String* value_string = ATX_LogManager_GetConfigValue(key, NULL);
    if (value_string) {
        /* the key already exists, replace the value */
        return ATX_String_Assign(value_string, value);
    } else {
        /* the value does not already exist, create a new one */
        ATX_Result result;
        ATX_LogConfigEntry* entry = ATX_AllocateMemory(sizeof(ATX_LogConfigEntry));
        if (entry == NULL) return ATX_ERROR_OUT_OF_MEMORY;
        result = ATX_List_AddData(LogManager.config, (void*)entry);
        if (ATX_FAILED(result)) {
            ATX_FreeMemory((void*)entry);
            return result;
        }
        entry->key = ATX_String_Create(key);
        entry->value = ATX_String_Create(value);
    }

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_LogManager_ClearConfig
+---------------------------------------------------------------------*/
static ATX_Result
ATX_LogManager_ClearConfig() 
{
    ATX_ListItem* item = ATX_List_GetFirstItem(LogManager.config);
    while (item) {
        ATX_LogConfigEntry* entry = (ATX_LogConfigEntry*)ATX_ListItem_GetData(item);
        ATX_String_Destruct(&entry->key);
        ATX_String_Destruct(&entry->value);
        ATX_FreeMemory((void*)entry);
        item = ATX_ListItem_GetNext(item);
    }
    ATX_List_Clear(LogManager.config);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_LogManager_ParseConfig
+---------------------------------------------------------------------*/
static ATX_Result
ATX_LogManager_ParseConfig(const char* config,
                           ATX_Size    config_size) 
{
    const char* cursor    = config;
    const char* line      = config;
    const char* separator = NULL;
    ATX_String  key       = ATX_EMPTY_STRING;
    ATX_String  value     = ATX_EMPTY_STRING;

    /* parse all entries */
    while (cursor <= config+config_size) {
        /* separators are newlines, ';' or end of buffer */
        if ( cursor == config+config_size ||
            *cursor == '\n'              || 
            *cursor == '\r'              || 
            *cursor == ';') {
            /* newline or end of buffer */
            if (separator && line[0] != '#') {
                /* we have a property */
                ATX_String_AssignN(&key,   line,                    (ATX_Size)(separator-line));
                ATX_String_AssignN(&value, line+(separator+1-line), (ATX_Size)(cursor-(separator+1)));
                ATX_String_TrimWhitespace(&key);
                ATX_String_TrimWhitespace(&value);
            
                ATX_LogManager_SetConfigValue(ATX_CSTR(key), ATX_CSTR(value));
            }
            line = cursor+1;
            separator = NULL;
        } else if (*cursor == '=' && separator == NULL) {
            separator = cursor;
        }
        cursor++;
    }

    /* cleanup */
    ATX_String_Destruct(&key);
    ATX_String_Destruct(&value);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_LogManager_ParseConfigFile
+---------------------------------------------------------------------*/
static ATX_Result
ATX_LogManager_ParseConfigFile(const char* filename) 
{
    ATX_Result result;

    /* load the file */
    ATX_DataBuffer* buffer = NULL;
    result = ATX_LoadFile(filename, &buffer);
    if (ATX_FAILED(result)) return result;

    /* parse the config */
    result = ATX_LogManager_ParseConfig((const char*)ATX_DataBuffer_GetData(buffer),
                                        ATX_DataBuffer_GetDataSize(buffer));

    /* destroy the buffer */
    ATX_DataBuffer_Destroy(buffer);

    return result;
}

/*----------------------------------------------------------------------
|   ATX_LogManager_ParseConfigSource
+---------------------------------------------------------------------*/
static ATX_Result
ATX_LogManager_ParseConfigSource(ATX_String* source) 
{
    if (ATX_String_StartsWith(source, "file:")) {
        /* file source */
        ATX_LogManager_ParseConfigFile(ATX_CSTR(*source)+5);
    } else if (ATX_String_StartsWith(source, "plist:")) {
        ATX_LogManager_ParseConfig(ATX_CSTR(*source)+6,
                                   ATX_String_GetLength(source)-6);
    } else {
        return ATX_ERROR_INVALID_SYNTAX;
    }

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_LogManager_HaveLoggerConfig
+---------------------------------------------------------------------*/
static ATX_Boolean
ATX_LogManager_HaveLoggerConfig(const char* name)
{
    ATX_ListItem* item = ATX_List_GetFirstItem(LogManager.config);
    ATX_Size      name_length = ATX_StringLength(name);
    while (item) {
        ATX_LogConfigEntry* entry = (ATX_LogConfigEntry*)ATX_ListItem_GetData(item);
        if (ATX_String_StartsWith(&entry->key, name)) {
            const char* suffix = ATX_CSTR(entry->key)+name_length;
            if (ATX_StringsEqual(suffix, ".level") ||
                ATX_StringsEqual(suffix, ".handlers") ||
                ATX_StringsEqual(suffix, ".forward")) {
                return ATX_TRUE;
            }
        }
        item = ATX_ListItem_GetNext(item);
    }

    /* no config found */
    return ATX_FALSE;

}

/*----------------------------------------------------------------------
|   ATX_LogManager_ConfigureLogger
+---------------------------------------------------------------------*/
static ATX_Result
ATX_LogManager_ConfigureLogger(ATX_Logger* logger)
{
    /* configure the level */
    {
        ATX_String* level_value = ATX_LogManager_GetConfigValue(
            ATX_CSTR(logger->name),".level");
        if (level_value) {
            int value;
            /* try a symbolic name */
            value = ATX_Log_GetLogLevel(ATX_CSTR(*level_value));
            if (value < 0) {
                /* try a numeric value */
                if (ATX_FAILED(ATX_String_ToInteger(level_value, &value, ATX_FALSE))) {
                    value = -1;
                }
            }
            if (value >= 0) {
                logger->level = value;
                logger->level_is_inherited = ATX_FALSE;
            }
        }
    }

    /* configure the handlers */
    {
        ATX_String* handlers = ATX_LogManager_GetConfigValue(
            ATX_CSTR(logger->name),".handlers");
        if (handlers) {
            const char*    handlers_list = ATX_CSTR(*handlers);
            const char*    cursor = handlers_list;
            const char*    name_start = handlers_list;
            ATX_String     handler_name = ATX_EMPTY_STRING;
            ATX_LogHandler handler = {NULL, NULL};
            for (;;) {
                if (*cursor == '\0' || *cursor == ',') {
                    if (cursor != name_start) {
                        ATX_String_AssignN(&handler_name, name_start, (ATX_Size)(cursor-name_start));
                        ATX_String_TrimWhitespace(&handler_name);
                        
                        /* create a handler */
                        if (ATX_SUCCEEDED(
                            ATX_LogHandler_Create(ATX_CSTR(logger->name),
                                                  ATX_CSTR(handler_name),
                                                  &handler))) {
                            ATX_Logger_AddHandler(logger, &handler);
                        }

                    }
                    if (*cursor == '\0') break;
                    name_start = cursor+1;
                }
                ++cursor;
            }
            ATX_String_Destruct(&handler_name);
        }
    }

    /* configure the forwarding */
    {
        ATX_String* forward = ATX_LogManager_GetConfigValue(
            ATX_CSTR(logger->name),".forward");
        if (forward && !ATX_LogManager_ConfigValueIsBooleanTrue(forward)) {
            logger->forward_to_parent = ATX_FALSE;
        }
    }

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_LogManager_SetLocker
+---------------------------------------------------------------------*/
ATX_Result
ATX_LogManager_SetLocker(ATX_LogManagerLocker locker)
{
    LogManager.locker = locker;
    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_LogManager_Terminate
+---------------------------------------------------------------------*/
ATX_Result
ATX_LogManager_Terminate(void)
{
    /* check if we're initialized */
    if (!LogManager.initialized) return ATX_ERROR_INVALID_STATE;

    /* destroy everything we've created */
    ATX_LogManager_ClearConfig();
    ATX_List_Destroy(LogManager.config);
    LogManager.config = NULL;

    {
        ATX_ListItem* item = ATX_List_GetFirstItem(LogManager.loggers);
        while (item) {
            ATX_Logger* logger = (ATX_Logger*)ATX_ListItem_GetData(item);
            ATX_Logger_Destroy(logger);
            item = ATX_ListItem_GetNext(item);
        }
    }

    /* destroy the logger list */
    ATX_List_Destroy(LogManager.loggers);
    LogManager.loggers = NULL;

    /* destroy the root logger */
    ATX_Logger_Destroy(LogManager.root);
    LogManager.root = NULL;

    /* we are no longer initialized */
    LogManager.initialized = ATX_FALSE;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_LogManager_AtExitHandler
+---------------------------------------------------------------------*/
static void 
ATX_LogManager_AtExitHandler(void)
{
    ATX_LogManager_Terminate();
}

/*----------------------------------------------------------------------
|   ATX_LogManager_Initialize
+---------------------------------------------------------------------*/
ATX_Result
ATX_LogManager_Initialize(void) 
{
    ATX_String  config_sources_env = ATX_EMPTY_STRING;
    const char* config_sources = ATX_CONFIG_DEFAULT_LOG_CONFIG_SOURCE;

    if (LogManager.initialized) {
        return ATX_SUCCESS;
    }

    /* create a logger list */
    ATX_List_Create(&LogManager.loggers);

    /* create a config */
    ATX_List_Create(&LogManager.config);

    /* set some default config values */
    ATX_LogManager_SetConfigValue(".handlers", ATX_LOG_ROOT_DEFAULT_HANDLER);

    /* see if the config sources have been set to non-default values */
    if (ATX_SUCCEEDED(ATX_GetEnvironment(ATX_CONFIG_LOG_CONFIG_ENV, &config_sources_env))) {
        config_sources = ATX_CSTR(config_sources_env);
    }

    /* load all configs */
    {
        ATX_String config_source = ATX_EMPTY_STRING;
        const char* cursor = config_sources; 
        const char* source = config_sources;
        for (;;) {
            if (*cursor == '\0' || *cursor == '|') {
                if (cursor != source) {
                    ATX_String_AssignN(&config_source, source, (ATX_Size)(cursor-source));
                    ATX_String_TrimWhitespace(&config_source);
                    ATX_LogManager_ParseConfigSource(&config_source);
                }
                if (*cursor == '\0') break;
            }
            cursor++;
        }
        ATX_String_Destruct(&config_source);
        ATX_String_Destruct(&config_sources_env);
    }

    /* create the root logger */
    LogManager.root = ATX_Logger_Create("");
    if (LogManager.root) {
        LogManager.root->level = ATX_CONFIG_DEFAULT_LOG_LEVEL;
        LogManager.root->level_is_inherited = ATX_FALSE;
        ATX_LogManager_ConfigureLogger(LogManager.root);
    }

    /* we are now initialized */
    LogManager.initialized = ATX_TRUE;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_Logger_Create
+---------------------------------------------------------------------*/
static ATX_Logger*
ATX_Logger_Create(const char* name)
{
    /* create a new logger */
    ATX_Logger* logger = 
        (ATX_Logger*)ATX_AllocateZeroMemory(sizeof(ATX_Logger));
    if (logger == NULL) return NULL;

    /* setup the logger */
    logger->level              = ATX_LOG_LEVEL_OFF;
    logger->level_is_inherited = ATX_TRUE;
    logger->name               = ATX_String_Create(name);
    logger->forward_to_parent  = ATX_TRUE;
    logger->parent             = NULL;

    return logger;
}

/*----------------------------------------------------------------------
|   ATX_Logger_Destroy
+---------------------------------------------------------------------*/
static ATX_Result
ATX_Logger_Destroy(ATX_Logger* self)
{
    /* destroy all handlers */
    ATX_LogHandlerEntry* entry = self->handlers;
    while (entry) {
        ATX_LogHandlerEntry* next = entry->next;
        entry->handler.iface->Destroy(&entry->handler);
        ATX_FreeMemory((void*)entry);
        entry = next;
    }
    
    /* destruct other members */
    ATX_String_Destruct(&self->name);

    /* free the object memory */
    ATX_FreeMemory((void*)self);

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_Logger_Log
+---------------------------------------------------------------------*/
void
ATX_Logger_Log(ATX_Logger*  self, 
               int          level, 
               const char*  source_file,
               unsigned int source_line,
               const char*  source_function,
               const char*  msg, 
                            ...)
{
    char     buffer[ATX_LOG_STACK_BUFFER_MAX_SIZE];
    ATX_Size buffer_size = sizeof(buffer);
    char*    message = buffer;
    int      result;
    va_list  args;

    /* check the log level (in case filtering has not already been done) */
    if (level < self->level) return;
        
    for(;;) {
        va_start(args, msg);
        /* try to format the message (it might not fit) */
        result = ATX_FormatStringVN(message, buffer_size-1, msg, args);
        va_end(args);
        if (result >= (int)(buffer_size-1)) result = -1;
        message[buffer_size-1] = 0; /* force a NULL termination */
        if (result >= 0) break;

        /* the buffer was too small, try something bigger */
        buffer_size = (buffer_size+ATX_LOG_HEAP_BUFFER_INCREMENT)*2;
        if (buffer_size > ATX_LOG_HEAP_BUFFER_MAX_SIZE) break;
        if (message != buffer) ATX_FreeMemory((void*)message);
        message = ATX_AllocateMemory(buffer_size);
        if (message == NULL) return;
    }

    {
        /* the message is formatted, publish it to the handlers */
        ATX_LogRecord record;
        ATX_Logger*   logger = self;
        
        /* setup the log record */
        record.logger_name     = ATX_CSTR(logger->name),
        record.level           = level;
        record.message         = message;
        record.source_file     = source_file;
        record.source_line     = source_line;
        record.source_function = source_function;
        ATX_System_GetCurrentTimeStamp(&record.timestamp);

        /* call all handlers for this logger and parents */
        while (logger) {
            /* call all handlers for the current logger */
            ATX_LogHandlerEntry* entry = logger->handlers;
            while (entry) {
                entry->handler.iface->Log(&entry->handler, &record);
                entry = entry->next;
            }

            /* forward to the parent unless this logger does not forward */
            if (logger->forward_to_parent) {
                logger = logger->parent;
            } else {
                break;
            }
        }
    }


    /* free anything we may have allocated */
    if (message != buffer) ATX_FreeMemory((void*)message);
}

/*----------------------------------------------------------------------
|   ATX_Logger_AddHandler
+---------------------------------------------------------------------*/
ATX_Result
ATX_Logger_AddHandler(ATX_Logger* self, ATX_LogHandler* handler)
{
    ATX_LogHandlerEntry* entry;

    /* check parameters */
    if (handler == NULL) return ATX_ERROR_INVALID_PARAMETERS;

    /* allocate a new entry */
    entry = (ATX_LogHandlerEntry*)ATX_AllocateMemory(sizeof(ATX_LogHandlerEntry));
    if (entry == NULL) return ATX_ERROR_OUT_OF_MEMORY;

    /* setup the entry */
    entry->handler = *handler;
    
    /* attach the new entry at the beginning of the list */
    entry->next = self->handlers;
    self->handlers = entry;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_Logger_SetParent
+---------------------------------------------------------------------*/
static ATX_Result
ATX_Logger_SetParent(ATX_Logger* self, ATX_Logger* parent)
{
    ATX_Logger* logger = self;

    /* set our new parent */
    self->parent = parent;

    /* find the first ancestor with its own log level */
    while (logger->level_is_inherited && logger->parent) {
        logger = logger->parent;
    }
    if (logger != self) self->level = logger->level;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_Log_FindLogger
+---------------------------------------------------------------------*/
static ATX_Logger*
ATX_Log_FindLogger(const char* name)
{
    ATX_ListItem* item = ATX_List_GetFirstItem(LogManager.loggers);
    while (item) {
        ATX_Logger* logger = (ATX_Logger*)ATX_ListItem_GetData(item);
        if (ATX_StringsEqual(ATX_CSTR(logger->name), name)) {
            return logger;
        }
        item = ATX_ListItem_GetNext(item);
    }

    return NULL;
}

/*----------------------------------------------------------------------
|   ATX_Log_GetLogger
+---------------------------------------------------------------------*/
ATX_Logger*
ATX_Log_GetLogger(const char* name)
{
    ATX_Logger* logger;

    /* check that the manager is initialized */
    if (!LogManager.initialized) {
        /* init the manager */
        ATX_LogManager_Initialize();
        ATX_ASSERT(LogManager.initialized);

        /* register a function to be called when the program exits */
        ATX_AtExit(ATX_LogManager_AtExitHandler);
    }

    /* check if this logger is already configured */
    logger = ATX_Log_FindLogger(name);
    if (logger) return logger;

    /* create a new logger */
    logger = ATX_Logger_Create(name);
    if (logger == NULL) return NULL;

    /* configure the logger */
    ATX_LogManager_ConfigureLogger(logger);

    /* find which parent to attach to */
    {
        ATX_Logger* parent = LogManager.root;
        ATX_String  parent_name = ATX_String_Create(name);
        for (;;) {
            ATX_Logger* candidate_parent;

            /* find the last dot */
            int dot = ATX_String_ReverseFindChar(&parent_name, '.');
            if (dot < 0) break;
            ATX_String_SetLength(&parent_name, dot);
            
            /* see if the parent exists */
            candidate_parent = ATX_Log_FindLogger(ATX_CSTR(parent_name));
            if (candidate_parent) {
                parent = candidate_parent;
                break;
            }

            /* this parent name does not exist, see if we need to create it */
            if (ATX_LogManager_HaveLoggerConfig(ATX_CSTR(parent_name))) {
                parent = ATX_Log_GetLogger(ATX_CSTR(parent_name));
                break;
            }
        }
        ATX_String_Destruct(&parent_name);

        /* attach to the parent */
        ATX_Logger_SetParent(logger, parent);
    }

    /* add this logger to the list */
    ATX_List_AddData(LogManager.loggers, (void*)logger);

    return logger;
}

/*----------------------------------------------------------------------
|   ATX_LogNullHandler forward references
+---------------------------------------------------------------------*/
static const ATX_LogHandlerInterface ATX_LogNullHandler_Interface;

/*----------------------------------------------------------------------
|   ATX_LogNullHandler_Destroy
+---------------------------------------------------------------------*/
static void
ATX_LogNullHandler_Destroy(ATX_LogHandler* self)
{
    ATX_COMPILER_UNUSED(self);
}

/*----------------------------------------------------------------------
|   ATX_LogNullHandler_Create
+---------------------------------------------------------------------*/
static ATX_Result
ATX_LogNullHandler_Create(const char* logger_name, ATX_LogHandler* handler)
{
    ATX_COMPILER_UNUSED(logger_name);

    /* setup the interface */
    handler->instance = NULL;
    handler->iface    = &ATX_LogNullHandler_Interface;

    return ATX_SUCCESS;
}

/*----------------------------------------------------------------------
|   ATX_LogNullHandler_Log
+---------------------------------------------------------------------*/
static void
ATX_LogNullHandler_Log(ATX_LogHandler* self, const ATX_LogRecord* record)
{
    ATX_COMPILER_UNUSED(self);
    ATX_COMPILER_UNUSED(record);
}

/*----------------------------------------------------------------------
|   ATX_LogNullHandler_Interface
+---------------------------------------------------------------------*/
static const ATX_LogHandlerInterface 
ATX_LogNullHandler_Interface = {
    ATX_LogNullHandler_Log,
    ATX_LogNullHandler_Destroy
};

/*----------------------------------------------------------------------
|   ATX_Log_FormatRecordToStream
+---------------------------------------------------------------------*/
static void
ATX_Log_FormatRecordToStream(const ATX_LogRecord* record,
                             ATX_OutputStream*    stream,
                             ATX_Boolean          use_colors,
                             ATX_Flags            format_filter)
{
    const char* level_name = ATX_Log_GetLogLevelName(record->level);
    char        level_string[16];
    char        buffer[64];
    const char* ansi_color = NULL;

    /* format and emit the record */
    if (level_name[0] == '\0') {
        ATX_IntegerToString(record->level, level_string, sizeof(level_string));
        level_name = level_string;
    }
    if ((format_filter & ATX_LOG_FORMAT_FILTER_NO_SOURCE) == 0) {
        ATX_OutputStream_WriteString(stream, record->source_file);
        ATX_OutputStream_WriteFully(stream, "(", 1);
        ATX_IntegerToStringU(record->source_line, buffer, sizeof(buffer));
        ATX_OutputStream_WriteString(stream, buffer);
        ATX_OutputStream_WriteFully(stream, "): ", 3);
    }
    ATX_OutputStream_WriteFully(stream, "[", 1);
    ATX_OutputStream_WriteString(stream, record->logger_name);
    ATX_OutputStream_WriteFully(stream, "] ", 2);
    if ((format_filter & ATX_LOG_FORMAT_FILTER_NO_TIMESTAMP) == 0) {
        ATX_IntegerToStringU(record->timestamp.seconds, buffer, sizeof(buffer));
        ATX_OutputStream_WriteString(stream, buffer);
        ATX_OutputStream_WriteString(stream, ":");
        ATX_IntegerToStringU(record->timestamp.nanoseconds/1000000L, buffer, sizeof(buffer));
        ATX_OutputStream_WriteString(stream, buffer);
        ATX_OutputStream_WriteFully(stream, " ", 1);
    }
    if ((format_filter & ATX_LOG_FORMAT_FILTER_NO_FUNCTION_NAME) == 0) {
        ATX_OutputStream_WriteFully(stream, "[",1);
        if (record->source_function) {
            ATX_OutputStream_WriteString(stream, record->source_function);
        }
        ATX_OutputStream_WriteFully(stream, "] ",2);
    }
    if (use_colors) {
        ansi_color = ATX_Log_GetLogLevelAnsiColor(record->level);
        if (ansi_color) {
            ATX_OutputStream_WriteFully(stream, "\033[", 2);
            ATX_OutputStream_WriteString(stream, ansi_color);
            ATX_OutputStream_WriteFully(stream, ";1m", 3);
        }
    }
    ATX_OutputStream_WriteString(stream, level_name);
    if (use_colors && ansi_color) {
        ATX_OutputStream_WriteFully(stream, "\033[0m", 4);
    }
    ATX_OutputStream_WriteFully(stream, ": ", 2);
    ATX_OutputStream_WriteString(stream, record->message);
    ATX_OutputStream_WriteFully(stream, "\r\n", 2);
}

/*----------------------------------------------------------------------
|   ATX_LogConsoleHandler forward references
+---------------------------------------------------------------------*/
static const ATX_LogHandlerInterface ATX_LogConsoleHandler_Interface;

/*----------------------------------------------------------------------
|   ATX_LogConsoleHandler_Log
+---------------------------------------------------------------------*/
static void
ATX_LogConsoleHandler_Log(ATX_LogHandler* _self, const ATX_LogRecord* record)
{
    ATX_LogConsoleHandler* self = (ATX_LogConsoleHandler*)_self->instance;
    ATX_MemoryStream*      memory_stream;
    ATX_OutputStream*      output_stream;

    if (ATX_FAILED(ATX_MemoryStream_Create(4096, &memory_stream))) return;
    if (ATX_SUCCEEDED(ATX_MemoryStream_GetOutputStream(memory_stream, &output_stream))) {
        const ATX_DataBuffer* buffer;
        ATX_Log_FormatRecordToStream(record, output_stream, self->use_colors, self->format_filter);
        ATX_OutputStream_WriteFully(output_stream, "\0", 1);
        ATX_MemoryStream_GetBuffer(memory_stream, &buffer);
        if (self->outputs & ATX_LOG_CONSOLE_HANDLER_OUTPUT_TO_CONSOLE) {
            ATX_ConsoleOutput((const char*)ATX_DataBuffer_GetData(buffer));
        }
        if (self->outputs & ATX_LOG_CONSOLE_HANDLER_OUTPUT_TO_DEBUG) {
            ATX_DebugOutput((const char*)ATX_DataBuffer_GetData(buffer));
        }
        ATX_RELEASE_OBJECT(output_stream);
    }
    ATX_MemoryStream_Destroy(memory_stream);
}

/*----------------------------------------------------------------------
|   ATX_LogConsoleHandler_Destroy
+---------------------------------------------------------------------*/
static void
ATX_LogConsoleHandler_Destroy(ATX_LogHandler* _self)
{
    ATX_LogConsoleHandler* self = (ATX_LogConsoleHandler*)_self->instance;

    /* free the object memory */
    ATX_FreeMemory((void*)self);
}

/*----------------------------------------------------------------------
|   ATX_LogConsoleHandler_Create
+---------------------------------------------------------------------*/
static ATX_Result
ATX_LogConsoleHandler_Create(const char*     logger_name,
                             ATX_LogHandler* handler)
{
    ATX_LogConsoleHandler* instance;
    ATX_Result             result = ATX_SUCCESS;

    /* compute a prefix for the configuration of this handler */
    ATX_String logger_prefix = ATX_String_Create(logger_name);
    ATX_CHECK(ATX_String_Append(&logger_prefix, ".ConsoleHandler"));

    /* allocate a new object */
    instance = ATX_AllocateZeroMemory(sizeof(ATX_LogConsoleHandler));
    
    /* configure the object */
    {
        ATX_String* colors;
        instance->use_colors = ATX_LOG_CONSOLE_HANDLER_DEFAULT_COLOR_MODE;
        colors = ATX_LogManager_GetConfigValue(ATX_CSTR(logger_prefix),".colors");
        if (colors) {
            if (ATX_LogManager_ConfigValueIsBooleanTrue(colors)) {
                instance->use_colors = ATX_TRUE;
            } else if (ATX_LogManager_ConfigValueIsBooleanFalse(colors)) {
                instance->use_colors = ATX_FALSE;
            }
        }
    }
    {
        ATX_String* outputs;
        instance->outputs = ATX_LOG_CONSOLE_HANDLER_DEFAULT_OUTPUTS;
        outputs = ATX_LogManager_GetConfigValue(ATX_CSTR(logger_prefix),".outputs");
        if (outputs) {
            int flags;
            ATX_String_ToInteger(outputs, &flags, ATX_TRUE);
            instance->outputs = flags;
        }
    } 
    {
        ATX_String* filter;
        instance->format_filter = 0;
        filter = ATX_LogManager_GetConfigValue(ATX_CSTR(logger_prefix),".filter");
        if (filter) {
            int flags;
            ATX_String_ToInteger(filter, &flags, ATX_TRUE);
            instance->format_filter = flags;
        }
    } 

    /* setup the interface */
    handler->instance = (ATX_LogHandlerInstance*)instance;
    handler->iface    = &ATX_LogConsoleHandler_Interface;

    /* cleanup */
    ATX_String_Destruct(&logger_prefix);

    return result;
}

/*----------------------------------------------------------------------
|   ATX_LogConsoleHandler_Interface
+---------------------------------------------------------------------*/
static const ATX_LogHandlerInterface 
ATX_LogConsoleHandler_Interface = {
    ATX_LogConsoleHandler_Log,
    ATX_LogConsoleHandler_Destroy
};

/*----------------------------------------------------------------------
|   ATX_LogFileHandler forward references
+---------------------------------------------------------------------*/
static const ATX_LogHandlerInterface ATX_LogFileHandler_Interface;

/*----------------------------------------------------------------------
|   ATX_LogFileHandler_Log
+---------------------------------------------------------------------*/
static void
ATX_LogFileHandler_Log(ATX_LogHandler* _self, const ATX_LogRecord* record)
{
    ATX_LogFileHandler* self = (ATX_LogFileHandler*)_self->instance;
    ATX_Log_FormatRecordToStream(record, self->stream, ATX_FALSE, 0);
}

/*----------------------------------------------------------------------
|   ATX_LogFileHandler_Destroy
+---------------------------------------------------------------------*/
static void
ATX_LogFileHandler_Destroy(ATX_LogHandler* _self)
{
    ATX_LogFileHandler* self = (ATX_LogFileHandler*)_self->instance;

    /* release the stream */
    ATX_RELEASE_OBJECT(self->stream);

    /* free the object memory */
    ATX_FreeMemory((void*)self);
}

/*----------------------------------------------------------------------
|   ATX_LogFileHandler_Create
+---------------------------------------------------------------------*/
static ATX_Result
ATX_LogFileHandler_Create(const char*     logger_name,
                          ATX_LogHandler* handler)
{
    ATX_LogFileHandler* instance;
    const char*         filename;
    ATX_String          filename_synth = ATX_EMPTY_STRING;
    ATX_File*           file;
    ATX_Boolean         append = ATX_TRUE;
    ATX_Result          result = ATX_SUCCESS;

    /* compute a prefix for the configuration of this handler */
    ATX_String logger_prefix = ATX_String_Create(logger_name);
    ATX_CHECK(ATX_String_Append(&logger_prefix, ".FileHandler"));

    /* allocate a new object */
    instance = ATX_AllocateZeroMemory(sizeof(ATX_LogFileHandler));
    
    /* configure the object */
    {
        /* filename */
        ATX_String* filename_conf = ATX_LogManager_GetConfigValue(ATX_CSTR(logger_prefix), ".filename");
        if (filename_conf) {
            filename = ATX_CSTR(*filename_conf);
        } else if (logger_name[0]) {
            ATX_String_Reserve(&filename_synth, ATX_StringLength(logger_name));
            ATX_String_Assign(&filename_synth, logger_name);
            ATX_String_Append(&filename_synth, ".log");
            filename = ATX_CSTR(filename_synth);
        } else {
            /* default name for the root logger */
            filename = ATX_CONFIG_DEFAULT_LOG_FILE_HANDLER_FILENAME;
        }
    }
    {
        /* append mode */
        ATX_String* append_mode = ATX_LogManager_GetConfigValue(ATX_CSTR(logger_prefix),".append");
        if (append_mode && !ATX_LogManager_ConfigValueIsBooleanFalse(append_mode)) {
            append = ATX_FALSE;
        }

    }

    /* open the log file */
    if (ATX_SUCCEEDED(ATX_File_Create(filename, &file))) {
        result = ATX_File_Open(file, 
                               ATX_FILE_OPEN_MODE_CREATE |
                               ATX_FILE_OPEN_MODE_WRITE  |
                               (append?ATX_FILE_OPEN_MODE_APPEND:0));
        if (ATX_SUCCEEDED(result)) {
            result = ATX_File_GetOutputStream(file, &instance->stream);
            if (ATX_FAILED(result)) {
                instance->stream = NULL;
            }
        }
    
        ATX_DESTROY_OBJECT(file);
    }

    /* setup the interface */
    handler->instance = (ATX_LogHandlerInstance*)instance;
    handler->iface    = &ATX_LogFileHandler_Interface;

    /* cleanup */
    ATX_String_Destruct(&logger_prefix);
    ATX_String_Destruct(&filename_synth);

    return result;
}

/*----------------------------------------------------------------------
|   ATX_LogFileHandler_Interface
+---------------------------------------------------------------------*/
static const ATX_LogHandlerInterface 
ATX_LogFileHandler_Interface = {
    ATX_LogFileHandler_Log,
    ATX_LogFileHandler_Destroy
};

/*----------------------------------------------------------------------
|   ATX_LogTcpHandler forward references
+---------------------------------------------------------------------*/
static const ATX_LogHandlerInterface ATX_LogTcpHandler_Interface;

/*----------------------------------------------------------------------
|   ATX_LogTcpHandler_Connect
+---------------------------------------------------------------------*/
static ATX_Result
ATX_LogTcpHandler_Connect(ATX_LogTcpHandler* self)
{
    ATX_Result result = ATX_SUCCESS;

    /* create a socket */
    ATX_Socket* tcp_socket = NULL;
    ATX_CHECK(ATX_TcpClientSocket_Create(&tcp_socket));

    /* connect to the host */
    result = ATX_Socket_ConnectToHost(tcp_socket, ATX_CSTR(self->host), self->port, 
                                      ATX_LOG_TCP_HANDLER_DEFAULT_CONNECT_TIMEOUT);
    if (ATX_SUCCEEDED(result)) {
        /* get the stream */
        result = ATX_Socket_GetOutputStream(tcp_socket, &self->stream);
        if (ATX_FAILED(result)) self->stream = NULL;
    }

    /* cleanup */
    ATX_DESTROY_OBJECT(tcp_socket);

    return result;
}

/*----------------------------------------------------------------------
|   ATX_LogTcpHandler_FormatRecord
+---------------------------------------------------------------------*/
static void
ATX_LogTcpHandler_FormatRecord(const ATX_LogRecord* record, 
                               ATX_String*          msg,
                               ATX_UInt32           sequence_number)
{
    /* format the record */
    const char* level_name = ATX_Log_GetLogLevelName(record->level);
    char        level_string[16];
    char        buffer[64];

    /* format and emit the record */
    if (level_name[0] == '\0') {
        ATX_IntegerToString(record->level, level_string, sizeof(level_string));
        level_name = level_string;
    }
    ATX_String_Reserve(msg, 2048);
    ATX_String_Append(msg, "Logger: ");
    ATX_String_Append(msg, record->logger_name);
    ATX_String_Append(msg, "\r\nLevel: ");
    ATX_String_Append(msg, level_name);
    ATX_String_Append(msg, "\r\nSource-File: ");
    ATX_String_Append(msg, record->source_file);
    ATX_String_Append(msg, "\r\nSource-Function: ");
    ATX_String_Append(msg, record->source_function);
    ATX_String_Append(msg, "\r\nSource-Line: ");
    ATX_IntegerToStringU(record->source_line, buffer, sizeof(buffer));
    ATX_String_Append(msg, buffer);
    ATX_String_Append(msg, "\r\nTimeStamp: ");
    ATX_IntegerToStringU(record->timestamp.seconds, buffer, sizeof(buffer));
    ATX_String_Append(msg, buffer);
    ATX_String_Append(msg, ":");
    ATX_IntegerToStringU(record->timestamp.nanoseconds/1000000L, buffer, sizeof(buffer));
    ATX_String_Append(msg, buffer);
    ATX_String_Append(msg, "\r\nSequence-Number: ");
    ATX_IntegerToStringU(sequence_number, buffer, sizeof(buffer));
    ATX_String_Append(msg, buffer);
    ATX_String_Append(msg, "\r\nContent-Length: ");
    ATX_IntegerToString(ATX_StringLength(record->message), buffer, sizeof(buffer));
    ATX_String_Append(msg, buffer);    
    ATX_String_Append(msg, "\r\n\r\n");
    ATX_String_Append(msg, record->message);
}

/*----------------------------------------------------------------------
|   ATX_LogTcpHandler_Log
+---------------------------------------------------------------------*/
static void
ATX_LogTcpHandler_Log(ATX_LogHandler* _self, const ATX_LogRecord* record)
{
    ATX_LogTcpHandler* self = (ATX_LogTcpHandler*)_self->instance;
    ATX_String         msg  = ATX_EMPTY_STRING;
    
    /* ensure we're connected */
    if (self->stream == NULL) {
        if (ATX_FAILED(ATX_LogTcpHandler_Connect(self))) {
            return;
        }
    }

    /* format the record */
    ATX_LogTcpHandler_FormatRecord(record, &msg, self->sequence_number++);
    

    /* emit the formatted record */
    if (ATX_FAILED(ATX_OutputStream_WriteString(self->stream, ATX_CSTR(msg)))) {
        ATX_RELEASE_OBJECT(self->stream);
    }

    ATX_String_Destruct(&msg);
}

/*----------------------------------------------------------------------
|   ATX_LogTcpHandler_Destroy
+---------------------------------------------------------------------*/
static void
ATX_LogTcpHandler_Destroy(ATX_LogHandler* _self)
{
    ATX_LogTcpHandler* self = (ATX_LogTcpHandler*)_self->instance;

    /* destroy fields */
    ATX_String_Destruct(&self->host);

    /* release the stream */
    ATX_RELEASE_OBJECT(self->stream);

    /* free the object memory */
    ATX_FreeMemory((void*)self);
}

/*----------------------------------------------------------------------
|   ATX_LogTcpHandler_Create
+---------------------------------------------------------------------*/
static ATX_Result
ATX_LogTcpHandler_Create(const char* logger_name, ATX_LogHandler* handler)
{
    ATX_LogTcpHandler* instance;
    const ATX_String*  hostname;
    const ATX_String*  port;
    ATX_Result         result = ATX_SUCCESS;

    /* compute a prefix for the configuration of this handler */
    ATX_String logger_prefix = ATX_String_Create(logger_name);
    ATX_CHECK(ATX_String_Append(&logger_prefix, ".TcpHandler"));

    /* allocate a new object */
    instance = ATX_AllocateZeroMemory(sizeof(ATX_LogTcpHandler));

    /* configure the object */
    hostname = ATX_LogManager_GetConfigValue(ATX_CSTR(logger_prefix), ".hostname");
    if (hostname) {
        ATX_String_Assign(&instance->host, ATX_CSTR(*hostname));
    } else {
        /* default hostname */
        ATX_String_Assign(&instance->host, "localhost");
    }
    port = ATX_LogManager_GetConfigValue(ATX_CSTR(logger_prefix), ".port");
    if (port) {
        int port_int;
        if (ATX_SUCCEEDED(ATX_String_ToInteger(port, &port_int, ATX_TRUE))) {
            instance->port = (ATX_UInt16)port_int;
        } else {
            instance->port = ATX_LOG_TCP_HANDLER_DEFAULT_PORT;
        }
    } else {
        /* default port */
        instance->port = ATX_LOG_TCP_HANDLER_DEFAULT_PORT;
    }

    /* setup the interface */
    handler->instance = (ATX_LogHandlerInstance*)instance;
    handler->iface    = &ATX_LogTcpHandler_Interface;

    /* cleanup */
    ATX_String_Destruct(&logger_prefix);

    return result;
}

/*----------------------------------------------------------------------
|   ATX_LogTcpHandler_Interface
+---------------------------------------------------------------------*/
static const ATX_LogHandlerInterface 
ATX_LogTcpHandler_Interface = {
    ATX_LogTcpHandler_Log,
    ATX_LogTcpHandler_Destroy
};

/*----------------------------------------------------------------------
|   ATX_LogTUdpHandler forward references
+---------------------------------------------------------------------*/
static const ATX_LogHandlerInterface ATX_LogUdpHandler_Interface;

/*----------------------------------------------------------------------
|   ATX_LogUdpHandler_Log
+---------------------------------------------------------------------*/
static void
ATX_LogUdpHandler_Log(ATX_LogHandler* _self, const ATX_LogRecord* record)
{
    ATX_LogUdpHandler* self = (ATX_LogUdpHandler*)_self->instance;
    ATX_DataBuffer*    buffer;
    
    /* format the record */
    ATX_String msg = ATX_EMPTY_STRING;
    ATX_LogTcpHandler_FormatRecord(record, &msg, self->sequence_number++);

    /* send the record in a datagram */
    ATX_DataBuffer_Create(0, &buffer);
    ATX_DataBuffer_SetBuffer(buffer, (void*)ATX_String_UseChars(&msg), ATX_String_GetLength(&msg)+1);
    ATX_DataBuffer_SetDataSize(buffer, ATX_String_GetLength(&msg)+1);
    ATX_DatagramSocket_Send(self->socket, buffer, &self->address);
    
    /* cleanup */
    ATX_DataBuffer_Destroy(buffer);
    ATX_String_Destruct(&msg);
}

/*----------------------------------------------------------------------
|   ATX_LogUdpHandler_Destroy
+---------------------------------------------------------------------*/
static void
ATX_LogUdpHandler_Destroy(ATX_LogHandler* _self)
{
    ATX_LogUdpHandler* self = (ATX_LogUdpHandler*)_self->instance;

    /* destroy fields */
    ATX_DESTROY_OBJECT(self->socket);

    /* free the object memory */
    ATX_FreeMemory((void*)self);
}

/*----------------------------------------------------------------------
|   ATX_LogUdpHandler_Create
+---------------------------------------------------------------------*/
static ATX_Result
ATX_LogUdpHandler_Create(const char* logger_name, ATX_LogHandler* handler)
{
    ATX_LogUdpHandler* instance;
    const ATX_String*  hostname_prop;
    const char*        hostname = "localhost";
    const ATX_String*  port_prop;
    ATX_UInt16         port = ATX_LOG_UDP_HANDLER_DEFAULT_PORT;
    ATX_Result         result = ATX_SUCCESS;

    /* compute a prefix for the configuration of this handler */
    ATX_String logger_prefix = ATX_String_Create(logger_name);
    ATX_CHECK(ATX_String_Append(&logger_prefix, ".UdpHandler"));

    /* allocate a new object */
    instance = ATX_AllocateZeroMemory(sizeof(ATX_LogUdpHandler));

    /* setup the interface */
    handler->instance = (ATX_LogHandlerInstance*)instance;
    handler->iface    = &ATX_LogUdpHandler_Interface;

    /* construct fields */
    result = ATX_UdpSocket_Create(&instance->socket);
    if (ATX_FAILED(result)) {
        ATX_String_Destruct(&logger_prefix);
        ATX_FreeMemory(instance);
        return result;
    }
    
    /* configure the object */
    hostname_prop = ATX_LogManager_GetConfigValue(ATX_CSTR(logger_prefix), ".hostname");
    if (hostname_prop) {
        hostname = ATX_CSTR(*hostname_prop);
    }
    port_prop = ATX_LogManager_GetConfigValue(ATX_CSTR(logger_prefix), ".port");
    if (port_prop) {
        int port_int;
        if (ATX_SUCCEEDED(ATX_String_ToInteger(port_prop, &port_int, ATX_TRUE))) {
            port = (ATX_UInt16)port_int;
        }
    }
    
    /* resolve the name */
    result = ATX_IpAddress_ResolveName(&instance->address.ip_address, hostname, ATX_LOG_UDP_HANDLER_DEFAULT_RESOLVER_TIMEOUT);
    if (ATX_FAILED(result)) {
        ATX_String_Destruct(&logger_prefix);
        ATX_LogUdpHandler_Destroy(handler);
        return result;
    }
    instance->address.port = port;
    
    /* cleanup */
    ATX_String_Destruct(&logger_prefix);

    return result;
}

/*----------------------------------------------------------------------
|   ATX_LogUdpHandler_Interface
+---------------------------------------------------------------------*/
static const ATX_LogHandlerInterface 
ATX_LogUdpHandler_Interface = {
    ATX_LogUdpHandler_Log,
    ATX_LogUdpHandler_Destroy
};
