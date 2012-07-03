#import <UIKit/UIKit.h>
#import "BlueTune.h"

@interface TouchPlayerController : NSObject 
{
    BLT_PlayerObject*          player;
    IBOutlet UIButton*         openButton;
    IBOutlet UIButton*         playButton;
    IBOutlet UIButton*         pauseButton;
    IBOutlet UIButton*         stopButton;
    IBOutlet UILabel*          playerState;
    IBOutlet UILabel*          playerTimecode;
    IBOutlet UITextField*      playerInput;
    IBOutlet UISlider*         playerPosition;
    IBOutlet UISlider*         playerVolume;
    IBOutlet UITableView*      playerStreamInfoView;
    IBOutlet UITableView*      playerPropertiesView;
    
    float volume;
    float pendingVolume;
    BOOL  positionSliderPressed;
}

-(IBAction) play:      (id) sender;
-(IBAction) stop:      (id) sender;
-(IBAction) pause:     (id) sender;
-(IBAction) setInput:  (id) sender;
-(IBAction) seek:      (id) sender;
-(IBAction) setVolume: (id) sender;
-(IBAction) positionSliderWasPressed: (id) sender;

-(void) ackWasReceived: (BLT_Player_CommandId) command_id;
-(void) nackWasReceived: (BLT_Player_CommandId) command_id result: (BLT_Result) result;
-(void) decoderStateDidChange: (BLT_Player_DecoderState) state;
-(void) streamTimecodeDidChange: (BLT_TimeCode) timecode;
-(void) streamPositionDidChange: (BLT_StreamPosition) position;
-(void) streamInfoDidChange: (const BLT_StreamInfo*) info updateMask: (BLT_Mask) mask;
-(void) propertyDidChange: (BLT_PropertyScope) scope source: (const char*) source name: (const char*) name value: (const ATX_PropertyValue*) value;

@end
