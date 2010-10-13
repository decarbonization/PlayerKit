//
//  PKAudioEffect.m
//  PlayerKit
//
//  Created by Peter MacWhinnie on 4/27/09.
//  Copyright 2009 Roundabout Software. All rights reserved.
//

#import "PKAudioEffect.h"
#import "PKAudioEffectPrivate.h"

#import "PKAudioPlayer.h"
#import "PKAudioPlayerPrivate.h"
#import "PKAudioPlayerEngine.h"

#import "CAComponent.h"
#import "CAComponentDescription.h"

@implementation PKAudioEffect

#pragma mark Construction

- (id)initWithAudioPlayer:(PKAudioPlayer *)audioPlayer node:(AUNode)node
{
	NSParameterAssert(audioPlayer);
	NSParameterAssert(node);
	
	if((self = [super init]))
	{
		mAudioPlayer = audioPlayer;
		mRepresentedNode = node;
		
		return self;
	}
	return nil;
}

#pragma mark -
#pragma mark Properties

@synthesize representedNode = mRepresentedNode;

- (AudioUnit)audioUnit
{
	try
	{
		return mAudioPlayer.audioPlayerEngine->GetAudioUnitForNode(mRepresentedNode);
	}
	catch (RBException e)
	{
		@throw RBExceptionToNSException(e);
	}
	
	return NULL;
}

- (AudioComponentDescription)representedComponent
{
	AudioComponentDescription componentDescription;
	try
	{
		componentDescription = mAudioPlayer.audioPlayerEngine->GetComponentDescriptionForNode(mRepresentedNode);
	}
	catch (RBException e)
	{
		@throw RBExceptionToNSException(e);
	}
	return componentDescription;
}

#pragma mark -

- (NSString *)title
{
	CFStringRef componentName = NULL;
	
	OSStatus error = AudioComponentCopyName(AudioComponentInstanceGetComponent(self.audioUnit), 
											&componentName);
	NSAssert1((error == noErr), @"Could not copy the name of audio unit. Error %d.", error);
	
	return [NSMakeCollectable(componentName) autorelease];
}

- (void)setState:(id)state
{
	try
	{
		mAudioPlayer.audioPlayerEngine->SetPropertyValue(state? &state : NULL, //in value
														 state? sizeof(state) : 0, //in valueSize
														 kAudioUnitProperty_ClassInfo, //in propertyID
														 kAudioUnitScope_Global, //in propertyScope
														 mRepresentedNode); //in node
	}
	catch (RBException e)
	{
		@throw RBExceptionToNSException(e);
	}
}

- (id)state
{
	id state = nil;
	
	try
	{
		UInt32 dataSize = sizeof(state);
		mAudioPlayer.audioPlayerEngine->CopyPropertyValue((void **)&state, //out value
														  &dataSize, //inout valueSize
														  kAudioUnitProperty_ClassInfo, //in propertyID
														  kAudioUnitScope_Global, //in propertyScope
														  mRepresentedNode); //in node
	}
	catch (RBException e)
	{
		@throw RBExceptionToNSException(e);
	}
	
	return [NSMakeCollectable(state) autorelease];
}

- (void)setEnabled:(BOOL)enabled
{
	try
	{
		UInt32 bypassEffect = !enabled;
		mAudioPlayer.audioPlayerEngine->SetPropertyValue(&bypassEffect, //in value
														 sizeof(bypassEffect), //in valueSize
														 kAudioUnitProperty_BypassEffect, //in propertyID
														 kAudioUnitScope_Global, //in propertyScope
														 mRepresentedNode); //in node
	}
	catch (RBException e)
	{
		@throw RBExceptionToNSException(e);
	}
}

- (BOOL)enabled
{
	UInt32 enabled = 0;
	
	try
	{
		UInt32 dataSize = sizeof(UInt32);
		mAudioPlayer.audioPlayerEngine->CopyPropertyValue((void **)&enabled, //out value
														  &dataSize, //inout valueSize
														  kAudioUnitProperty_BypassEffect, //in propertyID
														  kAudioUnitScope_Global, //in propertyScope
														  mRepresentedNode); //in node
	}
	catch (RBException e)
	{
		@throw RBExceptionToNSException(e);
	}
	
	return !enabled;
}

@end
