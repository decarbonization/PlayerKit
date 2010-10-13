/*
 *  PKAudioPlayerPrivate.h
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 9/22/09.
 *  Copyright 2009 Roundabout Software. All rights reserved.
 *
 */

#import <Cocoa/Cocoa.h>
#import "PKDecoder.h"

/*!
 @abstract	The private interface continuation for PKAudioPlayer.
 */
@interface PKAudioPlayer () //Continuation

#pragma mark Internal

/*!
 @abstract	The internal engine of the audio player.
 */
@property (readonly) PKAudioPlayerEngine *audioPlayerEngine;

#pragma mark -
#pragma mark Decoding

/*!
 @abstract		Sets the decoder object on the receiver.
 @param			decoder	A decoder object for the receiver to acquire audio data from. Optional.
 @param			error	An error will be provided upon return if anything goes wrong. Optional.
 @result		YES if the decoder can be used and was loaded into place successfully; NO otherwise.
 @discussion	Pass in NULL for `decoder` to reset the receiver's internal state.
 */
- (BOOL)setDecoder:(PKDecoder *)decoder error:(NSError **)error;

/*!
 @abstract	Returns the decoder the receiver is currently using, if any.
 */
- (PKDecoder *)decoder;

#pragma mark -
#pragma mark Effects

/*!
 @abstract	Add a new effect node to the receiver's internal audio player engine.
 @param		inDescription	A pointer to a AudioComponentDescription value describing the effect to add. May not be nil.
 @param		error			If the effect cannot be added to the audio player engine on return this will contain an NSError instance.
 @result	A PKAudioEffect if the effect node could be added; nil otherwise.
 */
- (PKAudioEffect *)addEffectWithComponentDescription:(const AudioComponentDescription *)inDescription error:(NSError **)error;

@end
