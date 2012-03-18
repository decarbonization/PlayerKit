/*
 *  PKAudioPlayerInternal.h
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 10/16/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#import "PKAudioPlayer.h"
#import <AudioToolbox/AudioToolbox.h>

#import "PKAudioPlayerEngine.h"
#import "PKDecoder.h"

#pragma mark Types

///The struct used to represent the internal state of the PKAudioPlayer.
typedef struct PKAudioPlayer {
	//Engine
	PKAudioPlayerEngine *engine;
	
	//Decoder
	PKDecoder *decoder;
	AudioConverterRef decoderConverter;
	AudioBufferList *decoderConverterBuffers;
	
	//State
	volatile int32_t isPaused;
	volatile int32_t hasBroadcastedPresence;
	volatile int32_t preserveExistingBuffersOnResume;
	CFStringRef sessionID;
	
	//Pulse
	dispatch_block_t mPulseHandler;
	dispatch_queue_t mPulseHandlerQueue;
} PKAudioPlayer;

///The singleton instance of the audio player state.
PK_EXTERN PK_VISIBILITY_HIDDEN PKAudioPlayer AudioPlayerState;

#pragma mark -
#pragma mark Tools

#define CFDICT(keys, values) CFDictionaryCreate(kCFAllocatorDefault, (const void *[])keys, (const void *[])values, sizeof((const void *[])keys) / sizeof(const void *), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks)

#define CHECK_STATE_INITIALIZED() ({ if(OSMemoryBarrier(), AudioPlayerStateInitCount == 0) RBAssert(0, CFSTR("Attempted use of PKAudioPlayer before PKAudioPlayerInit has been called.")); })

#pragma mark -
#pragma mark Controlling Playback

///Set the decoder for the audio player to use.
PK_EXTERN Boolean PKAudioPlayerSetDecoder(PKDecoder *decoder, CFErrorRef *outError);

///Get the decoder the audio player is currently using, if any.
PK_EXTERN PKDecoder *PKAudioPlayerGetDecoder();
