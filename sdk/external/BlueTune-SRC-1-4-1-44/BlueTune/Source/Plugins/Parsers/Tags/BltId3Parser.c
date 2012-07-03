/*****************************************************************
|
|   ID3 Parser Library
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Atomix.h"
#include "BltConfig.h"
#include "BltTypes.h"
#include "BltErrors.h"
#include "BltId3Parser.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
ATX_SET_LOCAL_LOGGER("bluetune.plugins.parsers.tags.id3")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define BLT_ID3V1_TAG_SIZE                    128
#define BLT_ID3V1_TAG_MAX_STRING_LENGTH       30
#define BLT_ID3V1_TAG_TITLE_OFFSET            3
#define BLT_ID3V1_TAG_TITLE_SIZE              30
#define BLT_ID3V1_TAG_ARTIST_OFFSET           33
#define BLT_ID3V1_TAG_ARTIST_SIZE             30
#define BLT_ID3V1_TAG_ALBUM_OFFSET            63
#define BLT_ID3V1_TAG_ALBUM_SIZE              30
#define BLT_ID3V1_TAG_YEAR_OFFSET             93
#define BLT_ID3V1_TAG_YEAR_SIZE               4
#define BLT_ID3V1_TAG_COMMENT_OFFSET          97
#define BLT_ID3V1_TAG_COMMENT_SIZE            30
#define BLT_ID3V1_TAG_GENRE_OFFSET            127

#define BLT_ID3V2_TAG_HEADER_SIZE             10
#define BLT_ID3V2_TAG_FOOTER_SIZE             10
#define BLT_ID3V2_TAG_HEADER_FLAG_HAS_FOOTER  0x10

static const char *const BLT_Id3GenreTable[] = {
    "Blues",
    "Classic Rock",
    "Country",
    "Dance",
    "Disco",
    "Funk",
    "Grunge",
    "Hip-Hop",
    "Jazz",
    "Metal",
    "New Age",
    "Oldies",
    "Other",
    "Pop",
    "R&B",
    "Rap",
    "Reggae",
    "Rock",
    "Techno",
    "Industrial",
    "Alternative",
    "Ska",
    "Death Metal",
    "Pranks",
    "Soundtrack",
    "Euro-Techno",
    "Ambient",
    "Trip-Hop",
    "Vocal",
    "Jazz+Funk",
    "Fusion",
    "Trance",
    "Classical",
    "Instrumental",
    "Acid",
    "House",
    "Game",
    "Sound Clip",
    "Gospel",
    "Noise",
    "AlternRock",
    "Bass",
    "Soul",
    "Punk",
    "Space",
    "Meditative",
    "Instrumental Pop",
    "Instrumental Rock",
    "Ethnic",
    "Gothic",
    "Darkwave",
    "Techno-Industrial",
    "Electronic",
    "Pop-Folk",
    "Eurodance",
    "Dream",
    "Southern Rock",
    "Comedy",
    "Cult",
    "Gangsta",
    "Top 40",
    "Christian Rap",
    "Pop/Funk",
    "Jungle",
    "Native American",
    "Cabaret",
    "New Wave",
    "Psychadelic",
    "Rave",
    "Showtunes",
    "Trailer",
    "Lo-Fi",
    "Tribal",
    "Acid Punk",
    "Acid Jazz",
    "Polka",
    "Retro",
    "Musical",
    "Rock & Roll",
    "Hard Rock"
};

/*----------------------------------------------------------------------
|   BLT_Id3Parser_TrimString
+---------------------------------------------------------------------*/
static void
BLT_Id3Parser_TrimString(char* buffer, BLT_Size max_size)
{
    ATX_Size length = ATX_StringLength(buffer);

    buffer[max_size] = '\0';
    if (length > 0) {
        char* tail = &buffer[ATX_StringLength(buffer)-1];
        while (tail >= buffer && *tail == ' ') {
            *tail-- = '\0';
        }
    }
}

