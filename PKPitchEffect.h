/*
 *  PKPitchEffect.h
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 1/14/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef PKPitchEffect_h
#define PKPitchEffect_h 1

#import <PlayerKit/PKAudioEffect.h>

///The opaque type used to represent PKPitchEffect. Typed alias for PKAudioEffectRef.
typedef PKAudioEffectRef PKPitchEffectRef;

#pragma mark -
#pragma mark Creation

///Create a Pitch effect.
PK_EXTERN PKPitchEffectRef PKPitchEffectCreate(CFErrorRef *outError);

#pragma mark -
#pragma mark Properties

///Sets the pitch.
PK_EXTERN Boolean PKPitchEffectSetPitch(PKPitchEffectRef effect, AudioUnitParameterValue value, CFErrorRef *outError);

///Gets the pitch.
PK_EXTERN AudioUnitParameterValue PKPitchEffectGetPitch(PKPitchEffectRef effect);

#pragma mark -

///Sets the blend amount.
PK_EXTERN Boolean PKPitchEffectSetBlendAmount(PKPitchEffectRef effect, AudioUnitParameterValue value, CFErrorRef *outError);

///Gets the blend amount.
PK_EXTERN AudioUnitParameterValue PKPitchEffectGetBlendAmount(PKPitchEffectRef effect);

#endif /* PKPitchEffect_h */