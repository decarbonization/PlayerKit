//
//  PKAudioPlayer.h
//  PlayerKit
//
//  Created by Peter MacWhinnie on 11/16/08.
//  Copyright 2008 Roundabout Software. All rights reserved.
//

//External Dependencies
#import <Cocoa/Cocoa.h>
#import <AudioToolbox/AudioToolbox.h> //for AudioConverterRef, AudioBufferList
#import <objc/message.h> //for Method.

@class PKAudioEffect, PKAvailableAudioEffect;
@protocol PKAudioPlayerDelegate;

/*!
 @abstract	The PKAudioPlayer class represents the public interface for PlayerKit's decoding and playback functionality.
 */
@interface PKAudioPlayer : NSObject
{
	/* owner */	struct PKAudioPlayerEngine *mPlaybackEngine;
	/* owner */	NSMutableArray *mAudioEffects;
	
	/* owner */	struct PKDecoder *mPlaybackDecoder;
	
	/* owner */	AudioConverterRef mDecoderDataConverter;
	/* owner */	AudioBufferList *mDecoderDataConverterBufferList;
	/* n/a */	Method mPlaybackDecoderFillBufferMethod;
	
	/* n/a */	BOOL mIsPlaybackPaused;
	/* weak */	id < PKAudioPlayerDelegate > mDelegate;
}

#pragma mark Properties

/*!
 @abstract	The delegate of the audio player. @seealso #PKAudioPlayerDelegate PKAudioPlayerDelegate
 */
@property (assign) id < PKAudioPlayerDelegate > delegate;

/*!
 @abstract	The amount of CPU the audio player is using per cycle.
 */
@property (readonly) float averageCPUUsage;

/*!
 @abstract	The volume of the audio player.
 */
@property float volume;

#pragma mark -

/*!
 @abstract	The duration of the audio player's decoder in seconds.
 */
@property (readonly) NSTimeInterval duration;

/*!
 @abstract	The current location (in seconds) of the receiver's decoder in its data source.
 */
@property NSTimeInterval currentTime;

#pragma mark -
#pragma mark Playback Control

/*!
 @abstract		Load a specified resource into the receiver for playback.
 @param			location	The location of the resource to load up for playback. Optional.
 @param			error		An error will be provided upon return if anything goes wrong. Optional.
 @result		YES if the receiver could load the specified resource; NO otherwise.
 @discussion	The built in decoders of PlayerKit only support local resources.
 */
- (BOOL)setLocation:(NSURL *)location error:(NSError **)error;

/*!
 @abstract	Returns the location of the resource the receiver is currently playing.
 */
- (NSURL *)location;

#pragma mark -

/*!
 @abstract		Initiates audio playback.
 @discussion	This method raises an exception upon failure to initiate playback.
				
				This method resumes playback if the receiver is paused.
 */
- (void)play;

/*!
 @abstract		Pauses audio playback.
 @discussion	This method raises an exception upon failure to pause playback.
 */
- (void)pause;

/*!
 @abstract		Terminates audio playback.
 @discussion	This method raises an exception upon failure to initiate playback.
				
				This method resets the receiver's pause state.
 */
- (void)stop;

/*!
 @abstract	Whether or not the audio player is playing.
 */
- (BOOL)isPlaying;


/*!
 @abstract   Indicates whether the receiver is paused.
 */
- (BOOL)isPaused;

/*!
 @abstract		Pauses audio playback.
 @discussion	This method raises an exception upon failure to pause playback.
 */
- (void)pause;

/*!
 @abstract		Resumes audio playback.
 @discussion	This method raises an exception upom failure to resume playback.
				
				This method will call -[PKAudioPlayer play] if the receiver is not paused.
 */
- (void)resume;

#pragma mark -
#pragma mark Effects

/*!
 @abstract	Remove the specified effect from the audio players effect graph.
 */
- (BOOL)removeEffect:(PKAudioEffect *)effect error:(NSError **)error;

/*!
 @abstract	Remove all effects from the audio graph.
 */
- (BOOL)removeAllEffects:(NSError **)error;

/*!
 @abstract	The effects in the audio players AUGraph.
 */
@property (readonly) NSArray *effects;

#if 0
#pragma mark -
#pragma mark Effects Loading and Archiving

/*!
 @abstract	Get a serializable representation of the receivers effects.
 */
- (NSArray *)arrayRepresentationOfEffects;

/*!
 @abstract  Load the effects from an array of effect-dictionaries.
 */
- (BOOL)loadEffectsFromArrayRepresentation:(NSArray *)effects error:(NSError **)error;

/*!
 @abstract	Replace the current effects with a new set of effects.
 */
- (BOOL)replaceEffectsWithEffectsArrayRepresentation:(NSArray *)effects error:(NSError **)error;
#endif
@end

/*!
 @protocol	PKAudioPlayerDelegate
 @abstract	This delegate describes the required and optional methods
			used in PKAudioPlayers delegate.
 */
@protocol PKAudioPlayerDelegate < NSObject >
@required

/*!
 @abstract		This method is called when an audio player encounters a non-fatal error.
 @discussion	Truly fatal errors are thrown as exceptions. Although errors passed
 to this delegate method are non-fatal, it is very likely they've
 stopped playback.
 */
- (void)audioPlayer:(PKAudioPlayer *)audioPlayer didEncounterError:(NSError *)error;

@optional

/*!
 @abstract	This method is called when an audio player stops playback.
 @param		audioPlayer	The audio player that called this method
 @param		didFinish	Whether or not playback progressed to the end.
 */
- (void)audioPlayer:(PKAudioPlayer *)audioPlayer didFinishPlaying:(BOOL)didFinish;
@end
