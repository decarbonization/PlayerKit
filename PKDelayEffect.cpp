/*
 *  PKDelayEffect.cpp
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 1/14/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "PKDelayEffect.h"
#include "CoreAudioErrors.h"
#include <iostream>

#pragma mark Creation

PK_EXTERN PKDelayEffectRef PKDelayEffectCreate(CFErrorRef *outError)
{
	AudioComponentDescription delayDescription = {
		kAudioUnitType_Effect, 
		kAudioUnitSubType_Delay, 
		kAudioUnitManufacturer_Apple, 
	};
	
	return PKAudioEffectCreate(delayDescription, outError);
}

#pragma mark -
#pragma mark Properties

PK_EXTERN Boolean PKDelayEffectSetAmount(PKDelayEffectRef effect, AudioUnitParameterValue value, CFErrorRef *outError)
{
	OSStatus error = PKAudioEffectSetParameter(effect, 
											   value, 
											   kDelayParam_WetDryMix, 
											   kAudioUnitScope_Global, 
											   0);
	if(error != noErr)
	{
		if(outError) *outError = PKCopyError(PKEffectsErrorDomain, 
											 error, 
											 NULL, 
											 CFSTR("Could not set delay amount, Error %@ (%d)."), CoreAudioGetErrorName(error), error);
		return false;
	}
	
	return true;
}

PK_EXTERN AudioUnitParameterValue PKDelayEffectGetAmount(PKDelayEffectRef effect)
{
	AudioUnitParameterValue value = 0.0;
	OSStatus error = PKAudioEffectCopyParameter(effect, &value, kDelayParam_WetDryMix, kAudioUnitScope_Global);
	if(error != noErr)
		std::cerr << __PRETTY_FUNCTION__ << ": Could not get value of amount. Ignoring error " << error << "." << std::endl;
	
	return value;
}

#pragma mark -

PK_EXTERN Boolean PKDelayEffectSetDelayTime(PKDelayEffectRef effect, AudioUnitParameterValue value, CFErrorRef *outError)
{
	OSStatus error = PKAudioEffectSetParameter(effect, 
											   value, 
											   kDelayParam_DelayTime, 
											   kAudioUnitScope_Global, 
											   0);
	if(error != noErr)
	{
		if(outError) *outError = PKCopyError(PKEffectsErrorDomain, 
											 error, 
											 NULL, 
											 CFSTR("Could not set delay time, Error %@ (%d)."), CoreAudioGetErrorName(error), error);
		return false;
	}
	
	return true;
}

PK_EXTERN AudioUnitParameterValue PKDelayEffectGetDelayTime(PKDelayEffectRef effect)
{
	AudioUnitParameterValue value = 0.0;
	OSStatus error = PKAudioEffectCopyParameter(effect, &value, kDelayParam_DelayTime, kAudioUnitScope_Global);
	if(error != noErr)
		std::cerr << __PRETTY_FUNCTION__ << ": Could not get value of delay time. Ignoring error " << error << "." << std::endl;
	
	return value;
}

#pragma mark -

PK_EXTERN Boolean PKDelayEffectSetFeedback(PKDelayEffectRef effect, AudioUnitParameterValue value, CFErrorRef *outError)
{
	OSStatus error = PKAudioEffectSetParameter(effect, 
											   value, 
											   kDelayParam_Feedback, 
											   kAudioUnitScope_Global, 
											   0);
	if(error != noErr)
	{
		if(outError) *outError = PKCopyError(PKEffectsErrorDomain, 
											 error, 
											 NULL, 
											 CFSTR("Could not set delay feedback amount, Error %@ (%d)."), CoreAudioGetErrorName(error), error);
		return false;
	}
	
	return true;
}

PK_EXTERN AudioUnitParameterValue PKDelayEffectGetFeedback(PKDelayEffectRef effect)
{
	AudioUnitParameterValue value = 0.0;
	OSStatus error = PKAudioEffectCopyParameter(effect, &value, kDelayParam_Feedback, kAudioUnitScope_Global);
	if(error != noErr)
		std::cerr << __PRETTY_FUNCTION__ << ": Could not get value of feedback. Ignoring error " << error << "." << std::endl;
	
	return value;
}

#pragma mark -

PK_EXTERN Boolean PKDelayEffectSetLopassCutoff(PKDelayEffectRef effect, AudioUnitParameterValue value, CFErrorRef *outError)
{
	OSStatus error = PKAudioEffectSetParameter(effect, 
											   value, 
											   kDelayParam_LopassCutoff, 
											   kAudioUnitScope_Global, 
											   0);
	if(error != noErr)
	{
		if(outError) *outError = PKCopyError(PKEffectsErrorDomain, 
											 error, 
											 NULL, 
											 CFSTR("Could not set delay lopass cutoff, Error %@ (%d)."), CoreAudioGetErrorName(error), error);
		return false;
	}
	
	return true;
}

PK_EXTERN AudioUnitParameterValue PKDelayEffectGetLopassCutoff(PKDelayEffectRef effect)
{
	AudioUnitParameterValue value = 0.0;
	OSStatus error = PKAudioEffectCopyParameter(effect, &value, kDelayParam_LopassCutoff, kAudioUnitScope_Global);
	if(error != noErr)
		std::cerr << __PRETTY_FUNCTION__ << ": Could not get value of lopass cutoff. Ignoring error " << error << "." << std::endl;
	
	return value;
}
