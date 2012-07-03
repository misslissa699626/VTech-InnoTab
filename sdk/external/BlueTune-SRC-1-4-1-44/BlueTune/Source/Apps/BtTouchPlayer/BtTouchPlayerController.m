#import "BtTouchPlayerController.h"

#if 0
@interface CocoaPlayerRecordList : NSObject 
{
    NSMutableArray* records;
}
-(int) numberOfRowsInTableView:(NSTableView *)aTableView;
-(id)        tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex;
@end


@implementation CocoaPlayerRecordList

-(id) init
{
    if ((self = [super init])) {
        records = [[NSMutableArray alloc] init];
    }
    return self;
}

-(void) dealloc
{
    [records release];
    [super dealloc];
}

-(int) numberOfRowsInTableView: (NSTableView *)view
{
    return [records count];
}

-(id)             tableView: (NSTableView *)   view 
  objectValueForTableColumn: (NSTableColumn *) column 
                        row: (int)             rowIndex
{
    id record = [records objectAtIndex: rowIndex];
    id value = [record objectForKey: [column identifier]];    
    return value;
}

-(void) setProperty: (NSString*) name value: (NSString*) value
{
    NSMutableDictionary* dict = [[NSMutableDictionary alloc] init];
    [dict setValue: name forKey: @"Name"];
    [dict setValue: value forKey: @"Value"];
    [records addObject: dict];
}

@end
#endif

@implementation TouchPlayerController
-(id) init 
{
    if ((self = [super init])) {
        player = [[BLT_PlayerObject alloc] init];
        [player setDelegate: self];
    }
    volume = 1.0f;
    pendingVolume = 1.0f;
    positionSliderPressed = FALSE;
    return self;
}

-(void) awakeFromNib
{        
    // set the data source for the list views
    //[playerPropertiesView setDataSource: [[CocoaPlayerRecordList alloc]init]];
    //[playerStreamInfoView setDataSource: [[CocoaPlayerRecordList alloc]init]];
}

-(IBAction) play: (id) sender;
{
    [player play];
}

-(IBAction) pause: (id) sender;
{
    [player pause];
}

-(IBAction) stop: (id) sender;
{
    [player stop];
}

-(IBAction) setInput: (id) sender;
{
    //[player setInput: @"http://www.bok.net/tmp/test.mp3"];
    [player stop];
    [player setInput: [playerInput text]];
}

-(IBAction) seek: (id) sender;
{
    positionSliderPressed = FALSE;
    [player seekToPosition: (unsigned int)(playerPosition.value*1000.0f)  range: (unsigned int)(playerPosition.maximumValue*1000.0f)];
}

-(IBAction) setVolume: (id) sender;
{
    if (pendingVolume == volume) {
        [player setVolume: [playerVolume value]];
    }
    pendingVolume = [playerVolume value];
}

-(IBAction) positionSliderWasPressed: (id) sender;
{
    positionSliderPressed = TRUE;
}

-(void) ackWasReceived: (BLT_Player_CommandId) command_id
{
    printf("ACK %d\n", command_id);
    
    if (command_id == BLT_PLAYER_COMMAND_ID_SET_INPUT) {
        // autoplay
        [player play];
    } else if (command_id == BLT_PLAYER_COMMAND_ID_SET_VOLUME) {
        // update the actual volume value
        volume = pendingVolume;
    }
}

-(void) nackWasReceived: (BLT_Player_CommandId) command_id result: (BLT_Result) result
{
    printf("NACK %d, %d\n", command_id, result);
}

-(void) decoderStateDidChange: (BLT_Player_DecoderState) state
{
    switch (state) {
        case BLT_PLAYER_DECODER_STATE_EOS:
            playerState.text = @"[EOS]"; break;
        case BLT_PLAYER_DECODER_STATE_STOPPED:
            playerState.text = @"[STOPPED]"; break;
        case BLT_PLAYER_DECODER_STATE_PLAYING:
            playerState.text = @"[PLAYING]"; break;
        case BLT_PLAYER_DECODER_STATE_PAUSED:
            playerState.text = @"[PAUSED]"; break;
    }
}