/*----------------------------------------------------------------------
|   BLT_Id3Parser_ParseV1
+---------------------------------------------------------------------*/
static BLT_Result
BLT_Id3Parser_ParseV1(ATX_InputStream* stream, 
                      BLT_LargeSize    stream_size,
                      BLT_Size*        tag_size,
                      ATX_Properties*  properties)
{
    char              id3_tag[BLT_ID3V1_TAG_SIZE];
    char              buffer[BLT_ID3V1_TAG_MAX_STRING_LENGTH+1];
    ATX_Properties*   stream_properties;
    ATX_PropertyValue property_value;
    BLT_Result        result;

    /* check that the size if enough to hold and BLT_ID3 tag */
    if (stream_size < BLT_ID3V1_TAG_SIZE) return BLT_FAILURE;

    /* default value */
    *tag_size = 0;
    
    /* don't seek if the stream says it seeks slowly */
    stream_properties = ATX_CAST(stream, ATX_Properties);
    if (stream_properties) {
        result = ATX_Properties_GetProperty(stream_properties, 
                                            ATX_INPUT_STREAM_PROPERTY_SEEK_SPEED,
                                            &property_value);
        if (ATX_SUCCEEDED(result) && 
            property_value.type == ATX_PROPERTY_VALUE_TYPE_INTEGER &&
            property_value.data.integer <= ATX_INPUT_STREAM_SEEK_SPEED_SLOW) {
            ATX_LOG_FINER("BLT_Id3Parser::ParseV1 - not seeking for ID3 footer, source is slow");
            return BLT_FAILURE;
        }
    }
    
    /* seek to start of tag */
    result = ATX_InputStream_Seek(stream, stream_size-BLT_ID3V1_TAG_SIZE);
    if (BLT_FAILED(result)) return result;

    /* read the tag into a buffer */
    result = ATX_InputStream_ReadFully(stream, id3_tag, BLT_ID3V1_TAG_SIZE);
    if (BLT_FAILED(result)) return result;

    /* check that it is a tag */
    if (id3_tag[0] != 'T' ||
        id3_tag[1] != 'A' ||
        id3_tag[2] != 'G') {
        return BLT_FAILURE;
    }

    /* we have an ID3V1 trailer */
    *tag_size = BLT_ID3V1_TAG_SIZE;

    /* parse tag */
    ATX_CopyStringN(buffer, 
                    &id3_tag[BLT_ID3V1_TAG_TITLE_OFFSET], 
                    BLT_ID3V1_TAG_TITLE_SIZE);
    BLT_Id3Parser_TrimString(buffer, BLT_ID3V1_TAG_TITLE_SIZE);
    property_value.data.string = buffer;
    property_value.type = ATX_PROPERTY_VALUE_TYPE_STRING;
    ATX_Properties_SetProperty(properties,
                               "Tags/Title",
                               &property_value);

    ATX_CopyStringN(buffer, 
                    &id3_tag[BLT_ID3V1_TAG_ARTIST_OFFSET], 
                    BLT_ID3V1_TAG_ARTIST_SIZE);
    BLT_Id3Parser_TrimString(buffer, BLT_ID3V1_TAG_ARTIST_SIZE);
    property_value.data.string = buffer;
    property_value.type = ATX_PROPERTY_VALUE_TYPE_STRING;
    ATX_Properties_SetProperty(properties,
                               "Tags/Artist",
                               &property_value);

    ATX_CopyStringN(buffer, 
                    &id3_tag[BLT_ID3V1_TAG_ALBUM_OFFSET], 
                    BLT_ID3V1_TAG_ALBUM_SIZE);
    BLT_Id3Parser_TrimString(buffer, BLT_ID3V1_TAG_ALBUM_SIZE);
    property_value.data.string = buffer;
    property_value.type = ATX_PROPERTY_VALUE_TYPE_STRING;
    ATX_Properties_SetProperty(properties,
                               "Tags/Album",
                               &property_value);

    ATX_CopyStringN(buffer, 
                    &id3_tag[BLT_ID3V1_TAG_YEAR_OFFSET], 
                    BLT_ID3V1_TAG_YEAR_SIZE);
    BLT_Id3Parser_TrimString(buffer, BLT_ID3V1_TAG_YEAR_SIZE);
    property_value.data.string = buffer;
    property_value.type = ATX_PROPERTY_VALUE_TYPE_STRING;
    ATX_Properties_SetProperty(properties,
                               "Tags/Year",
                               &property_value);

    ATX_CopyStringN(buffer, 
                  &id3_tag[BLT_ID3V1_TAG_COMMENT_OFFSET], 
                  BLT_ID3V1_TAG_COMMENT_SIZE);
    BLT_Id3Parser_TrimString(buffer, BLT_ID3V1_TAG_COMMENT_SIZE);
    property_value.data.string = buffer;
    property_value.type = ATX_PROPERTY_VALUE_TYPE_STRING;
    ATX_Properties_SetProperty(properties,
                               "Tags/Comment",
                               &property_value);

    if ((unsigned char)id3_tag[BLT_ID3V1_TAG_GENRE_OFFSET] < 
        ATX_ARRAY_SIZE(BLT_Id3GenreTable)) {
        property_value.data.string = BLT_Id3GenreTable[(int)id3_tag[BLT_ID3V1_TAG_GENRE_OFFSET]];
        property_value.type = ATX_PROPERTY_VALUE_TYPE_STRING;
        ATX_Properties_SetProperty(properties,
                                   "Tags/Genre",
                                   &property_value);
    }

    /* check for ID3 V1.1 extension (i.e track number at the end of */
    /* the comment tag                                              */
    if (id3_tag[BLT_ID3V1_TAG_GENRE_OFFSET-2] == 0 &&
        id3_tag[BLT_ID3V1_TAG_GENRE_OFFSET-1]) {
        property_value.data.integer = id3_tag[BLT_ID3V1_TAG_GENRE_OFFSET-1];
        property_value.type = ATX_PROPERTY_VALUE_TYPE_INTEGER;
        ATX_Properties_SetProperty(properties,
                                   "Tags/Index",
                                   &property_value);
    }

    return BLT_SUCCESS;
}

