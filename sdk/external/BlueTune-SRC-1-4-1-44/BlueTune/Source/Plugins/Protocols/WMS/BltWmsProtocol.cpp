/*****************************************************************
|
|   Windows Media Services Protocol Module
|
|   (c) 2002-2010 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltConfig.h"
#include "BltWmsProtocol.h"
#include "BltCore.h"
#include "BltMediaNode.h"
#include "BltMedia.h"
#include "BltPcm.h"
#include "BltPacketProducer.h"
#include "BltByteStreamUser.h"
#include "BltStream.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.protocols.wms")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int BLT_WMS_ASF_MAX_STREAMS = 128;

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
class WmsAsxPlaylist {
public:
    struct Ref {
        NPT_String m_Url;
    };
    struct Entry {
        NPT_String    m_Title;
        NPT_List<Ref> m_Refs;
    };
  
    static BLT_Result Parse(const char*      asx_data, 
                            ATX_Size         asx_data_size, 
                            WmsAsxPlaylist*& playlist);
    static BLT_Result ParseAsXml(const char*      asx_data, 
                                 ATX_Size         asx_data_size, 
                                 WmsAsxPlaylist*& playlist);
    static BLT_Result ParseAsText(const char*      asx_data, 
                                  ATX_Size         asx_data_size, 
                                  WmsAsxPlaylist*& playlist);
    
    // members
    NPT_String      m_Version;
    NPT_String      m_Title;
    NPT_List<Entry> m_Entries;
};

struct WmsAsfPacket {
    WmsAsfPacket(unsigned int         packet_id,
                 unsigned int         packet_length,
                 const unsigned char* packet_data) :
        m_Id(packet_id),
        m_Length(packet_length),
        m_Data(packet_data) {}
        
    unsigned int         m_Id;
    unsigned int         m_Length;
    const unsigned char* m_Data;
};

class WmsClient {
public:
    struct Stream {
        typedef enum {
            STREAM_TYPE_NONE,
            STREAM_TYPE_OTHER,
            STREAM_TYPE_AUDIO,
            STREAM_TYPE_VIDEO
        } StreamType;
        Stream() : m_Type(STREAM_TYPE_NONE), m_Selected(false), m_Encrypted(false), m_AverageBitrate(0), m_HasExtendedInfo(false), m_Flags(0), m_DataRate(0), m_LanguageIdIndex(0) {}
        StreamType m_Type;
        bool       m_Selected;
        bool       m_Encrypted;
        NPT_UInt32 m_AverageBitrate;
        bool       m_HasExtendedInfo; // for the fields below
        NPT_UInt32 m_Flags;
        NPT_UInt32 m_DataRate;
        NPT_UInt16 m_LanguageIdIndex;
    };
    WmsClient(const char* url);
   ~WmsClient();
    BLT_Result Describe();
    BLT_Result Select();
    BLT_Result Play();
    
    // accessors
    NPT_Ordinal GetSelectedStream() const { return m_SelectedStream; }
    
    // members
    NPT_HttpResponse* m_Response;
    
private:
    // methods
    BLT_Result ParseAsf(const unsigned char* asf_data, NPT_UInt64 asf_data_size);
    BLT_Result ParseAsfHeader(const unsigned char* asf_data, NPT_UInt64 asf_data_size);
    BLT_Result ParseAsfHeaderExtension(const unsigned char* asf_data, NPT_UInt64 asf_data_size);
    BLT_Result ParseAsfFileProperties(const unsigned char* asf_data, NPT_UInt64 asf_data_size);
    BLT_Result ParseAsfStreamProperties(const unsigned char* asf_data, NPT_UInt64 asf_data_size);
    BLT_Result ParseAsfExtendedStreamProperties(const unsigned char* asf_data, NPT_UInt64 asf_data_size);
    BLT_Result ParseAsfStreamBitrateProperties(const unsigned char* asf_data, NPT_UInt64 asf_data_size);

    // members
    NPT_String    m_Url;
    NPT_String    m_Guid;
    NPT_String    m_UserAgent;
    NPT_UInt32    m_RequestContext;
    NPT_LargeSize m_ContentLength;
    Stream        m_Streams[BLT_WMS_ASF_MAX_STREAMS];
    NPT_Ordinal   m_SelectedStream;
    struct {
        unsigned char m_Id[16];
        NPT_UInt64    m_Size;
        NPT_UInt64    m_PacketCount;
        NPT_UInt32    m_Flags;
        NPT_UInt32    m_MinPacketSize;
        NPT_UInt32    m_MaxPacketSize;
        NPT_UInt32    m_MaxBitrate;
    } m_File;
};

struct WmsProtocolModule {
    /* base class */
    ATX_EXTENDS(BLT_BaseModule);

    /* members */
    BLT_UInt32 asf_audio_type_id;
    BLT_UInt32 asf_video_type_id;
    BLT_UInt32 asf_application_type_id;
    BLT_UInt32 mms_framed_type_id;
};

struct WmsProtocolInput {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_InputStreamUser);

    /* members */
    BLT_MediaType asf_audio_media_type;
    BLT_MediaType asf_video_media_type;
    BLT_MediaType asf_application_media_type;
};

struct WmsProtocolOutput {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_MediaPort);
    ATX_IMPLEMENTS(BLT_PacketProducer);

    /* members */
    BLT_Size         size;
    BLT_MediaType    media_type;
    BLT_MediaPacket* packet;
};

struct WmsProtocol {
    /* base class */
    ATX_EXTENDS(BLT_BaseMediaNode);

    /* members */
    WmsProtocolInput  input;
    WmsProtocolOutput output;
    WmsAsxPlaylist*   playlist;
    WmsClient*        client;
};

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int BLT_WMS_PROTOCOL_MAX_ASX_SIZE              = (128*1024);  // max 128k 
const unsigned int BLT_WMS_PROTOCOL_MAX_DESCRIBE_PAYLOAD_SIZE = (1024*1024); // max 1M

const unsigned int BLT_WMS_PACKET_ID_HEADER        = 0x48;
const unsigned int BLT_WMS_PACKET_ID_DATA          = 0x44;
const unsigned int BLT_WMS_PACKET_ID_EOS           = 0x45;
const unsigned int BLT_WMS_PACKET_ID_STREAM_CHANGE = 0x43;

const unsigned int BLT_WMS_ASF_FILE_PROPERTY_FLAG_BROADCAST = 1;
const unsigned int BLT_WMS_ASF_FILE_PROPERTY_FLAG_SEEKABLE  = 2;

const unsigned char BLT_WMS_ASF_GUID_HEADER[16] = 
    // 75B22630-668E-11CF-A6D9-00AA0062CE6C
    { 0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C}; 

const unsigned char BLT_WMS_ASF_GUID_DATA[16] = 
    // 75B22636-668E-11CF-A6D9-00AA0062CE6C
    { 0x36, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C}; 

const unsigned char BLT_WMS_ASF_GUID_HEADER_EXTENSION[16] = 
    // 5FBF03B5-A92E-11CF-8EE3-00C00C205365
    { 0xB5, 0x03, 0xBF, 0x5F, 0x2E, 0xA9, 0xCF, 0x11, 0x8E, 0xE3, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65};

