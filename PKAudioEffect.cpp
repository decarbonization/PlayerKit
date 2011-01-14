/*
 *  PKAudioEffect.cpp
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 10/16/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#import "PKAudioEffect.h"
#import "PKAudioPlayerInternal.h"

struct PKAudioEffect
{
	AUNode node;
	PKAudioPlayerEngine *engine;
};

#pragma mark Lifecycle

PK_EXTERN PKAudioEffectRef PKAudioEffectCreate(AudioComponentDescription description, CFErrorRef *outError)
{
	AUNode node = 0;
	try
	{
		node = AudioPlayerState.engine->AddNode(&description);
	}
	catch (RBException e)
	{
		if(outError) *outError = e.CopyError();
		
		return NULL;
	}
	
	PKAudioEffect *effect = new PKAudioEffect;
	effect->node = node;
	effect->engine = AudioPlayerState.engine;
	effect->engine->Retain();
	
	return effect;
}

PK_EXTERN Boolean PKAudioEffectRemove(PKAudioEffectRef effect, CFErrorRef *outError)
{
	try
	{
		RBParameterAssert(effect);
		
		effect->engine->RemoveNode(effect->node);
		effect->engine->Release();
		
		delete effect;
	}
	catch (RBException e)
	{
		if(outError) *outError = e.CopyError();
		
		delete effect;
		
		return false;
	}
	
	return true;
}

#pragma mark -
#pragma mark Properties

PK_EXTERN CFStringRef PKAudioEffectCopyTitle(PKAudioEffectRef effect)
{
	if(effect)
	{
		try
		{
			return effect->engine->CopyTitleForNode(effect->node);
		}
		catch (RBException e)
		{
			//Ignore it.
		}
	}
	
	return NULL;
}

#pragma mark -

PK_EXTERN Boolean PKAudioEffectSetClassInfo(PKAudioEffectRef effect, CFPropertyListRef classInfo, CFErrorRef *outError)
{
	try
	{
		RBParameterAssert(effect);
		
		OSStatus error = PKAudioEffectSetProperty(effect, 
												  &classInfo, 
												  sizeof(classInfo), 
												  kAudioUnitProperty_ClassInfo, 
												  kAudioUnitScope_Global,
												  0);
		if(error != noErr)
		{
			if(outError) *outError = PKCopyError(PKEffectsErrorDomain, 
												 error, 
												 NULL, 
												 CFSTR("Could not set class data. Error code %ld."), error);
			
			return false;
		}
	}
	catch (RBException e)
	{
		if(outError) *outError = e.CopyError();
		
		return false;
	}
	
	return true;
}

PK_EXTERN CFPropertyListRef PKAudioEffectCopyClassInfo(PKAudioEffectRef effect)
{
	CFPropertyListRef classInfo = NULL;
	UInt32 classInfoSize = sizeof(classInfo);
	if(PKAudioEffectCopyProperty(effect, (void **)&classInfo, &classInfoSize, kAudioUnitProperty_ClassInfo, kAudioUnitScope_Global, 0) == noErr)
		return classInfo;
	
	return NULL;
}

#pragma mark -

PK_EXTERN Boolean PKAudioEffectSetEnabled(PKAudioEffectRef effect, Boolean enabled, CFErrorRef *outError)
{
	UInt32 bypass = !enabled;
	OSStatus error = PKAudioEffectSetProperty(effect, 
											  &bypass, 
											  sizeof(bypass), 
											  kAudioUnitProperty_BypassEffect, 
											  kAudioUnitScope_Global, 
											  0);
	if(error != noErr)
	{
		if(outError) *outError = PKCopyError(PKEffectsErrorDomain, 
											 error, 
											 NULL, 
											 CFSTR("Could not set enabled. Error code %ld."), error);
		
		return false;
	}
	
	return true;
}

PK_EXTERN Boolean PKAudioEffectIsEnabled(PKAudioEffectRef effect)
{
	UInt32 enabled = 0;
	UInt32 enabledSize = sizeof(enabled);
	PKAudioEffectCopyProperty(effect, 
							  (void **)&enabled, 
							  &enabledSize, 
							  kAudioUnitProperty_BypassEffect, 
							  kAudioUnitScope_Global, 
							  0);
	
	return enabled;
}

#pragma mark -
#pragma mark Audio Unit Properties

PK_EXTERN OSStatus PKAudioEffectSetProperty(PKAudioEffectRef effect, const void *inData, UInt32 inSize, AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, AudioUnitElement element)
{
	if(!effect)
		return -50 /* paramErr */;
	
	try
	{
		effect->engine->SetPropertyValue(inData, inSize, inPropertyID, inScope, effect->node, element);
	}
	catch (RBException e)
	{
		return e.GetCode();
	}
	
	return noErr;
}

PK_EXTERN OSStatus PKAudioEffectCopyProperty(PKAudioEffectRef effect, void **outValue, UInt32 *ioSize, AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, AudioUnitElement element)
{
	if(!effect)
		return -50 /* paramErr */;
	
	try
	{
		effect->engine->CopyPropertyValue(outValue, ioSize, inPropertyID, inScope, effect->node, element);
	}
	catch (RBException e)
	{
		return e.GetCode();
	}
	
	return noErr;
}

#pragma mark -
#pragma mark Audio Unit Parameters

PK_EXTERN OSStatus PKAudioEffectSetParameter(PKAudioEffectRef effect, AudioUnitParameterValue inData, AudioUnitParameterID inPropertyID, AudioUnitScope inScope, UInt32 inBufferOffsetInNumberOfFrames)
{
	if(!effect)
		return -50 /* paramErr */;
	
	try
	{
		effect->engine->SetParameterValue(inData, inPropertyID, inScope, effect->node, inBufferOffsetInNumberOfFrames);
	}
	catch (RBException e)
	{
		return e.GetCode();
	}
	
	return noErr;
}

PK_EXTERN OSStatus PKAudioEffectCopyParameter(PKAudioEffectRef effect, AudioUnitParameterValue *outValue, AudioUnitParameterID inPropertyID, AudioUnitScope inScope)
{
	if(!effect)
		return -50 /* paramErr */;
	
	try
	{
		effect->engine->CopyParameterValue(outValue, inPropertyID, inScope, effect->node);
	}
	catch (RBException e)
	{
		return e.GetCode();
	}
	
	return noErr;
}

#pragma mark -

PK_EXTERN OSStatus PKAudioEffectCopyParameterInfo(PKAudioEffectRef effect, AudioUnitParameterID inPropertyID, AudioUnitParameterInfo *outInfo)
{
	UInt32 outInfoSize = sizeof(*outInfo);
	return PKAudioEffectCopyProperty(effect, (void **)outInfo, &outInfoSize, kAudioUnitProperty_ParameterInfo, kAudioUnitScope_Global, inPropertyID);
}
