/*
 *  PKPitchEffect.cpp
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 1/14/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "PKPitchEffect.h"
#include "CoreAudioErrors.h"
#include <iostream>

#pragma mark -
#pragma mark Creation

PK_EXTERN PKPitchEffectRef PKPitchEffectCreate(CFErrorRef *outError)
{
	AudioComponentDescription pitchDescription = {
		kAudioUnitType_Effect, 
		kAudioUnitSubType_Pitch, 
		kAudioUnitManufacturer_Apple, 
	};
	
	return PKAudioEffectCreate(pitchDescription, outError);
}

#pragma mark -
#pragma mark Properties

PK_EXTERN Boolean PKPitchEffectSetPitch(PKPitchEffectRef effect, AudioUnitParameterValue value, CFErrorRef *outError)
{
	OSStatus error = PKAudioEffectSetParameter(effect, 
											   value, 
											   kTimePitchParam_Pitch, 
											   kAudioUnitScope_Global, 
											   0);
	if(error != noErr)
	{
		if(outError) *outError = PKCopyError(PKEffectsErrorDomain, 
											 error, 
											 NULL, 
											 CFSTR("Could not set pitch, Error %@ (%d)."), CoreAudioGetErrorName(error), error);
		return false;
	}
	
	return true;
}

PK_EXTERN AudioUnitParameterValue PKPitchEffectGetPitch(PKPitchEffectRef effect)
{
	AudioUnitParameterValue value = 0.0;
	OSStatus error = PKAudioEffectCopyParameter(effect, &value, kTimePitchParam_Pitch, kAudioUnitScope_Global);
	if(error != noErr)
		std::cerr << __PRETTY_FUNCTION__ << ": Could not get value of pitch. Ignoring error " << error << "." << std::endl;
	
	return value;
}

#pragma mark -

PK_EXTERN Boolean PKPitchEffectSetBlendAmount(PKPitchEffectRef effect, AudioUnitParameterValue value, CFErrorRef *outError)
{
	OSStatus error = PKAudioEffectSetParameter(effect, 
											   value, 
											   kTimePitchParam_EffectBlend, 
											   kAudioUnitScope_Global, 
											   0);
	if(error != noErr)
	{
		if(outError) *outError = PKCopyError(PKEffectsErrorDomain, 
											 error, 
											 NULL, 
											 CFSTR("Could not set pitch blend amount, Error %@ (%d)."), CoreAudioGetErrorName(error), error);
		return false;
	}
	
	return true;
}

PK_EXTERN AudioUnitParameterValue PKPitchEffectGetBlendAmount(PKPitchEffectRef effect)
{
	AudioUnitParameterValue value = 0.0;
	OSStatus error = PKAudioEffectCopyParameter(effect, &value, kTimePitchParam_EffectBlend, kAudioUnitScope_Global);
	if(error != noErr)
		std::cerr << __PRETTY_FUNCTION__ << ": Could not get value of pitch blemd amount. Ignoring error " << error << "." << std::endl;
	
	return value;
}
