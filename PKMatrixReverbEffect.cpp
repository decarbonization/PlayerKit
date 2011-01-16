/*
 *  PKMatrixReverbEffect.cpp
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 1/12/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "PKMatrixReverbEffect.h"
#include "CAComponentDescription.h"
#include "CoreAudioErrors.h"
#include <iostream>

#pragma mark Creation

PK_EXTERN PKMatrixReverbEffectRef PKMatrixReverbEffectCreate(CFErrorRef *outError)
{
	AudioComponentDescription matrixReverbDescription = {
		kAudioUnitType_Effect, 
		kAudioUnitSubType_MatrixReverb, 
		kAudioUnitManufacturer_Apple, 
	};
	
	return PKAudioEffectCreate(matrixReverbDescription, outError);
}

#pragma mark -
#pragma mark Properties

PK_EXTERN Boolean PKMatrixReverbEffectSetAmount(PKMatrixReverbEffectRef effect, AudioUnitParameterValue value, CFErrorRef *outError)
{
	OSStatus error = PKAudioEffectSetParameter(effect, 
											   value, 
											   kReverbParam_DryWetMix, 
											   kAudioUnitScope_Global, 
											   0);
	if(error != noErr)
	{
		if(outError) *outError = PKCopyError(PKEffectsErrorDomain, 
											 error, 
											 NULL, 
											 CFSTR("Could not reverb amount, Error %@ (%d)."), CoreAudioGetErrorName(error), error);
		return false;
	}
	
	return true;
}

PK_EXTERN AudioUnitParameterValue PKMatrixReverbEffectGetAmount(PKMatrixReverbEffectRef effect)
{
	AudioUnitParameterValue value = 0.0;
	OSStatus error = PKAudioEffectCopyParameter(effect, &value, kReverbParam_DryWetMix, kAudioUnitScope_Global);
	if(error != noErr)
		std::cerr << __PRETTY_FUNCTION__ << ": Could not get value of amount. Ignoring error " << error << "." << std::endl;
	
	return value;
}
