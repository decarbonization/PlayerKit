/*
 *  PKAudioEffectPrivate.h
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 9/22/09.
 *  Copyright 2009 Roundabout Software. All rights reserved.
 *
 */

#import <Cocoa/Cocoa.h>

/*!
 @abstract	The private interface continuation for PKAudioEffect.
 */
@interface PKAudioEffect () //Continuation

#pragma mark Initialization

/*!
 @method
 @abstract	Initialize an audio effect with an owning audio player and an audio unit graph node.
 @param		audioPlayer		The audio player this effect belongs to. May not be nil.
 @param		node			The node this effect represents in the audio player's audio unit graph.
 @result	A fully initialized audio effect object. May be nil.
 */
- (id)initWithAudioPlayer:(PKAudioPlayer *)audioPlayer node:(AUNode)node;

#pragma mark -
#pragma mark Properties

/*!
 @property	representedNode
 @abstract	The node this audio effect object represents in the audio graph.
 */
@property (readonly) AUNode representedNode;

@end
