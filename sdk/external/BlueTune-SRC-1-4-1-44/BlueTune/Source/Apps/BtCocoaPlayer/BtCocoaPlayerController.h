//
//  CocoaPlayerController.h
//  CocoaPlayer
//
//  Created by Gilles on 9/7/08.
//  Copyright 2008 Gilles Boccon-Gibod. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "BlueTune.h"

@interface CocoaPlayerController : NSObject 
{
    BLT_PlayerObject*          player;
    IBOutlet NSButton*         playButton;
    IBOutlet NSButton*         stopButton;
    IBOutlet NSButton*         pauseButton;
    IBOutlet NSButton*         fileButton;
    IBOutlet NSTextField*      playerState;
    IBOutlet NSTextField*      playerInput;
    IBOutlet NSTextField*      playerTimecode;
    IBOutlet NSSlider*         playerPosition;
    IBOutlet NSSlider*         playerVolume;
    IBOutlet NSTableView*      playerStreamInfoView;
    IBOutlet NSTableView*      playerPropertiesView;
}

-(IBAction) play:       (id) sender;
-(IBAction) stop:       (id) sender;
-(IBAction) pause:      (id) sender;
-(IBAction) seek:       (id) sender;
-(IBAction) setInput:   (id) sender;
-(IBAction) setVolume:  (id) sender;
-(IBAction) chooseFile: (id) sender;

-(void) ackWasReceived: (BLT_Player_CommandId) command_id;
-(void) nackWasReceived: (BLT_Player_CommandId) command_id result: (BLT_Result) result;
-(void) decoderStateDidChange: (BLT_Player_DecoderState) state;
-(void) streamTimecodeDidChange: (BLT_TimeCode) timecode;
-(void) streamPositionDidChange: (BLT_StreamPosition) position;
-(void) streamInfoDidChange: (const BLT_StreamInfo*) info updateMask: (BLT_Mask) mask;
-(void) propertyDidChange: (BLT_PropertyScope) scope source: (const char*) source name: (const char*) name value: (const ATX_PropertyValue*) value;

@end

/* custom slider that can detect when the user is dragging */
@interface CocoaPlayerSliderCell : NSSliderCell 
{
    BOOL userIsDraggingSlider;
}

-(BOOL) isUserDraggingSlider;
-(BOOL) startTrackingAt:(NSPoint)startPoint inView:(NSView *)controlView;
-(void) stopTracking:(NSPoint)lastPoint at:(NSPoint)stopPoint inView:(NSView *)controlView mouseIsUp:(BOOL)flag;

@end

@interface CocoaPlayerSlider : NSSlider
{
}
-(void)setDoubleValue:(double)aDouble;
-(BOOL)isUserDraggingSlider;

@end