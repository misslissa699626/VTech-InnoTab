/*****************************************************************
|
|   BlueTune - Stream Controller
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "Neptune.h"
#include "BlueTune.h"
#include "BtStreamController.h"

/*----------------------------------------------------------------------
|    BtStreamController::BtStreamController
+---------------------------------------------------------------------*/
BtStreamController::BtStreamController(NPT_InputStreamReference& input,
                                       BLT_Player&               player) :
    m_InputStream(input),
    m_Player(player)
{
}

/*----------------------------------------------------------------------
|    BtStreamController::DoSeekToTimeStamp
|
|    Parse a timecode of the form: {hh:}{mm:}{ss}{.ff}
|
+---------------------------------------------------------------------*/
void
BtStreamController::DoSeekToTimeStamp(const char* time)
{
    BLT_UInt8    val[4] = {0,0,0,0};
    ATX_Size     length = ATX_StringLength(time);
    unsigned int val_c = 0;
    bool         has_dot = false;
    
    if (length != 11 && length != 8 && length != 5 && length != 2) return;
    
    do {
        if ( time[0] >= '0' && time[0] <= '9' && 
             time[1] >= '0' && time[0] <= '9' &&
            (time[2] == ':' || time[2] == '.' || time[2] == '\0')) {
            if (time[2] == '.') {
                if (length != 5) return; // dots only on the last part
                has_dot = true;
            } else {
                if (val_c == 3) return; // too many parts
            }
            val[val_c++] = (time[0]-'0')*10 + (time[1]-'0');
            length -= (time[2]=='\0')?2:3;
            time += 3;
        } else {
            return;
        }
    } while (length >= 2);
    
    BLT_UInt8 h,m,s,f;
    if (has_dot) --val_c;    
    h = val[(val_c+1)%4];
    m = val[(val_c+2)%4];
    s = val[(val_c+3)%4];
    f = val[(val_c  )%4];

    m_Player.SeekToTimeStamp(h,m,s,f);
}

/*----------------------------------------------------------------------
|   BtStreamController::DoSetProperty
|
|   The command syntax is:
|   <scope>,<name>=<type>:<value>
|   where scope can be:
|       core
|    or
|       stream
|   an type can be:
|     s --> value is a string 
|     i --> value is an integer
|
+---------------------------------------------------------------------*/
void
BtStreamController::DoSetProperty(const char* property_spec)
{
    NPT_String        spec(property_spec);
    BLT_PropertyScope property_scope;
    
    int sc = spec.Find(',');
    if (sc < 1) return;
    NPT_String scope(property_spec, sc);
    if (scope == "core") {
        property_scope = BLT_PROPERTY_SCOPE_CORE;
    } else if (scope == "stream") {
        property_scope = BLT_PROPERTY_SCOPE_STREAM;
    } else {
        return;
    }
    
    int eq = spec.Find('=', sc);
    if (eq < 1) return;
    NPT_String property_name(property_spec+sc+1, eq-sc-1);
    
    char property_type = property_spec[eq+1];
    if (property_type == '\0') return;
    if (property_spec[eq+2] != ':') return;
    
    ATX_PropertyValue property_value;
    if (property_type == 's') {
        property_value.type = ATX_PROPERTY_VALUE_TYPE_STRING;
        property_value.data.string = property_spec+eq+3;
    } else if (property_type == 'i') {
        NPT_Int32 value;
        if (NPT_FAILED(NPT_ParseInteger32(property_spec+eq+3, value))) {
            return;
        }
        property_value.type = ATX_PROPERTY_VALUE_TYPE_INTEGER;
        property_value.data.integer = value;
    }
    
    m_Player.SetProperty(property_scope, NULL, property_name.GetChars(), &property_value);
}

/*----------------------------------------------------------------------
|   BtStreamController::DoLoadPlugin
|
|   The command syntax is:
|   <name>[,<search-flags>]
|
+---------------------------------------------------------------------*/
void
BtStreamController::DoLoadPlugin(const char* property_spec)
{
    NPT_String spec(property_spec);
    
    BLT_Flags search_flags = 0;
    int sc = spec.Find(',');
    if (sc > 0) {
        // flags
        NPT_UInt32 value = 0;
        NPT_ParseInteger32(spec.GetChars()+sc+1, value);
        search_flags = (BLT_Flags)value;
        spec.SetLength(sc);
    }
    
    m_Player.LoadPlugin(spec.GetChars(), search_flags);
}

/*----------------------------------------------------------------------
|   BtStreamController::DoLoadPlugins
|
|   The command syntax is:
|   <directory>[,<file-extension>]
|
+---------------------------------------------------------------------*/
void
BtStreamController::DoLoadPlugins(const char* property_spec)
{
    NPT_String spec(property_spec);
    NPT_String directory  = spec;
    NPT_String file_extension;

    int sc = spec.Find(',');
    if (sc > 0) {
        // file extension
        directory.SetLength(sc);
        file_extension = spec.SubString(sc+1);
    }
    
    m_Player.LoadPlugins(directory.GetChars(), file_extension.GetChars());
}

/*----------------------------------------------------------------------
|    BtStreamController::Run
+---------------------------------------------------------------------*/
void
BtStreamController::Run()
{
    char       buffer[1024];
    bool       done = false;
    BLT_Result result;

    // get the command stream
    NPT_BufferedInputStream input(m_InputStream, 0);

    do {
        NPT_Size bytes_read;
        result = input.ReadLine(buffer, 
                                sizeof(buffer), 
                                &bytes_read);
        if (NPT_SUCCEEDED(result)) {
            if (NPT_StringsEqualN(buffer, "set-input ", 10)) {
                m_Player.SetInput(&buffer[10]);
            } else if (NPT_StringsEqualN(buffer, "set-output ", 11)) {
                m_Player.SetOutput(&buffer[11]);
            } else if (NPT_StringsEqualN(buffer, "add-node ", 9)) {
                m_Player.AddNode(&buffer[9]);
            } else if (NPT_StringsEqual(buffer, "play")) {
                m_Player.Play();
            } else if (NPT_StringsEqual(buffer, "stop")) {
                m_Player.Stop();
            } else if (NPT_StringsEqual(buffer, "pause")) {
                m_Player.Pause();
            } else if (NPT_StringsEqualN(buffer, "seek ", 5)) {
                DoSeekToTimeStamp(buffer+5);
            } else if (NPT_StringsEqualN(buffer, "set-property ", 13)) {
                DoSetProperty(buffer+13);
            } else if (NPT_StringsEqualN(buffer, "load-plugin ", 12)) {
                DoLoadPlugin(buffer+12);
            } else if (NPT_StringsEqualN(buffer, "load-plugins ", 13)) {
                DoLoadPlugins(buffer+13);
            } else if (NPT_StringsEqualN(buffer, "exit", 4)) {
                done = BLT_TRUE;
            } else {
                ATX_Debug("ERROR: invalid command\n");
            }
        } else {
            ATX_Debug("end: %d\n", result);
        }
    } while (BLT_SUCCEEDED(result) && !done);

    // interrupt ourselves so that we can exit our message pump loop
    m_Player.Interrupt();
}