/*----------------------------------------------------------------------
|   BLT_Id3Parser_ParseV2
+---------------------------------------------------------------------*/
static BLT_Result
BLT_Id3Parser_ParseV2(ATX_InputStream* stream, 
                      BLT_LargeSize    stream_size, 
                      BLT_Size*        tag_size,
                      ATX_Properties*  properties)
{
    char       header[BLT_ID3V2_TAG_HEADER_SIZE];
    BLT_Result result;
    
    BLT_COMPILER_UNUSED(properties);

    /* check that the size if enough to hold and BLT_ID3 tag */
    if (stream_size < BLT_ID3V2_TAG_HEADER_SIZE) {
        return BLT_FAILURE;
    }

    /* read the tag header into a buffer */
    result = ATX_InputStream_ReadFully(stream, header, BLT_ID3V2_TAG_HEADER_SIZE);
    if (BLT_FAILED(result)) return result;

    /* if there is an ID3v2 header, look at its size */
    if (header[0] == 'I' &&
        header[1] == 'D' &&
        header[2] == '3') {
        unsigned int footer_size = 0;
        /* look at the tag version */
        if (header[3] == 2 || header[3] == 3) {
            /* version 2.x or 3.x */
        } else if (header[3] == 4) {
            /* version 4.x */
            if (header[5] & BLT_ID3V2_TAG_HEADER_FLAG_HAS_FOOTER) {
                footer_size = BLT_ID3V2_TAG_FOOTER_SIZE;
            }
        } else {
            /* unsupported */
            return BLT_FAILURE;
        }

        /* get the tag size */
        *tag_size = BLT_ID3V2_TAG_HEADER_SIZE + footer_size +
            (((unsigned long)(header[9] & 0x7F)      ) |
             ((unsigned long)(header[8] & 0x7F) <<  7) |
             ((unsigned long)(header[7] & 0x7F) << 14) |
             ((unsigned long)(header[6] & 0x7F) << 21));
        return BLT_SUCCESS;
    }

    return BLT_FAILURE;
}

/*----------------------------------------------------------------------
|   BLT_Id3Parser_ParseStream
+---------------------------------------------------------------------*/
BLT_Result
BLT_Id3Parser_ParseStream(ATX_InputStream* stream, 
                          BLT_Position     stream_start,
                          BLT_LargeSize    stream_size,
                          BLT_Size*        header_size,
                          BLT_Size*        trailer_size, 
                          ATX_Properties*  properties)
{
    BLT_Result result_v1;
    BLT_Result result_v2;
    
    /* default values */
    *header_size  = 0;
    *trailer_size = 0;

    /* try ID3V1 */
    result_v1 = BLT_Id3Parser_ParseV1(stream, 
                                      stream_size, 
                                      trailer_size, 
                                      properties);

    /* rewind to where we were before parsing */
    ATX_InputStream_Seek(stream, stream_start);

    /* try ID3V2 */
    result_v2 = BLT_Id3Parser_ParseV2(stream, 
                                      stream_size, 
                                      header_size, 
                                      properties);

    /* rewind to where we were before parsing */
    ATX_InputStream_Seek(stream, stream_start);

    return 
        (result_v1 == BLT_SUCCESS || result_v2 == BLT_SUCCESS) ?
        BLT_SUCCESS : BLT_FAILURE;
}
