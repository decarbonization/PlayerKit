/*
 *  PKGraphicEQEffect.cpp
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 10/16/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "PKGraphicEQEffect.h"
#include "CAComponentDescription.h"
#include "CoreAudioErrors.h"
#include <iostream>

#pragma mark Creation

PK_EXTERN PKGraphicEQEffectRef PKGraphicEQEffectCreate(CFErrorRef *outError)
{
	AudioComponentDescription graphicEQDescription = {
		kAudioUnitType_Effect, 
		kAudioUnitSubType_GraphicEQ, 
		kAudioUnitManufacturer_Apple, 
	};
	
	return PKAudioEffectCreate(graphicEQDescription, outError);
}

#pragma mark -
#pragma mark Properties

PK_EXTERN Boolean PKGraphicEQEffectSetHas32Bands(PKGraphicEQEffectRef effect, Boolean has32Bands, CFErrorRef *outError)
{
	OSStatus error = PKAudioEffectSetParameter(effect, 
											   has32Bands? 32 : 10, 
											   kGraphicEQParam_NumberOfBands, 
											   kAudioUnitScope_Global, 
											   0);
	if(error != noErr)
	{
		if(outError) *outError = PKCopyError(PKEffectsErrorDomain, 
											 error, 
											 NULL, 
											 CFSTR("Could not modify number of bands of effect. Error %@ (%d)."), CoreAudioGetErrorName(error), error);
		
		return false;
	}
	
	return true;
}

PK_EXTERN Boolean PKGraphicEQEffectHas32Bands(PKGraphicEQEffectRef effect)
{
	AudioUnitParameterValue numberOfBands = 0;
	if(PKAudioEffectCopyParameter(effect, 
								  &numberOfBands, 
								  kGraphicEQParam_NumberOfBands, 
								  kAudioUnitScope_Global) != noErr)
	{
		return true;
	}
	
	return (numberOfBands == 32);
}

#pragma mark -

PK_EXTERN Boolean PKGraphicEQEffectSetValueOfBand(PKGraphicEQEffectRef effect, CFIndex bandIndex, AudioUnitParameterValue bandValue, CFErrorRef *outError)
{
	UInt32 numberOfBands = PKGraphicEQEffectHas32Bands(effect)? 32 : 10;
	
	if(bandIndex < 0 || bandIndex > numberOfBands)
	{
		if(outError) *outError = PKCopyError(PKEffectsErrorDomain, 
											 -50 /*paramErr*/, 
											 NULL, 
											 CFSTR("Band %ld is out of range {0, %ld}"), bandIndex, numberOfBands);
		return false;
	}
	
	OSStatus error = PKAudioEffectSetParameter(effect, 
											   bandValue, 
											   bandIndex, 
											   kAudioUnitScope_Global, 
											   0);
	if(error != noErr)
	{
		if(outError) *outError = PKCopyError(PKEffectsErrorDomain, 
											 error, 
											 NULL, 
											 CFSTR("Could not set band %ld, Error %@ (%d)."), CoreAudioGetErrorName(error), error);
		return false;
	}
	
	return true;
}

PK_EXTERN AudioUnitParameterValue PKGraphicEQEffectGetValueOfBand(PKGraphicEQEffectRef effect, CFIndex bandIndex)
{
	AudioUnitParameterValue value = 0.0;
	OSStatus error = PKAudioEffectCopyParameter(effect, &value, bandIndex, kAudioUnitScope_Global);
	if(error != noErr)
	{
		value = 0.0;
		std::cerr << "Could not get value of band " << bandIndex << ". Ignoring error " << error << "." << std::endl;
	}
	
	return value;
}

PK_EXTERN void PKGraphicEQEffectIterateBands(PKGraphicEQEffectRef effect, void(^iterator)(AudioUnitParameterValue value, CFIndex index, Boolean *stop))
{
	if(!effect)
		return;
	
	CFIndex numberOfBands = PKGraphicEQEffectHas32Bands(effect)? 32 : 10;
	Boolean stop = false;
	for (CFIndex index = 0; index < numberOfBands; index++)
	{
		AudioUnitParameterValue value = PKGraphicEQEffectGetValueOfBand(effect, index);
		iterator(value, index, &stop);
		
		if(stop)
			break;
	}
}