const unsigned char BLT_WMS_ASF_GUID_FILE_PROPERTIES[16] = 
    // 8CABDCA1-A947-11CF-8EE4-00C00C205365
    { 0xA1, 0xDC, 0xAB, 0x8C, 0x47, 0xA9, 0xCF, 0x11, 0x8E, 0xE4, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65};

const unsigned char BLT_WMS_ASF_GUID_STREAM_PROPERTIES[16] = 
    // B7DC0791-A9B7-11CF-8EE6-00C00C205365
    { 0x91, 0x07, 0xDC, 0xB7, 0xB7, 0xA9, 0xCF, 0x11, 0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65};
    
const unsigned char BLT_WMS_ASF_GUID_EXTENDED_STREAM_PROPERTIES[16] = 
    // 14E6A5CB-C672-4332-8399-A96952065B5A
    { 0xCB, 0xA5, 0xE6, 0x14, 0x72, 0xC6, 0x32, 0x43, 0x83, 0x99, 0xA9, 0x69, 0x52, 0x06, 0x5B, 0x5A};

const unsigned char BLT_WMS_ASF_GUID_STREAM_BITRATE_PROPERTIES[16] = 
    // 7BF875CE-468D-11D1-8D82-006097C9A2B2
    { 0xCE, 0x75, 0xF8, 0x7B, 0x8D, 0x46, 0xD1, 0x11, 0x8D, 0x82, 0x00, 0x60, 0x97, 0xC9, 0xA2, 0xB2};
    
const unsigned char BLT_WMS_ASF_GUID_AUDIO_MEDIA[16] = 
    // F8699E40-5B4D-11CF-A8FD-00805F5C442B
    { 0x40, 0x9E, 0x69, 0xF8, 0x4D, 0x5B, 0xCF, 0x11, 0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B};