-(void) streamTimecodeDidChange: (BLT_TimeCode) timecode
{
    NSString* timecode_string = [NSString stringWithFormat:@"%02d:%02d:%02d", timecode.h, timecode.m, timecode.s];
    playerTimecode.text = timecode_string;
}

-(void) streamPositionDidChange: (BLT_StreamPosition) position
{
    if (!positionSliderPressed) {
        float where = (float)position.offset/(float)position.range;
        playerPosition.value = where;
    }
}

-(void) streamInfoDidChange: (const BLT_StreamInfo*) info updateMask: (BLT_Mask) mask
{
    /*if (mask & BLT_STREAM_INFO_MASK_DATA_TYPE) {
        [[playerStreamInfoView dataSource] setProperty: @"Data Type" value: [NSString stringWithUTF8String: info->data_type]];
    }
    if (mask & BLT_STREAM_INFO_MASK_DURATION) {
        [[playerStreamInfoView dataSource] setProperty: @"Duration" value: [NSString stringWithFormat:@"%2f", (float)info->duration/1000.0f]];
    }
    if (mask & BLT_STREAM_INFO_MASK_NOMINAL_BITRATE) {
        [[playerStreamInfoView dataSource] setProperty: @"Nominal Bitrate" value: [NSString stringWithFormat:@"%d", info->nominal_bitrate]];
    }
    if (mask & BLT_STREAM_INFO_MASK_AVERAGE_BITRATE) {
        [[playerStreamInfoView dataSource] setProperty: @"Average Bitrate" value: [NSString stringWithFormat:@"%d", info->average_bitrate]];
    }
    if (mask & BLT_STREAM_INFO_MASK_INSTANT_BITRATE) {
        [[playerStreamInfoView dataSource] setProperty: @"Instant Bitrate" value: [NSString stringWithFormat:@"%d", info->instant_bitrate]];
    }
    if (mask & BLT_STREAM_INFO_MASK_SIZE) {
        [[playerStreamInfoView dataSource] setProperty: @"Size" value: [NSString stringWithFormat:@"%d", info->size]];
    }
    if (mask & BLT_STREAM_INFO_MASK_SAMPLE_RATE) {
        [[playerStreamInfoView dataSource] setProperty: @"Sample Rate" value: [NSString stringWithFormat:@"%d", info->sample_rate]];
    }
    if (mask & BLT_STREAM_INFO_MASK_CHANNEL_COUNT) {
        [[playerStreamInfoView dataSource] setProperty: @"Channels" value: [NSString stringWithFormat:@"%d", info->channel_count]];
    }
    if (mask & BLT_STREAM_INFO_MASK_FLAGS) {
        [[playerStreamInfoView dataSource] setProperty: @"Flags" value: [NSString stringWithFormat:@"%x", info->flags]];
    }
    [playerStreamInfoView reloadData];*/
}

-(void) propertyDidChange: (BLT_PropertyScope)        scope 
                   source: (const char*)              source 
                     name: (const char*)              name 
                    value: (const ATX_PropertyValue*) value
{
    if (name == NULL || value == NULL) return;
    
    NSString* value_string = nil;
    switch (value->type) {
        case ATX_PROPERTY_VALUE_TYPE_STRING:
            value_string = [NSString stringWithUTF8String: value->data.string];
            break;

        case ATX_PROPERTY_VALUE_TYPE_INTEGER:
            value_string = [NSString stringWithFormat:@"%d", value->data.string];
            break;
            
        default:
            value_string = @"";
    }
    if (value_string) {
        //[[playerPropertiesView dataSource] setProperty: [NSString stringWithUTF8String: name] value: value_string];
    }
}

@end
