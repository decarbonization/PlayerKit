//
//  PKAudioEffect.h
//  PlayerKit
//
//  Created by Peter MacWhinnie on 4/27/09.
//  Copyright 2009 Roundabout Software. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <AudioToolbox/AudioToolbox.h>

@class PKAudioPlayer;

/*!
 @class			PKAudioEffect
 @abstract		This class is used to represent effects in a PKAudioPlayer.
 @discussion	You never create an instance of this class directly, instead
				you get one from PKAudioPlayer that is already initialized.
 */
@interface PKAudioEffect : NSObject
{
	PKAudioPlayer *mAudioPlayer;
	AUNode mRepresentedNode;
}

#pragma mark Properties

/*!
 @property	representedComponent
 @abstract	The component description associated with the audio effect.
 */
@property (readonly) AudioComponentDescription representedComponent;

/*!
 @property	title
 @abstract	The title of the audio effect.
 */
@property (readonly) NSString *title;

/*!
 @property	audioUnit
 @abstract	The audio unit of the audio effect.
 */
@property (readonly) AudioUnit audioUnit;

/*!
 @property	state
 @abstract	The state of this audio effect.
 */
@property (copy) id state;

/*!
 @property	enabled
 @abstract	Whether or not this effect is enabled.
 */
@property BOOL enabled;

@end
