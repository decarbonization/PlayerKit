/*
 *  PKMatrixReverbEffect.h
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 1/12/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef PKMatrixReverbEffect_h
#define PKMatrixReverbEffect_h 1

#import <PlayerKit/PKAudioEffect.h>

///The opaque type used to represent PKGraphicEQEffectRef. Typed alias for PKAudioEffectRef.
typedef PKAudioEffectRef PKMatrixReverbEffectRef;

#pragma mark -
#pragma mark Creation

///Create a Matrix Reverb effect.
PK_EXTERN PKMatrixReverbEffectRef PKMatrixReverbEffectCreate(CFErrorRef *outError);

#pragma mark -
#pragma mark Properties

PK_EXTERN OSStatus PKMatrixReverbEffectSetDryWetMix(PKMatrixReverbEffectRef effect, AudioUnitParameterValue value); 
PK_EXTERN AudioUnitParameterValue PKMatrixReverbEffectGetDryWetMix(PKMatrixReverbEffectRef effect);

PK_EXTERN OSStatus PKMatrixReverbEffectSetSmallLargeMix(PKMatrixReverbEffectRef effect, AudioUnitParameterValue value); 
PK_EXTERN AudioUnitParameterValue PKMatrixReverbEffectGetSmallLargeMix(PKMatrixReverbEffectRef effect);

#pragma mark -

PK_EXTERN OSStatus PKMatrixReverbEffectSetSmallSize(PKMatrixReverbEffectRef effect, AudioUnitParameterValue value); 
PK_EXTERN AudioUnitParameterValue PKMatrixReverbEffectGetSmallSize(PKMatrixReverbEffectRef effect);

PK_EXTERN OSStatus PKMatrixReverbEffectSetLargeSize(PKMatrixReverbEffectRef effect, AudioUnitParameterValue value); 
PK_EXTERN AudioUnitParameterValue PKMatrixReverbEffectGetLargeSize(PKMatrixReverbEffectRef effect);

#pragma mark -

PK_EXTERN OSStatus PKMatrixReverbEffectSetPreDelay(PKMatrixReverbEffectRef effect, AudioUnitParameterValue value); 
PK_EXTERN AudioUnitParameterValue PKMatrixReverbEffectGetPreDelay(PKMatrixReverbEffectRef effect);

PK_EXTERN OSStatus PKMatrixReverbEffectSetLargeDelay(PKMatrixReverbEffectRef effect, AudioUnitParameterValue value); 
PK_EXTERN AudioUnitParameterValue PKMatrixReverbEffectGetLargeDelay(PKMatrixReverbEffectRef effect);

#pragma mark -

PK_EXTERN OSStatus PKMatrixReverbEffectSetSmallDensity(PKMatrixReverbEffectRef effect, AudioUnitParameterValue value); 
PK_EXTERN AudioUnitParameterValue PKMatrixReverbEffectGetSmallDensity(PKMatrixReverbEffectRef effect);

PK_EXTERN OSStatus PKMatrixReverbEffectSetLargeDensity(PKMatrixReverbEffectRef effect, AudioUnitParameterValue value); 
PK_EXTERN AudioUnitParameterValue PKMatrixReverbEffectGetLargeDensity(PKMatrixReverbEffectRef effect);

#pragma mark -

PK_EXTERN OSStatus PKMatrixReverbEffectSetLargeDelayRange(PKMatrixReverbEffectRef effect, AudioUnitParameterValue value); 
PK_EXTERN AudioUnitParameterValue PKMatrixReverbEffectGetLargeDelayRange(PKMatrixReverbEffectRef effect);

PK_EXTERN OSStatus PKMatrixReverbEffectSetSmallDelayRange(PKMatrixReverbEffectRef effect, AudioUnitParameterValue value); 
PK_EXTERN AudioUnitParameterValue PKMatrixReverbEffectGetSmallDelayRange(PKMatrixReverbEffectRef effect);

#pragma mark -

PK_EXTERN OSStatus PKMatrixReverbEffectSetLargeBrightness(PKMatrixReverbEffectRef effect, AudioUnitParameterValue value); 
PK_EXTERN AudioUnitParameterValue PKMatrixReverbEffectGetLargeBrightness(PKMatrixReverbEffectRef effect);

PK_EXTERN OSStatus PKMatrixReverbEffectSetSmallBrightness(PKMatrixReverbEffectRef effect, AudioUnitParameterValue value); 
PK_EXTERN AudioUnitParameterValue PKMatrixReverbEffectGetSmallBrightness(PKMatrixReverbEffectRef effect);

#pragma mark -

PK_EXTERN OSStatus PKMatrixReverbEffectSetModulationRate(PKMatrixReverbEffectRef effect, AudioUnitParameterValue value); 
PK_EXTERN AudioUnitParameterValue PKMatrixReverbEffectGetModulationRate(PKMatrixReverbEffectRef effect);

PK_EXTERN OSStatus PKMatrixReverbEffectSetModulationDepth(PKMatrixReverbEffectRef effect, AudioUnitParameterValue value); 
PK_EXTERN AudioUnitParameterValue PKMatrixReverbEffectGetModulationDepth(PKMatrixReverbEffectRef effect);

#pragma mark -

PK_EXTERN OSStatus PKMatrixReverbEffectSetFilterFrequency(PKMatrixReverbEffectRef effect, AudioUnitParameterValue value); 
PK_EXTERN AudioUnitParameterValue PKMatrixReverbEffectGetFilterFrequency(PKMatrixReverbEffectRef effect);

PK_EXTERN OSStatus PKMatrixReverbEffectSetFilterBandwidth(PKMatrixReverbEffectRef effect, AudioUnitParameterValue value); 
PK_EXTERN AudioUnitParameterValue PKMatrixReverbEffectGetFilterBandwidth(PKMatrixReverbEffectRef effect);

#pragma mark -

PK_EXTERN OSStatus PKMatrixReverbEffectSetFilterGain(PKMatrixReverbEffectRef effect, AudioUnitParameterValue value); 
PK_EXTERN AudioUnitParameterValue PKMatrixReverbEffectGetFilterGain(PKMatrixReverbEffectRef effect);

#endif /* PKMatrixReverbEffect_h */