/*----------------------------------------------------------------------
|   WmsAsxPlaylist::ParseAsXml
+---------------------------------------------------------------------*/
BLT_Result
WmsAsxPlaylist::ParseAsXml(const char*      asx_data, 
                           ATX_Size         asx_data_size, 
                           WmsAsxPlaylist*& playlist)
{
    NPT_XmlParser parser;
    NPT_XmlNode*  asx_root = NULL;
    NPT_Result result = parser.Parse(asx_data, asx_data_size, asx_root);
    if (NPT_FAILED(result) || asx_root == NULL) {
        if (result == NPT_ERROR_XML_TAG_MISMATCH) {
            // try converting all known tags to lowercase and parse again
            NPT_String asx_data_string(asx_data, asx_data_size);
            bool in_tag = false;
            for (char* cursor = asx_data_string.UseChars(); *cursor; cursor++) {
                char c = *cursor;
                if (in_tag) {
                    if (c == '/' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                        if (c >= 'a' && c <= 'z') *cursor = c-0x20;
                    } else {
                        in_tag = false;
                    }
                } else {
                    if (c == '<') {
                        in_tag = true;
                    }
                }
            }
            result = parser.Parse(asx_data_string.GetChars(), asx_data_string.GetLength(), asx_root);
        }
        if (NPT_FAILED(result) || asx_root == NULL) {
            ATX_LOG_WARNING_1("cannot parse ASX playlist (%d)", result);
            return BLT_ERROR_INVALID_MEDIA_FORMAT;
        }
    }
    NPT_XmlElementNode* asx_root_element = asx_root->AsElementNode();
    if (asx_root_element == NULL) {
        ATX_LOG_WARNING("cannot parse ASX playlist: top node is not an element");
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }
    if (asx_root_element->GetTag().Compare("ASX", true)) {
        ATX_LOG_WARNING("ASX Playlist does not start with <ASX> element");
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }
    
    // instantiate a new object
    playlist = new WmsAsxPlaylist();
    
    // initialize the object
    for (NPT_List<NPT_XmlNode*>::Iterator i = asx_root_element->GetChildren().GetFirstItem();
                                          i;
                                        ++i) {
        NPT_XmlElementNode* child = (*i)->AsElementNode();
        if (child == NULL) continue;
        if (child->GetTag().Compare("TITLE", true) == 0) {
            if (child->GetText()) playlist->m_Title = *child->GetText();
            ATX_LOG_FINER_1("ASX TITLE = %s", playlist->m_Title.GetChars());
        } else if (child->GetTag().Compare("ENTRY", true) == 0) {
            playlist->m_Entries.Add(WmsAsxPlaylist::Entry());
            WmsAsxPlaylist::Entry& asx_entry = *(playlist->m_Entries.GetLastItem());
            ATX_LOG_FINER("new ASX ENTRY");
            for (NPT_List<NPT_XmlNode*>::Iterator j = child->GetChildren().GetFirstItem();
                                                  j;
                                                ++j) {
                child = (*j)->AsElementNode();
                if (child == NULL) continue;
                if (child->GetTag().Compare("REF", true) == 0) {
                    asx_entry.m_Refs.Add(WmsAsxPlaylist::Ref());
                    WmsAsxPlaylist::Ref& asx_ref = *(asx_entry.m_Refs.GetLastItem());
                    ATX_LOG_FINER("new ASX ENTRY REF");
                    for (NPT_List<NPT_XmlAttribute*>::Iterator attr = child->GetAttributes().GetFirstItem();
                                                               attr;
                                                             ++attr) {
                        if ((*attr) == NULL) continue;
                        if ((*attr)->GetName().Compare("HREF", true) == 0) {
                            asx_ref.m_Url = (*attr)->GetValue();
                            ATX_LOG_FINER_1("ASX ENTRY REF: HREF=%s", asx_ref.m_Url.GetChars());
                        }
                    }
                }
            }
        }
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WmsAsxPlaylist::ParseAsText
+---------------------------------------------------------------------*/
BLT_Result
WmsAsxPlaylist::ParseAsText(const char*      asx_data, 
                            ATX_Size         asx_data_size, 
                            WmsAsxPlaylist*& playlist)
{
    NPT_String asx_data_string(asx_data, asx_data_size);
    const char* line_start = asx_data_string.GetChars();

    // instantiate a new object
    playlist = new WmsAsxPlaylist();
    playlist->m_Entries.Add(WmsAsxPlaylist::Entry());
    WmsAsxPlaylist::Entry& asx_entry = *(playlist->m_Entries.GetLastItem());
    ATX_LOG_FINER("new ASX ENTRY");

    for (const char* cursor = asx_data_string.GetChars();; ++cursor) {
        if (*cursor == '\r' || *cursor == '\n' || *cursor == '\0') {
            /* process the line */
            if (cursor != line_start) {
                NPT_String line(line_start, (NPT_Size)(cursor-line_start));
                if (line.StartsWith("Ref", true)) {
                    int sep = line.Find('=', 3);
                    if (sep > 3) {
                        ++sep;
                        while (line.GetChars()[sep] == ' ') sep++; // skip whitespace
                        if (line.GetChars()[sep]) {
                            ATX_LOG_FINER_1("new ASX ENTRY REF: %s", line.GetChars()+sep);
                            asx_entry.m_Refs.Add(WmsAsxPlaylist::Ref());
                            WmsAsxPlaylist::Ref& asx_ref = *(asx_entry.m_Refs.GetLastItem());
                            asx_ref.m_Url = line.GetChars()+sep;
                        }
                    }
                }
            }
            
            /* move on to the next line */
            if (*cursor == '\0') break;
            line_start = cursor+1;
        } else {
            if (*line_start == '\r' ||
                *line_start == '\n' ||
                *line_start == ' ') {
                // skip empty lines or leading/trailing whitespace
                line_start = cursor;
            }
        }
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WmsAsxPlaylist::Parse
+---------------------------------------------------------------------*/
BLT_Result
WmsAsxPlaylist::Parse(const char*      asx_data, 
                      ATX_Size         asx_data_size, 
                      WmsAsxPlaylist*& playlist)
{
    // check if this is really XML-based ASX or a text replacement for it
    if (asx_data_size >= 11) {
        NPT_String head;
        head.Assign(asx_data, 11);
        if (head.Compare("[Reference]", true) == 0) {
            return ParseAsText(asx_data, asx_data_size, playlist);
        }
    }
    
    // default to XML
    return ParseAsXml(asx_data, asx_data_size, playlist);
}

/*----------------------------------------------------------------------
|   WmsAsf_ParsePackets
+---------------------------------------------------------------------*/
static BLT_Result
WmsAsf_ParsePackets(const unsigned char*    data, 
                    unsigned int            data_size,
                    NPT_List<WmsAsfPacket>& packets)
{
    while (data_size) {
        // check that we have enough for a framing header
        if (data_size < 4) return BLT_ERROR_INVALID_MEDIA_FORMAT;
        
        // check the framing byte
        if ((data[0] & 0x7F) != 0x24) {
            ATX_LOG_WARNING_1("invalid framing byte %x", data[0]);
            return BLT_ERROR_INVALID_MEDIA_FORMAT;
        }
        ++data;
        --data_size;
        
        // get the packet_id
        unsigned int packet_id = *data++; --data_size;
        
        // get the packet length
        unsigned int packet_length = NPT_BytesToInt16Le(data);
        data += 2;
        data_size -= 2;
        
        // check the packet size
        if (packet_length > data_size) {
            ATX_LOG_WARNING("packet size larger than available buffer");
            return BLT_ERROR_INVALID_MEDIA_FORMAT;
        }
        
        // add the packet to the list
        packets.Add(WmsAsfPacket(packet_id, packet_length, data));
        
        // move to the next packet
        data      += packet_length;
        data_size -= packet_length;
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WmsAsf_GuidsEqual
+---------------------------------------------------------------------*/
static bool
WmsAsf_GuidsEqual(const unsigned char* guid_1, const unsigned char* guid_2)
{
    return NPT_MemoryEqual((const void*)guid_1, (const void*)guid_2, 16);
}

/*----------------------------------------------------------------------
|   WmsAsf_GetNextObject
+---------------------------------------------------------------------*/
BLT_Result
WmsAsf_GetNextObject(const unsigned char*  data,
                     NPT_UInt64&           data_size,
                     const unsigned char*& guid,
                     NPT_UInt64&           payload_size)
{
    // default values
    guid = NULL;
    payload_size = 0;
    
    // check that we have enough for the object fields
    if (data_size < 16+8) {
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }
    payload_size = NPT_BytesToInt64Le(data+16);
    if (payload_size > data_size) {
        if (WmsAsf_GuidsEqual(data, BLT_WMS_ASF_GUID_DATA) && data_size >= 50) {
            // special case for data objects for which we only have the header
            guid = data;
            payload_size = 0;
            data_size = 0;
        } else {
            payload_size = 0;
            return BLT_ERROR_INVALID_MEDIA_FORMAT;
        }
    } else if (payload_size == 0) {
        // unspecified
        payload_size = data_size-(16+8);
        guid = data;
        data_size = 0;
    } else {
        guid = data;
        data_size -= payload_size;
        payload_size -= (16+8);
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WmsClient::WmsClient
+---------------------------------------------------------------------*/
WmsClient::WmsClient(const char* url) :
    m_Response(NULL),
    m_Url(url),
    m_UserAgent("NSPlayer/7.10.0.3059"),
    m_RequestContext(0),
    m_ContentLength(0),
    m_SelectedStream(0)
{
    NPT_TimeStamp now;
    NPT_System::GetCurrentTimeStamp(now);
    m_Guid = NPT_String::Format("5a434f41-9dc0-41d1-b245-5ffd%08x", now.ToNanos());
    NPT_SetMemory(&m_File, 0, sizeof(m_File));
}

/*----------------------------------------------------------------------
|   WmsClient::~WmsClient
+---------------------------------------------------------------------*/
WmsClient::~WmsClient()
{
    delete m_Response;
}

/*----------------------------------------------------------------------
|   WmsClient::ParseAsf
+---------------------------------------------------------------------*/
BLT_Result 
WmsClient::ParseAsf(const unsigned char* asf_data, NPT_UInt64 asf_data_size)
{
    NPT_Result result;
    
    while (asf_data_size) {
        NPT_UInt64           object_payload_size = 0;
        const unsigned char* object_guid = NULL;
        result = WmsAsf_GetNextObject(asf_data, asf_data_size, object_guid, object_payload_size);
        if (BLT_FAILED(result)) {
            ATX_LOG_WARNING("error while parsing ASF object");
            return result;
        }
        
        // dispatch to the appropriate object handler
        if (WmsAsf_GuidsEqual(object_guid, BLT_WMS_ASF_GUID_HEADER)) {
            result = ParseAsfHeader(asf_data+16+8, object_payload_size);
        } else if (WmsAsf_GuidsEqual(object_guid, BLT_WMS_ASF_GUID_HEADER_EXTENSION)) {
            result = ParseAsfHeaderExtension(asf_data+16+8, object_payload_size);
        } else if (WmsAsf_GuidsEqual(object_guid, BLT_WMS_ASF_GUID_FILE_PROPERTIES)) {
            result = ParseAsfFileProperties(asf_data+16+8, object_payload_size);
        } else if (WmsAsf_GuidsEqual(object_guid, BLT_WMS_ASF_GUID_STREAM_PROPERTIES)) {
            result = ParseAsfStreamProperties(asf_data+16+8, object_payload_size);
        } else if (WmsAsf_GuidsEqual(object_guid, BLT_WMS_ASF_GUID_EXTENDED_STREAM_PROPERTIES)) {
            result = ParseAsfExtendedStreamProperties(asf_data+16+8, object_payload_size);
        } else if (WmsAsf_GuidsEqual(object_guid, BLT_WMS_ASF_GUID_STREAM_BITRATE_PROPERTIES)) {
            result = ParseAsfStreamBitrateProperties(asf_data+16+8, object_payload_size);
        }
        if (BLT_FAILED(result)) return result;
        
        // move on to the next object
        asf_data += 16+8+object_payload_size;
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WmsClient::ParseAsfHeader
+---------------------------------------------------------------------*/
BLT_Result 
WmsClient::ParseAsfHeader(const unsigned char* asf_data, NPT_UInt64 asf_data_size)
{
    // skip the fields that we ignore and parse the contained objects
    if (asf_data_size < 6) {
        ATX_LOG_WARNING("ASF header too small");
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }
    return ParseAsf(asf_data+6, asf_data_size-6);
}

/*----------------------------------------------------------------------
|   WmsClient::ParseAsfHeaderExtension
+---------------------------------------------------------------------*/
BLT_Result 
WmsClient::ParseAsfHeaderExtension(const unsigned char* asf_data, NPT_UInt64 asf_data_size)
{
    // skip the fields that we ignore and parse the contained objects
    if (asf_data_size < 22) {
        ATX_LOG_WARNING("ASF header extension too small");
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }
    return ParseAsf(asf_data+22, asf_data_size-22);
}

/*----------------------------------------------------------------------
|   WmsClient::ParseAsfFileProperties
+---------------------------------------------------------------------*/
BLT_Result 
WmsClient::ParseAsfFileProperties(const unsigned char* asf_data, NPT_UInt64 asf_data_size)
{
    if (asf_data_size < 80) {
        ATX_LOG_WARNING("ASF file properties too small");
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }
    NPT_CopyMemory(m_File.m_Id, asf_data, 16);             asf_data += 16;
    m_File.m_Size          = NPT_BytesToInt64Le(asf_data); asf_data += 8;
    /* skip creation date */                               asf_data += 8;
    m_File.m_PacketCount   = NPT_BytesToInt64Le(asf_data); asf_data += 8;
    /* skip play duration, send duration and preroll */    asf_data += (8+8+8);
    m_File.m_Flags         = NPT_BytesToInt32Le(asf_data); asf_data += 4;
    m_File.m_MinPacketSize = NPT_BytesToInt32Le(asf_data); asf_data += 4;
    m_File.m_MaxPacketSize = NPT_BytesToInt32Le(asf_data); asf_data += 4;
    m_File.m_MaxBitrate    = NPT_BytesToInt32Le(asf_data); asf_data += 4;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WmsClient::ParseAsfStreamProperties
+---------------------------------------------------------------------*/
BLT_Result 
WmsClient::ParseAsfStreamProperties(const unsigned char* asf_data, NPT_UInt64 asf_data_size)
{
    if (asf_data_size < 54) {
        ATX_LOG_WARNING("ASF stream properties too small");
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }
    WmsClient::Stream::StreamType stream_type = WmsClient::Stream::STREAM_TYPE_OTHER;
    if (WmsAsf_GuidsEqual(asf_data, BLT_WMS_ASF_GUID_AUDIO_MEDIA)) {
        stream_type = WmsClient::Stream::STREAM_TYPE_AUDIO;
    }
    asf_data += 16+16+8+4+4;
    NPT_UInt8 stream_number = asf_data[0]&0x7F;
    bool      is_encrypted  = (asf_data[1]&0x80)!=0;
    m_Streams[stream_number].m_Type      = stream_type;
    m_Streams[stream_number].m_Encrypted = is_encrypted;
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WmsClient::ParseAsfExtendedStreamProperties
+---------------------------------------------------------------------*/
BLT_Result 
WmsClient::ParseAsfExtendedStreamProperties(const unsigned char* asf_data, NPT_UInt64 asf_data_size)
{
    if (asf_data_size < 64) {
        ATX_LOG_WARNING("ASF extended stream properties too small");
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }
    asf_data += 2*8;
    NPT_UInt32 data_rate               = NPT_BytesToInt32Le(asf_data); asf_data += 7*4;
    NPT_UInt32 flags                   = NPT_BytesToInt32Le(asf_data); asf_data += 4;
    NPT_UInt16 stream_number           = NPT_BytesToInt16Le(asf_data); asf_data += 2;
    NPT_UInt16 language_id_index       = NPT_BytesToInt16Le(asf_data); asf_data += 2;
                                                                       asf_data += 8;
    NPT_UInt16 stream_name_count       = NPT_BytesToInt16Le(asf_data); asf_data += 2;
    NPT_UInt16 payload_extension_count = NPT_BytesToInt16Le(asf_data); asf_data += 2;

    if (stream_number >= BLT_WMS_ASF_MAX_STREAMS) return BLT_SUCCESS; // ignore
    WmsClient::Stream& stream = m_Streams[stream_number];
    stream.m_HasExtendedInfo = true;
    stream.m_DataRate        = data_rate;
    stream.m_Flags           = flags;
    stream.m_LanguageIdIndex = language_id_index;
    
    asf_data_size -= (2*8+8*4+2*2+8+2*2);
    for (unsigned int i=0; i<(unsigned int)(stream_name_count+payload_extension_count) && asf_data_size >= 16+8; i++) {
        NPT_UInt64 object_size = NPT_BytesToInt64Le(asf_data+16);
        if (object_size > asf_data_size-(16+8)) return BLT_SUCCESS; // ignore
        asf_data      += (object_size+16+8);
        asf_data_size -= (object_size+16+8);
    }

    return ParseAsf(asf_data, asf_data_size);
}

/*----------------------------------------------------------------------
|   WmsClient::ParseAsfStreamBitrateProperties
+---------------------------------------------------------------------*/
BLT_Result 
WmsClient::ParseAsfStreamBitrateProperties(const unsigned char* asf_data, NPT_UInt64 asf_data_size)
{
    if (asf_data_size < 2) {
        ATX_LOG_WARNING("ASF stream bitrate properties too small");
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }
    NPT_UInt16 record_count = NPT_BytesToInt16Le(asf_data);
    asf_data += 2;
    if (asf_data_size >= (unsigned int)(2+record_count*6)) {
        for (unsigned int i=0; i<record_count; i++) {
            NPT_UInt8 stream_number = asf_data[0]&0x7F;
            m_Streams[stream_number].m_AverageBitrate = NPT_BytesToInt32Le(asf_data+2);
            asf_data += 6;
        }
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WmsClient::Describe
+---------------------------------------------------------------------*/
BLT_Result
WmsClient::Describe()
{
    NPT_HttpClient    client;
    NPT_HttpResponse* response = NULL;
    NPT_HttpRequest   request(m_Url, NPT_HTTP_METHOD_GET);
    
    // setup the request headers
    NPT_String guid_header = "xClientGUID={";
    guid_header += m_Guid;
    guid_header += "}";
    NPT_String context_header = NPT_String::Format("no-cache,rate=1.000000,stream-time=0,stream-offset=0:0,request-context=%d,max-duration=0",
                                                   m_RequestContext++);
    NPT_HttpHeaders& headers = request.GetHeaders();
    headers.SetHeader(NPT_HTTP_HEADER_USER_AGENT, m_UserAgent);
    headers.SetHeader("Accept", "*/*");
    headers.AddHeader("Pragma", context_header);
    headers.AddHeader("Pragma", guid_header);
    
    NPT_Result result = client.SendRequest(request, response);
    if (NPT_FAILED(result)) return result;

    switch (response->GetStatusCode()) {
        case 200:
            break;

        case 403:
            return ATX_ERROR_ACCESS_DENIED;
            
        case 404:
            return BLT_ERROR_STREAM_INPUT_NOT_FOUND;

        default:
            return BLT_ERROR_PROTOCOL_FAILURE;
    }

    // analyze the response type
    if (response->GetEntity()->GetContentType() != "application/vnd.ms.wms-hdr.asfv1") {
        ATX_LOG_WARNING_1("unexpected Describe response type: %s", 
                          response->GetEntity()->GetContentType().GetChars());
        delete response;
        return BLT_ERROR_PROTOCOL_FAILURE;
    }
    
    // get the body stream and size
    if (response->GetEntity()->GetContentLength() == 0 ||
        response->GetEntity()->GetContentLength() > BLT_WMS_PROTOCOL_MAX_DESCRIBE_PAYLOAD_SIZE) {
        delete response;
        return BLT_ERROR_PROTOCOL_FAILURE;
    }
    
    // load the response body
    NPT_DataBuffer payload;
    result = response->GetEntity()->Load(payload);
    delete response;
    response = NULL;
    if (NPT_FAILED(result)) {
        return result;
    }

    // parse the packets in the response
    NPT_List<WmsAsfPacket> packets;
    result = WmsAsf_ParsePackets(payload.GetData(),
                                 payload.GetDataSize(),
                                 packets);
    if (BLT_FAILED(result)) {
        ATX_LOG_WARNING_1("failed to parse packets in response (%d)", result);
        return result;
    }
    
    // look for packets of type 'Header'
    NPT_DataBuffer asf_header;
    for (NPT_List<WmsAsfPacket>::Iterator i = packets.GetFirstItem();
                                          i;
                                        ++i) {
        if ((*i).m_Id == BLT_WMS_PACKET_ID_HEADER) {
            NPT_Size asf_header_size = asf_header.GetDataSize();
            if ((*i).m_Length > 4+4) {
                asf_header.SetDataSize(asf_header_size+(*i).m_Length-(4+4));
                NPT_CopyMemory(asf_header.UseData()+asf_header_size,
                               (*i).m_Data+(4+4), // skip unused MMS Data Packet fields 
                               (*i).m_Length-(4+4));
            }
        }
    }
    
    // check the size of the header
    if (asf_header.GetDataSize() < 50) {
        ATX_LOG_WARNING("not enough data in the ASF header");
        return BLT_ERROR_INVALID_MEDIA_FORMAT;
    }
    
    // parse the ASF header
    result = ParseAsf(asf_header.GetData(), asf_header.GetDataSize());
    if (BLT_FAILED(result)) {
        ATX_LOG_WARNING("failed to parse ASF header");
        return result;
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WmsClient::Select
+---------------------------------------------------------------------*/
BLT_Result
WmsClient::Select()
{
    int max_bitrate = -1;

    m_SelectedStream = 0;
    for (unsigned int i=1; i<BLT_WMS_ASF_MAX_STREAMS; i++) {
        if (m_Streams[i].m_Type == WmsClient::Stream::STREAM_TYPE_AUDIO) {
            if ((int)m_Streams[i].m_AverageBitrate > max_bitrate) {
                max_bitrate = m_Streams[i].m_AverageBitrate;
                m_SelectedStream = i;
            } 
        }
    }
    if (m_SelectedStream == 0) {
        ATX_LOG_WARNING("no stream selected");
        return BLT_ERROR_NOT_SUPPORTED;
    }
    m_Streams[m_SelectedStream].m_Selected = true;
    ATX_LOG_INFO_3("selected stream number %d, bitrate = %d, datarate = %d", 
                   m_SelectedStream, 
                   m_Streams[m_SelectedStream].m_AverageBitrate,
                   m_Streams[m_SelectedStream].m_DataRate);
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WmsClient::Play
+---------------------------------------------------------------------*/
BLT_Result
WmsClient::Play()
{
    // cleanup any previous state
    delete m_Response;
    m_Response = NULL;
    
    // setup a client to get the stream
    NPT_HttpClient  client;
    NPT_HttpRequest request(m_Url, NPT_HTTP_METHOD_GET);
    
    // setup the request headers
    NPT_String guid_header = "xClientGUID={";
    guid_header += m_Guid;
    guid_header += "}";
    NPT_String context_header;
    if (m_File.m_Flags & BLT_WMS_ASF_FILE_PROPERTY_FLAG_BROADCAST) {
        context_header = NPT_String::Format("no-cache,rate=1.000000,request-context=%d",
                                            m_RequestContext++);
    } else {
        context_header = NPT_String::Format("no-cache,rate=1.000000,stream-time=0,stream-offset=0:0,request-context=%d,max-duration=0",
                                            m_RequestContext++);
    }
    NPT_String switch_entry_header = "stream-switch-entry=";
    unsigned int stream_switch_count = 0;
    const char* separator = "";
    for (unsigned int i=0; i<BLT_WMS_ASF_MAX_STREAMS; i++) {
        if (m_Streams[i].m_Type == WmsClient::Stream::STREAM_TYPE_AUDIO) {
            switch_entry_header += NPT_String::Format("%sffff:%d:%d", separator, i, m_Streams[i].m_Selected?0:2);
            ++stream_switch_count;
            separator = " ";
        }
    }
    NPT_String switch_count_header = NPT_String::Format("stream-switch-count=%d", stream_switch_count);
    
    NPT_HttpHeaders& headers = request.GetHeaders();
    headers.SetHeader(NPT_HTTP_HEADER_USER_AGENT, m_UserAgent);
    headers.SetHeader("Accept", "*/*");
    headers.AddHeader("Pragma", "xPlayStrm=1");
    headers.AddHeader("Pragma", context_header);
    headers.AddHeader("Pragma", guid_header);
    headers.AddHeader("Pragma", switch_count_header);
    headers.AddHeader("Pragma", switch_entry_header);
    
    NPT_Result result = client.SendRequest(request, m_Response);
    if (NPT_FAILED(result)) return result;

    switch (m_Response->GetStatusCode()) {
        case 200:
            break;

        case 403:
            delete m_Response;
            m_Response = NULL;
            return ATX_ERROR_ACCESS_DENIED;
            
        case 404:
            delete m_Response;
            m_Response = NULL;
            return BLT_ERROR_STREAM_INPUT_NOT_FOUND;

        default:
            delete m_Response;
            m_Response = NULL;
            return BLT_ERROR_PROTOCOL_FAILURE;
    }

    // analyze the response type
    if (m_Response->GetEntity()->GetContentType() != "application/x-mms-framed" &&
        m_Response->GetEntity()->GetContentType() != "video/x-ms-wmv" &&
        m_Response->GetEntity()->GetContentType() != "audio/x-ms-wma") {
        ATX_LOG_WARNING_1("unexpected Play response type: %s", 
                          m_Response->GetEntity()->GetContentType().GetChars());
        return BLT_ERROR_PROTOCOL_FAILURE;
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WmsProtocol_ParseAsx
+---------------------------------------------------------------------*/
static BLT_Result
WmsProtocol_ParseAsx(WmsProtocol* self, const char* asx_data, ATX_Size asx_data_size)
{
    // delete any previous playlist
    delete self->playlist;
    
    // parse the new playlist
    return WmsAsxPlaylist::Parse(asx_data, asx_data_size, self->playlist);
}

/*----------------------------------------------------------------------
|   WmsProtocolInput_SetStream
+---------------------------------------------------------------------*/
BLT_METHOD
WmsProtocolInput_SetStream(BLT_InputStreamUser* _self,
                           ATX_InputStream*     stream,
                           const BLT_MediaType* media_type)
{
    WmsProtocol* self = ATX_SELF_M(input, WmsProtocol, BLT_InputStreamUser);
    ATX_Result   result;
    
    /* check parameters */
    if (stream == NULL) return ATX_ERROR_INVALID_STATE;
    
    /* check media type */
    if (media_type == NULL || 
        (media_type->id != self->input.asf_audio_media_type.id &&
         media_type->id != self->input.asf_video_media_type.id &&
         media_type->id != self->input.asf_application_media_type.id)) {
        return BLT_ERROR_INVALID_MEDIA_TYPE;
    }

    /* load the input stream */
    ATX_DataBuffer* asx_buffer = NULL;
    result = ATX_InputStream_Load(stream, BLT_WMS_PROTOCOL_MAX_ASX_SIZE, &asx_buffer);
    if (ATX_FAILED(result)) return result;
    if (asx_buffer == NULL) return BLT_ERROR_INVALID_MEDIA_FORMAT;
    
    /* parse the ASX buffer */
    result = WmsProtocol_ParseAsx(self,
                                  (const char*)ATX_DataBuffer_GetData(asx_buffer), 
                                  ATX_DataBuffer_GetDataSize(asx_buffer));
    if (NPT_FAILED(result)) return result;
    
    /* use the playlist */
    NPT_String url;
    NPT_List<WmsAsxPlaylist::Entry>::Iterator first_entry = self->playlist->m_Entries.GetFirstItem();
    if (first_entry) {
        for (NPT_List<WmsAsxPlaylist::Ref>::Iterator i = (*first_entry).m_Refs.GetFirstItem();
                                                     i;
                                                   ++i) {
            WmsAsxPlaylist::Ref& ref = *i;
            if (ref.m_Url.StartsWith("http://")) {
                // select this one
                url = ref.m_Url;
                break;
            }
            if (ref.m_Url.StartsWith("mms://")) {
                // try this as but don't stop here
                if (url.GetLength() == 0) {
                    url = "http://";
                    url += ref.m_Url.GetChars()+6;
                }
            }
        }
    }
    if (url.GetLength() == 0) {
        ATX_LOG_WARNING("no suitable streaming URL found");
        return BLT_ERROR_STREAM_INPUT_NOT_FOUND;
    }
    
    delete self->client;
    self->client = new WmsClient(url);
    result = self->client->Describe();
    if (BLT_FAILED(result)) {
        ATX_LOG_WARNING_1("Describe() failed (%d)", result);
        delete self->client;
        self->client = NULL;
        return result;
    }
    result = self->client->Select();
    if (BLT_FAILED(result)) {
        ATX_LOG_WARNING_1("Select() failed (%d)", result);
        delete self->client;
        self->client = NULL;
        return result;
    }
    result = self->client->Play();
    if (BLT_FAILED(result)) {
        ATX_LOG_WARNING_1("Play() failed (%d)", result);
        delete self->client;
        self->client = NULL;
        return result;
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WmsProtocolInput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
WmsProtocolInput_QueryMediaType(BLT_MediaPort*        _self,
                                BLT_Ordinal           index,
                                const BLT_MediaType** media_type)
{
    WmsProtocol* self = ATX_SELF_M(input, WmsProtocol, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = &self->input.asf_audio_media_type;
        return BLT_SUCCESS;
    } else if (index == 1) {
        *media_type = &self->input.asf_video_media_type;
        return BLT_SUCCESS;
    } else if (index == 2) {
        *media_type = &self->input.asf_application_media_type;
        return BLT_SUCCESS;
    } else{
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(WmsProtocolInput)
    ATX_GET_INTERFACE_ACCEPT(WmsProtocolInput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(WmsProtocolInput, BLT_InputStreamUser)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_InputStreamUser interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(WmsProtocolInput, BLT_InputStreamUser)
    WmsProtocolInput_SetStream
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(WmsProtocolInput, 
                                         "input",
                                         STREAM_PULL,
                                         IN)
ATX_BEGIN_INTERFACE_MAP(WmsProtocolInput, BLT_MediaPort)
    WmsProtocolInput_GetName,
    WmsProtocolInput_GetProtocol,
    WmsProtocolInput_GetDirection,
    WmsProtocolInput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   WmsProtocolOutput_QueryMediaType
+---------------------------------------------------------------------*/
BLT_METHOD
WmsProtocolOutput_QueryMediaType(BLT_MediaPort*        _self,
                                 BLT_Ordinal           index,
                                 const BLT_MediaType** media_type)
{
    WmsProtocol* self = ATX_SELF_M(output, WmsProtocol, BLT_MediaPort);
    
    if (index == 0) {
        *media_type = &self->output.media_type;
        return BLT_SUCCESS;
    } else {
        *media_type = NULL;
        return BLT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   WmsProtocolOutput_GetPacket
+---------------------------------------------------------------------*/
BLT_METHOD
WmsProtocolOutput_GetPacket(BLT_PacketProducer* _self,
                            BLT_MediaPacket**   packet)
{
    WmsProtocol* self = ATX_SELF_M(output, WmsProtocol, BLT_PacketProducer);

    // default value
    *packet = NULL;
    
    // check that we have a client
    if (self->client == NULL) return BLT_ERROR_EOS;
    
    NPT_InputStreamReference in_stream;
    self->client->m_Response->GetEntity()->GetInputStream(in_stream);
    unsigned char buffer[8];
    in_stream->ReadFully(buffer, 4);
    if ((buffer[0]&0x7F) != 0x24) {
        ATX_LOG_WARNING("ERROR: invalid framing");
    }
    unsigned char packet_type = buffer[1];
    unsigned int packet_length = NPT_BytesToInt16Le(&buffer[2]);
    ATX_LOG_FINEST_1("packet_length=%d", packet_length);
    if (packet_type == BLT_WMS_PACKET_ID_STREAM_CHANGE) {
        ATX_LOG_FINEST("STREAM CHANGE");
    } else if (packet_type == BLT_WMS_PACKET_ID_DATA) {
        ATX_LOG_FINEST("DATA");
    } else if (packet_type == BLT_WMS_PACKET_ID_EOS) {
        ATX_LOG_FINEST("EOS");
    } else if (packet_type == BLT_WMS_PACKET_ID_HEADER) {
        ATX_LOG_FINEST("HEADER");
    } else {
        ATX_LOG_FINEST_1("packet_type=%d", packet_type);
    }
    BLT_Core_CreateMediaPacket(ATX_BASE(self, BLT_BaseMediaNode).core, 4+packet_length, &self->output.media_type, packet);
    BLT_MediaPacket_SetPayloadSize(*packet, packet_length+4);
    unsigned char* payload = (unsigned char*)BLT_MediaPacket_GetPayloadBuffer(*packet);
    payload[0] = buffer[0];
    payload[1] = buffer[1];
    payload[2] = buffer[2];
    payload[3] = buffer[3];
    if (packet_length) {
        if (packet_type == BLT_WMS_PACKET_ID_HEADER || packet_type == BLT_WMS_PACKET_ID_DATA) {
            in_stream->ReadFully(payload+4, 8);
            unsigned int location_id = NPT_BytesToInt32Le(payload+4);
            unsigned int packet_length_2 = NPT_BytesToInt16Le(payload+10);
            if (packet_length_2 != packet_length) {
                ATX_LOG_WARNING("packet length mismatch\n");
            }
            ATX_LOG_FINE_3("(%d) - AF=%d, location=%d\n", packet_length_2, buffer[5], location_id);
            in_stream->ReadFully(payload+12, packet_length-8);
            
            /* set some of the packet flags */
            if (packet_type == BLT_WMS_PACKET_ID_HEADER) {
                // reuse the 'incarnation' header byte to indicate the stream that was selected
                payload[8] = self->client->GetSelectedStream();
            }
        } else {
            in_stream->ReadFully(payload+4, packet_length);
        }
    }
    
    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(WmsProtocolOutput)
    ATX_GET_INTERFACE_ACCEPT(WmsProtocolOutput, BLT_MediaPort)
    ATX_GET_INTERFACE_ACCEPT(WmsProtocolOutput, BLT_PacketProducer)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   BLT_MediaPort interface
+---------------------------------------------------------------------*/
BLT_MEDIA_PORT_IMPLEMENT_SIMPLE_TEMPLATE(WmsProtocolOutput, 
                                         "output",
                                         PACKET,
                                         OUT)
ATX_BEGIN_INTERFACE_MAP(WmsProtocolOutput, BLT_MediaPort)
    WmsProtocolOutput_GetName,
    WmsProtocolOutput_GetProtocol,
    WmsProtocolOutput_GetDirection,
    WmsProtocolOutput_QueryMediaType
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   BLT_PacketProducer interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(WmsProtocolOutput, BLT_PacketProducer)
    WmsProtocolOutput_GetPacket
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   WmsProtocol_Destroy
+---------------------------------------------------------------------*/
static BLT_Result
WmsProtocol_Destroy(WmsProtocol* self)
{
    ATX_LOG_FINE("enter");

    /* delete member objects */
    delete self->playlist;
    delete self->client;
    
    /* release any packet we may have */
    if (self->output.packet) BLT_MediaPacket_Release(self->output.packet);

    /* destruct the inherited object */
    BLT_BaseMediaNode_Destruct(&ATX_BASE(self, BLT_BaseMediaNode));

    /* free the object memory */
    ATX_FreeMemory(self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    WmsProtocol_Deactivate
+---------------------------------------------------------------------*/
BLT_METHOD
WmsProtocol_Deactivate(BLT_MediaNode* _self)
{
    //WmsProtocol* self = ATX_SELF_EX(WmsProtocol, BLT_BaseMediaNode, BLT_MediaNode);

    ATX_LOG_FINER("enter");

    /* call the base class method */
    BLT_BaseMediaNode_Deactivate(_self);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WmsProtocol_GetPortByName
+---------------------------------------------------------------------*/
BLT_METHOD
WmsProtocol_GetPortByName(BLT_MediaNode*  _self,
                          BLT_CString     name,
                          BLT_MediaPort** port)
{
    WmsProtocol* self = ATX_SELF_EX(WmsProtocol, BLT_BaseMediaNode, BLT_MediaNode);

    if (ATX_StringsEqual(name, "input")) {
        *port = &ATX_BASE(&self->input, BLT_MediaPort);
        return BLT_SUCCESS;
    } else if (ATX_StringsEqual(name, "output")) {
        *port = &ATX_BASE(&self->output, BLT_MediaPort);
        return BLT_SUCCESS;
    } else {
        *port = NULL;
        return BLT_ERROR_NO_SUCH_PORT;
    }
}

/*----------------------------------------------------------------------
|   WmsProtocol_Seek
+---------------------------------------------------------------------*/
BLT_METHOD
WmsProtocol_Seek(BLT_MediaNode* _self,
                 BLT_SeekMode*  mode,
                 BLT_SeekPoint* point)
{
    WmsProtocol* self = ATX_SELF_EX(WmsProtocol, BLT_BaseMediaNode, BLT_MediaNode);

    /* estimate the seek point */
    if (ATX_BASE(self, BLT_BaseMediaNode).context == NULL) return BLT_FAILURE;
    BLT_Stream_EstimateSeekPoint(ATX_BASE(self, BLT_BaseMediaNode).context, *mode, point);
    if (!(point->mask & BLT_SEEK_POINT_MASK_OFFSET)) {
        return BLT_FAILURE;
    }

    /* align the offset to the nearest sample */
    /*point->offset -= point->offset%(self->output.block_size);*/

    /* seek to the estimated offset */
    /* seek into the input stream (ignore return value) */
    
    /* set the mode so that the nodes down the chain know the seek has */
    /* already been done on the stream                                  */
    *mode = BLT_SEEK_MODE_IGNORE;

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|    GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(WmsProtocol)
    ATX_GET_INTERFACE_ACCEPT_EX(WmsProtocol, BLT_BaseMediaNode, BLT_MediaNode)
    ATX_GET_INTERFACE_ACCEPT_EX(WmsProtocol, BLT_BaseMediaNode, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    BLT_MediaNode interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(WmsProtocol, BLT_BaseMediaNode, BLT_MediaNode)
    BLT_BaseMediaNode_GetInfo,
    WmsProtocol_GetPortByName,
    BLT_BaseMediaNode_Activate,
    WmsProtocol_Deactivate,
    BLT_BaseMediaNode_Start,
    BLT_BaseMediaNode_Stop,
    BLT_BaseMediaNode_Pause,
    BLT_BaseMediaNode_Resume,
    WmsProtocol_Seek
ATX_END_INTERFACE_MAP_EX

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(WmsProtocol, 
                                         BLT_BaseMediaNode, 
                                         reference_count)

/*----------------------------------------------------------------------
|   WmsProtocol_Create
+---------------------------------------------------------------------*/
static BLT_Result
WmsProtocol_Create(BLT_Module*              module,
                   BLT_Core*                core, 
                   BLT_ModuleParametersType parameters_type,
                   const void*              parameters, 
                   BLT_MediaNode**          object)
{
    WmsProtocol* self;

    ATX_LOG_FINE("enter");

    /* check parameters */
    if (parameters == NULL || 
        parameters_type != BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR) {
        return BLT_ERROR_INVALID_PARAMETERS;
    }

    /* allocate memory for the object */
    self = (WmsProtocol*)ATX_AllocateZeroMemory(sizeof(WmsProtocol));
    if (self == NULL) {
        *object = NULL;
        return BLT_ERROR_OUT_OF_MEMORY;
    }

    /* construct the inherited object */
    BLT_BaseMediaNode_Construct(&ATX_BASE(self, BLT_BaseMediaNode), module, core);

    /* construct the object */
    BLT_MediaType_Init(&self->input.asf_audio_media_type,
                       ((WmsProtocolModule*)module)->asf_audio_type_id);
    BLT_MediaType_Init(&self->input.asf_video_media_type,
                       ((WmsProtocolModule*)module)->asf_video_type_id);
    BLT_MediaType_Init(&self->input.asf_application_media_type,
                       ((WmsProtocolModule*)module)->asf_application_type_id);
    BLT_MediaType_Init(&self->output.media_type,
                       ((WmsProtocolModule*)module)->mms_framed_type_id);

    /* setup interfaces */
    ATX_SET_INTERFACE_EX(self, WmsProtocol, BLT_BaseMediaNode, BLT_MediaNode);
    ATX_SET_INTERFACE_EX(self, WmsProtocol, BLT_BaseMediaNode, ATX_Referenceable);
    ATX_SET_INTERFACE(&self->input,  WmsProtocolInput,  BLT_MediaPort);
    ATX_SET_INTERFACE(&self->input,  WmsProtocolInput,  BLT_InputStreamUser);
    ATX_SET_INTERFACE(&self->output, WmsProtocolOutput, BLT_MediaPort);
    ATX_SET_INTERFACE(&self->output, WmsProtocolOutput, BLT_PacketProducer);
    *object = &ATX_BASE_EX(self, BLT_BaseMediaNode, BLT_MediaNode);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WmsProtocolModule_Attach
+---------------------------------------------------------------------*/
BLT_METHOD
WmsProtocolModule_Attach(BLT_Module* _self, BLT_Core* core)
{
    WmsProtocolModule* self = ATX_SELF_EX(WmsProtocolModule, BLT_BaseModule, BLT_Module);
    BLT_Registry*      registry;
    BLT_Result         result;

    /* get the registry */
    result = BLT_Core_GetRegistry(core, &registry);
    if (BLT_FAILED(result)) return result;

    /* register the ".asx" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".asx",
                                            "video/x-ms-asf");
    if (BLT_FAILED(result)) return result;

    /* register the ".asf" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".asf",
                                            "video/x-ms-asf");
    if (BLT_FAILED(result)) return result;

    /* register the ".wma" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".wvma",
                                            "audio/x-ms-wma");
    if (BLT_FAILED(result)) return result;

    /* register the ".wmv" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".wmv",
                                            "video/x-ms-wmv");
    if (BLT_FAILED(result)) return result;

    /* register the ".wmx" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".wmx",
                                            "video/x-ms-wmx");
    if (BLT_FAILED(result)) return result;

    /* register the ".wvx" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".wvx",
                                            "video/x-ms-asf");
    if (BLT_FAILED(result)) return result;

    /* register the ".wax" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".wax",
                                            "audio/x-ms-asf");
    if (BLT_FAILED(result)) return result;

    /* register the ".wm" file extension */
    result = BLT_Registry_RegisterExtension(registry, 
                                            ".wm",
                                            "video/x-ms-wm");
    if (BLT_FAILED(result)) return result;

    /* get the type id for "video/x-ms-asf" */
    result = BLT_Registry_GetIdForName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "video/x-ms-asf",
        &self->asf_video_type_id);
    if (BLT_FAILED(result)) return result;

    /* get the type id for "audio/x-ms-asf" */
    result = BLT_Registry_GetIdForName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "audio/x-ms-asf",
        &self->asf_audio_type_id);
    if (BLT_FAILED(result)) return result;
    
    /* register the type id for application/x-ms-asf */
    result = BLT_Registry_RegisterName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "application/x-ms-asf",
        &self->asf_application_type_id);
    if (BLT_FAILED(result)) return result;
    
    /* register the type id for application/x-mms-framed */
    result = BLT_Registry_RegisterName(
        registry,
        BLT_REGISTRY_NAME_CATEGORY_MEDIA_TYPE_IDS,
        "application/x-mms-framed",
        &self->mms_framed_type_id);
    if (BLT_FAILED(result)) return result;

    ATX_LOG_FINE_1("(video/x-ms-asf type = %d)",           self->asf_video_type_id);
    ATX_LOG_FINE_1("(audio/x-ms-asf type = %d)",           self->asf_audio_type_id);
    ATX_LOG_FINE_1("(application/x-ms-asf type = %d)",     self->asf_application_type_id);
    ATX_LOG_FINE_1("(application/x-mms-framed type = %d)", self->mms_framed_type_id);

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   WmsProtocolModule_Probe
+---------------------------------------------------------------------*/
BLT_METHOD
WmsProtocolModule_Probe(BLT_Module*              _self, 
                        BLT_Core*                core,
                        BLT_ModuleParametersType parameters_type,
                        BLT_AnyConst             parameters,
                        BLT_Cardinal*            match)
{
    WmsProtocolModule* self = ATX_SELF_EX(WmsProtocolModule, BLT_BaseModule, BLT_Module);
    BLT_COMPILER_UNUSED(core);

    switch (parameters_type) {
      case BLT_MODULE_PARAMETERS_TYPE_MEDIA_NODE_CONSTRUCTOR:
        {
            BLT_MediaNodeConstructor* constructor = 
                (BLT_MediaNodeConstructor*)parameters;

            /* we need the input protocol to be STREAM_PULL and the output */
            /* protocol to be PACKET                                       */
             if ((constructor->spec.input.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                  constructor->spec.input.protocol != BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL) ||
                (constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_ANY &&
                 constructor->spec.output.protocol != BLT_MEDIA_PORT_PROTOCOL_PACKET)) {
                return BLT_FAILURE;
            }

            /* we need the input media type to be 'video/x-ms-asf' */
            if (constructor->spec.input.media_type->id != self->asf_audio_type_id &&
                constructor->spec.input.media_type->id != self->asf_video_type_id &&
                constructor->spec.input.media_type->id != self->asf_application_type_id) {
                return BLT_FAILURE;
            }

            /* the output type should be unknown or application/x-mms-framed at this point */
            if (constructor->spec.output.media_type->id != BLT_MEDIA_TYPE_ID_UNKNOWN &&
                constructor->spec.output.media_type->id != self->mms_framed_type_id) {
                return BLT_FAILURE;
            }

            /* compute the match level */
            if (constructor->name != NULL) {
                /* we're being probed by name */
                if (ATX_StringsEqual(constructor->name, "WmsProtocol")) {
                    /* our name */
                    *match = BLT_MODULE_PROBE_MATCH_EXACT;
                } else {
                    /* not out name */
                    return BLT_FAILURE;
                }
            } else {
                /* we're probed by protocol/type specs only */
                *match = BLT_MODULE_PROBE_MATCH_MAX - 10;
            }

            ATX_LOG_FINE_1("WmsProtocolModule::Probe - Ok [%d]", *match);
            return BLT_SUCCESS;
        }    
        break;

      default:
        break;
    }

    return BLT_FAILURE;
}

/*----------------------------------------------------------------------
|   GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(WmsProtocolModule)
    ATX_GET_INTERFACE_ACCEPT_EX(WmsProtocolModule, BLT_BaseModule, BLT_Module)
    ATX_GET_INTERFACE_ACCEPT_EX(WmsProtocolModule, BLT_BaseModule, ATX_Referenceable)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|   node factory
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_SIMPLE_MEDIA_NODE_FACTORY(WmsProtocolModule, WmsProtocol)

/*----------------------------------------------------------------------
|   BLT_Module interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP_EX(WmsProtocolModule, BLT_BaseModule, BLT_Module)
    BLT_BaseModule_GetInfo,
    WmsProtocolModule_Attach,
    WmsProtocolModule_CreateInstance,
    WmsProtocolModule_Probe
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|   ATX_Referenceable interface
+---------------------------------------------------------------------*/
#define WmsProtocolModule_Destroy(x) \
    BLT_BaseModule_Destroy((BLT_BaseModule*)(x))

ATX_IMPLEMENT_REFERENCEABLE_INTERFACE_EX(WmsProtocolModule, 
                                         BLT_BaseModule,
                                         reference_count)

/*----------------------------------------------------------------------
|   module object
+---------------------------------------------------------------------*/
BLT_MODULE_IMPLEMENT_STANDARD_GET_MODULE(WmsProtocolModule,
                                         "Windows Media Services Protocol",
                                         "com.axiosys.protocols.wms",
                                         "1.0.0",
                                         BLT_MODULE_AXIOMATIC_COPYRIGHT)
