/*
 *  PKAudioEffect.h
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 10/16/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef PKAudioEffect_h
#define PKAudioEffect_h 1

#import <CoreFoundation/CoreFoundation.h>
#import <PlayerKit/PKAudioPlayer.h>

#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>

///The opaque reference type used to represent effects in PlayerKit.
typedef struct PKAudioEffect * PKAudioEffectRef;

#pragma mark Lifecycle

///Create an audio effect and insert it into the audio player from a specified audio component description.
PK_EXTERN PKAudioEffectRef PKAudioEffectCreate(AudioComponentDescription description, CFErrorRef *outError);

///Remove an audio effect from the audio player. Any references to `effect` are invalid after a call to this function.
PK_EXTERN Boolean PKAudioEffectRemove(PKAudioEffectRef effect, CFErrorRef *outError);

#pragma mark -
#pragma mark Properties

///Returns the title of the audio effect.
PK_EXTERN CFStringRef PKAudioEffectCopyTitle(PKAudioEffectRef effect);

#pragma mark -

///Set the class info of the audio effect.
PK_EXTERN Boolean PKAudioEffectSetClassInfo(PKAudioEffectRef effect, CFPropertyListRef classInfo, CFErrorRef *outError);

///Returns the class info if the audio effect.
PK_EXTERN CFPropertyListRef PKAudioEffectCopyClassInfo(PKAudioEffectRef effect);

#pragma mark -

///Sets whether or not an audio effect is enabled.
PK_EXTERN Boolean PKAudioEffectSetEnabled(PKAudioEffectRef effect, Boolean enabled, CFErrorRef *outError);

///Returns whether or not an audio effect is enabled.
PK_EXTERN Boolean PKAudioEffectIsEnabled(PKAudioEffectRef effect);

#pragma mark -
#pragma mark Audio Unit Properties

///Set a property on the effect's represented audio unit.
PK_EXTERN OSStatus PKAudioEffectSetProperty(PKAudioEffectRef effect, const void *inData, UInt32 inSize, AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, AudioUnitElement element);

///Copy a property's value from the effect's represented audio unit.
PK_EXTERN OSStatus PKAudioEffectCopyProperty(PKAudioEffectRef effect, void **outValue, UInt32 *ioSize, AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, AudioUnitElement element);

#pragma mark -
#pragma mark Audio Unit Parameters

///Set a parameter on the effect's represented audio unit.
PK_EXTERN OSStatus PKAudioEffectSetParameter(PKAudioEffectRef effect, AudioUnitParameterValue inData, AudioUnitParameterID inPropertyID, AudioUnitScope inScope, UInt32 inBufferOffsetInNumberOfFrames);

///Copy a parameter's value from the effect's represented audio unit.
PK_EXTERN OSStatus PKAudioEffectCopyParameter(PKAudioEffectRef effect, AudioUnitParameterValue *outValue, AudioUnitParameterID inPropertyID, AudioUnitScope inScope);

#pragma mark -

///Copy a parameter's info.
PK_EXTERN OSStatus PKAudioEffectCopyParameterInfo(PKAudioEffectRef effect, AudioUnitParameterID inPropertyID, AudioUnitParameterInfo *outInfo);

#endif /* PKAudioEffect_h */
