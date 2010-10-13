//
//  PKAudioPlayer.m
//  PlayerKit
//
//  Created by Peter MacWhinnie on 11/16/08.
//  Copyright 2008 Roundabout Software. All rights reserved.
//

#import "PKAudioPlayer.h"
#import "PKAudioPlayerPrivate.h"

#import <libKern/OSAtomic.h>
#import "PKDecoder.h"
#import "PKAudioPlayerEngine.h"

#import "CAStreamBasicDescription.h"
#import "CAAudioBufferList.h"

#import "PKAudioEffect.h"
#import "PKAudioEffectPrivate.h"

#pragma mark -

static void PKAudioPlayerDidEncounterError(PKAudioPlayerEngine *graph, CFErrorRef error, void *userData);
static void PKAudioPlayerDidFinishRunning(PKAudioPlayerEngine *graph, bool wasStopped, void *userData);

static UInt32 PKAudioPlayerScheduleSlice(PKAudioPlayerEngine *graph, AudioBufferList *ioBuffer, UInt32 numberOfSamples, CFErrorRef *error, void *userData);
static UInt32 PKAudioPlayerScheduleSliceWithConverter(PKAudioPlayerEngine *graph, AudioBufferList *ioBuffer, UInt32 numberOfSamples, CFErrorRef *error, void *userData);

@implementation PKAudioPlayer

#pragma mark Destruction

- (void)finalize
{
	if(mPlaybackEngine)
		delete mPlaybackEngine;
	
	if(mDecoderDataConverter)
	{
		AudioConverterDispose(mDecoderDataConverter);
		mDecoderDataConverter = nil;
	}
	
	if(mPlaybackDecoder)
	{
		mPlaybackDecoder->Release();
		mPlaybackDecoder = NULL;
	}
	
	[super finalize];
}

- (void)dealloc
{
	if(mPlaybackEngine)
		delete mPlaybackEngine;
	
	if(mDecoderDataConverter)
	{
		AudioConverterDispose(mDecoderDataConverter);
		mDecoderDataConverter = nil;
	}
	
	if(mPlaybackDecoder)
	{
		mPlaybackDecoder->Release();
		mPlaybackDecoder = NULL;
	}
	
	[mAudioEffects release];
	mAudioEffects = nil;
	
	[super dealloc];
}

#pragma mark -
#pragma mark Construction

- (id)init
{
	if((self = [super init]))
	{
		try
		{
			mPlaybackEngine = PKAudioPlayerEngine::New();
			
			//
			//	We create a __block copy of self to prevent a retain cycle
			//	as a result of the blocks below retaining it.
			//
			__block PKAudioPlayer *audioPlayer = self;
			mPlaybackEngine->SetErrorHandler(^(CFErrorRef error) {
				
				NSError *cocoaError = NSMakeCollectable(error);
				CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopCommonModes, ^{
					try
					{
						if(mPlaybackEngine->IsRunning())
							mPlaybackEngine->StopGraph();
						
						mPlaybackEngine->StopProcessing();
						
						mIsPlaybackPaused = NO;
						audioPlayer.currentTime = 0.0;
					}
					catch (RBException e)
					{
						NSError *error = [NSMakeCollectable(e.CopyError()) autorelease];
						NSLog(@"An exception was raised while resetting a PKAudioPlayer instance's "
							  @"internal state after an error occurred. Error: {%@}", error);
					}
					
					[mDelegate audioPlayer:audioPlayer didEncounterError:cocoaError];
					
					[cocoaError release];
				});
				
			});
			
			mPlaybackEngine->SetEndOfPlaybackHandler(^{
				
				CFRunLoopPerformBlock(CFRunLoopGetMain(), kCFRunLoopCommonModes, ^{
					if([audioPlayer.delegate respondsToSelector:@selector(audioPlayer:didFinishPlaying:)])
						[audioPlayer.delegate audioPlayer:audioPlayer didFinishPlaying:YES];
				});
				
			});
		}
		catch (RBException e)
		{
			@throw RBExceptionToNSException(e);
		}
		
		
		mAudioEffects = [NSMutableArray new];
		
		return self;
	}
	return nil;
}

#pragma mark -
#pragma mark Playback Callbacks

static UInt32 PKAudioPlayerScheduleSlice(PKAudioPlayerEngine *graph, AudioBufferList *ioBuffer, UInt32 numberOfFramesToRead, CFErrorRef *error, void *userData)
{
	PKAudioPlayer *self = (PKAudioPlayer *)userData;
	
	try
	{
		return self->mPlaybackDecoder->FillBuffers(ioBuffer, numberOfFramesToRead);
	}
	catch (RBException e)
	{
		if(error) *error = e.CopyError();
		
		return 0;
	}
}

struct PKAudioPlayerConverterState
{
	PKAudioPlayer *mAudioPlayer;
	CFErrorRef mError;
};

static OSStatus PKAudioPlayerConverterCallback(AudioConverterRef inAudioConverter, UInt32 *ioNumberDataPackets, AudioBufferList *ioData, AudioStreamPacketDescription **outDataPacketDescription, void *inUserData)
{
	PKAudioPlayerConverterState *sharedState = (PKAudioPlayerConverterState *)inUserData;
	PKAudioPlayer *self = sharedState->mAudioPlayer;
	
	for (int index = 0; index < self->mDecoderDataConverterBufferList->mNumberBuffers; index++)
		ioData->mBuffers[index] = self->mDecoderDataConverterBufferList->mBuffers[index];
	
	try
	{
		*ioNumberDataPackets = self->mPlaybackDecoder->FillBuffers(ioData, *ioNumberDataPackets);
		
		if(*ioNumberDataPackets == 0)
		{
			//This is necessary or AudioConverter will continue to call this function
			//producing garbage that the user then has to hear. We don't want that to happen.
			for (int index = 0; index < self->mDecoderDataConverterBufferList->mNumberBuffers; index++)
				ioData->mBuffers[index].mDataByteSize = 0;
		}
	}
	catch (RBException e)
	{
		sharedState->mError = e.CopyError();
		
		return CFErrorGetCode(sharedState->mError);
	}
	
	return noErr;
}

static UInt32 PKAudioPlayerScheduleSliceWithConverter(PKAudioPlayerEngine *graph, AudioBufferList *ioBuffer, UInt32 numberOfFramesToRead, CFErrorRef *error, void *userData)
{
	PKAudioPlayer *self = (PKAudioPlayer *)userData;
	
	UInt32 ioNumberOfFramesForConverter = numberOfFramesToRead;
	PKAudioPlayerConverterState converterState = { mAudioPlayer: self, mError: NULL };
	OSStatus errorCode = AudioConverterFillComplexBuffer(self->mDecoderDataConverter, //in audioConverter
														 PKAudioPlayerConverterCallback, //in inputDataProc
														 &converterState, //in userData
														 &ioNumberOfFramesForConverter, //io dataPacketSize
														 ioBuffer, //out outputData
														 NULL); //out packetDescription
	if(ioNumberOfFramesForConverter == 0)
	{
		if(errorCode != noErr)
			if(error) *error = converterState.mError;
		
		return 0;
	}
	
	return ioNumberOfFramesForConverter;
}

#pragma mark -
#pragma mark Properties

@synthesize delegate = mDelegate;
@synthesize audioPlayerEngine = mPlaybackEngine;

#pragma mark -

- (void)setVolume:(float)volume
{
	try
	{
		mPlaybackEngine->SetVolume(volume);
	}
	catch (RBException e)
	{
		@throw RBExceptionToNSException(e);
	}
}

- (float)volume
{
	try
	{
		return mPlaybackEngine->GetVolume();
	}
	catch (RBException e)
	{
		@throw RBExceptionToNSException(e);
	}
	
	return 0.0f;
}

- (float)averageCPUUsage
{
	return mPlaybackEngine->GetAverageCPUUsage();
}

#pragma mark -

- (NSTimeInterval)duration
{
	@synchronized(self)
	{
		if(mPlaybackDecoder)
			return mPlaybackDecoder->GetTotalNumberOfFrames() / mPlaybackDecoder->GetStreamFormat().mSampleRate;
	}
	
	return 0.0f;
}

- (void)setCurrentTime:(NSTimeInterval)newTime
{
	@synchronized(self)
	{
		if(mPlaybackDecoder && mPlaybackDecoder->CanSeek())
		{
			BOOL shouldStopAndRestartGraph = [self isPlaying];
			try
			{
				if(shouldStopAndRestartGraph)
				{
					mPlaybackEngine->StopGraph();
					
					mPlaybackEngine->Acquire();
					mPlaybackEngine->PauseProcessing();
				}
				
				mPlaybackDecoder->SetCurrentFrame(newTime * mPlaybackDecoder->GetStreamFormat().mSampleRate);
				
				if(shouldStopAndRestartGraph)
				{
					mPlaybackEngine->ResumeProcessing();
					mPlaybackEngine->Relinquish();
					
					mPlaybackEngine->StartGraph();
				}
			}
			catch (RBException e)
			{
				//
				//	We prevent deadlocks here by relinquishing our exclusive access to the audio player engine.
				//	We only relinquish if we should be starting and stopping the graph and there are
				//	one or more locks active on the playback engine.
				//
				if(shouldStopAndRestartGraph)
					mPlaybackEngine->Relinquish();
				
				@throw RBExceptionToNSException(e);
			}
		}
	}
}

- (NSTimeInterval)currentTime
{
	@synchronized(self)
	{
		if(mPlaybackDecoder)
			return mPlaybackDecoder->GetCurrentFrame() / mPlaybackDecoder->GetStreamFormat().mSampleRate;
	}
	
	return 0.0f;
}

#pragma mark -
#pragma mark Decoding

- (void)setupAudioConverterWithSourceFormat:(CAStreamBasicDescription *)sourceFormat resultFormat:(CAStreamBasicDescription *)resultFormat
{
	resultFormat->mSampleRate = kPKCanonicalSampleRate;
	resultFormat->SetCanonical(2, false);
	
	OSStatus errorCode = AudioConverterNew(sourceFormat, resultFormat, &mDecoderDataConverter);
	NSAssert1((errorCode == noErr), @"AudioConverterNew failed. Error: %d.", errorCode);
	
	UInt32 baseBufferSize = kPKCanonicalBaseBufferSize;
	if(sourceFormat->IsInterleaved())
	{
		UInt32 bufferSize = (baseBufferSize * sourceFormat->SampleWordSize());
		mDecoderDataConverterBufferList = CAAudioBufferList::Create(1);
		mDecoderDataConverterBufferList->mBuffers[0].mData = malloc(bufferSize);
		mDecoderDataConverterBufferList->mBuffers[0].mDataByteSize = bufferSize;
		mDecoderDataConverterBufferList->mBuffers[0].mNumberChannels = bufferSize;
	}
	else
	{
		mDecoderDataConverterBufferList = CAAudioBufferList::Create(sourceFormat->mChannelsPerFrame);
		UInt32 bufferSize = (baseBufferSize * sourceFormat->mBytesPerPacket) / sourceFormat->mChannelsPerFrame;
		
		for (int index = 0; index < mDecoderDataConverterBufferList->mNumberBuffers; index++)
		{
			mDecoderDataConverterBufferList->mBuffers[index].mData = malloc(bufferSize);
			mDecoderDataConverterBufferList->mBuffers[index].mDataByteSize = bufferSize;
			mDecoderDataConverterBufferList->mBuffers[index].mNumberChannels = 1;
		}
	}
}

#pragma mark -

- (BOOL)setDecoder:(PKDecoder *)decoder error:(NSError **)error
{
	if(mPlaybackDecoder == decoder)
		return YES;
	
	@synchronized(self)
	{
		if([self isPlaying])
		{
			[self stop];
		}
		
		if(mPlaybackDecoder)
		{
			mPlaybackDecoder->Release();
			mPlaybackDecoder = NULL;
		}
		
		if(mDecoderDataConverter)
		{
			AudioConverterDispose(mDecoderDataConverter);
			mDecoderDataConverter = nil;
		}
		
		if(mDecoderDataConverterBufferList)
		{
			for (int index = 0; index < mDecoderDataConverterBufferList->mNumberBuffers; index++)
			{
				free(mDecoderDataConverterBufferList->mBuffers[index].mData);
			}
			
			CAAudioBufferList::Destroy(mDecoderDataConverterBufferList);
			mDecoderDataConverterBufferList = nil;
		}
		
		if(decoder)
		{
			CAStreamBasicDescription nativeFormat = decoder->GetStreamFormat();
			
			//
			//	If the data being given to us is already canonical,
			//	we use the data as is. This is an attempt at efficiency.
			//
			CAStreamBasicDescription audioFormat;
			if(PKStreamFormatIsCanonical(nativeFormat))
			{
				audioFormat = nativeFormat;
				
				mDecoderDataConverter = nil;
				mDecoderDataConverterBufferList = nil;
				
				mPlaybackEngine->SetScheduleSliceFunctionHandler(PKAudioPlayerScheduleSlice);
				mPlaybackEngine->SetScheduleSliceFunctionHandlerUserData(self);
			}
			else
			{
				[self setupAudioConverterWithSourceFormat:&nativeFormat resultFormat:&audioFormat];
				
				mPlaybackEngine->SetScheduleSliceFunctionHandler(PKAudioPlayerScheduleSliceWithConverter);
				mPlaybackEngine->SetScheduleSliceFunctionHandlerUserData(self);
			}
			
			//
			//	We assign this now, so the error handling
			//	in setAudioStreamBasicDescription:error:
			//	can access what file we're trying to load
			//
			mPlaybackDecoder = decoder;
			
			try
			{
				mPlaybackEngine->SetStreamFormat(audioFormat);
			}
			catch (RBException e)
			{
				mPlaybackDecoder = nil;
				
				if(error) *error = PKError(PKPlaybackErrorDomain, 
										   kPNPlaybackErrorIncompatibleStreamFormat, 
										   [NSDictionary dictionaryWithObject:[NSMakeCollectable(e.CopyError()) autorelease] 
																	   forKey:NSUnderlyingErrorKey], 
										   @"The stream format of the file %@ cannot be decoded.", [NSMakeCollectable(decoder->CopyLocation()) autorelease]);
				
				return NO;
			}
			
			mPlaybackDecoder = decoder;
		}
	}
	
	return YES;
}

- (PKDecoder *)decoder
{
	@synchronized(self)
	{
		return mPlaybackDecoder;
	}
	
	return nil;
}

#pragma mark -
#pragma mark Playback Control

- (BOOL)setLocation:(NSURL *)location error:(NSError **)error
{
	if(!location)
	{
		return [self setDecoder:nil error:error];
	}
	
	try
	{
		PKDecoder *decoder = PKDecoder::DecoderForURL(CFURLRef(location));
		if(decoder)
		{
			[self setDecoder:decoder error:error];
		}
		else
		{
			return NO;
		}
	}
	catch (RBException e)
	{
		if(error) *error = [NSMakeCollectable(e.CopyError()) autorelease];
		
		return NO;
	}
	
	return YES;
}

- (NSURL *)location
{
	if(mPlaybackDecoder)
	{
		return [NSMakeCollectable(mPlaybackDecoder->CopyLocation()) autorelease];
	}
	
	return nil;
}

#pragma mark -

- (void)play
{
	@synchronized(self)
	{
		if([self isPaused])
		{
			[self resume];
			return;
		}
		
		try
		{
			if(!mPlaybackEngine->IsRunning())
			{
				mPlaybackEngine->StartGraph();
				mPlaybackEngine->StartProcessing();
			}
		}
		catch (RBException e)
		{
			@throw RBExceptionToNSException(e);
		}
	}
}

- (void)stop
{
	@synchronized(self)
	{
		if(![self isPlaying] && ![self isPaused])
			return;
		
		try
		{
			if(mPlaybackEngine->IsRunning())
				mPlaybackEngine->StopGraph();
			
			mPlaybackEngine->StopProcessing();
			
			mIsPlaybackPaused = NO;
			self.currentTime = 0.0;
			
			if([mDelegate respondsToSelector:@selector(audioPlayer:didFinishPlaying:)])
				[mDelegate audioPlayer:self didFinishPlaying:YES];
		}
		catch (RBException e)
		{
			@throw RBExceptionToNSException(e);
		}
	}
}

- (BOOL)isPlaying
{
	return mPlaybackEngine->IsRunning();
}

#pragma mark -

- (BOOL)isPaused
{
	@synchronized(self)
	{
		return mIsPlaybackPaused;
	}
	
	return NO;
}

- (void)resume
{
    @synchronized(self)
    {
        if(mIsPlaybackPaused)
        {
            mIsPlaybackPaused = NO;
			
			try
			{
				mPlaybackEngine->ResumeProcessing(/* preserveExistingSampleBuffers = */true);
				mPlaybackEngine->StartGraph();
			}
			catch (RBException e)
			{
				@throw RBExceptionToNSException(e);
			}
        }
		else
		{
			[self play];
		}
    }
}

- (void)pause
{
    @synchronized(self)
    {
		if(mIsPlaybackPaused)
			return;
		
		//
		//	If we're not playing, we do nothing. Why? Because attempting to
		//	resume processing when we aren't paused will cause some problems.
		//
        if([self isPlaying])
        {
			try
			{
				mPlaybackEngine->StopGraph();
				mPlaybackEngine->PauseProcessing();
			}
			catch (RBException &e)
			{
				@throw RBExceptionToNSException(e);
			}
			
			mIsPlaybackPaused = YES;
        }
    }
}

#pragma mark -
#pragma mark Effects

- (PKAudioEffect *)addEffectWithComponentDescription:(const AudioComponentDescription *)inDescription error:(NSError **)error
{
	try
	{
		AUNode node = mPlaybackEngine->AddNode(inDescription);
		
		NSIndexSet *insertionIndexes = [NSIndexSet indexSetWithIndex:[mAudioEffects count]];
		[self willChange:NSKeyValueChangeInsertion valuesAtIndexes:insertionIndexes forKey:@"effects"];
		
		PKAudioEffect *effect = [[PKAudioEffect alloc] initWithAudioPlayer:self node:node];
		[mAudioEffects addObject:effect];
		
		[self didChange:NSKeyValueChangeInsertion valuesAtIndexes:insertionIndexes forKey:@"effects"];
		
		return [effect autorelease];
	}
	catch (RBException e)
	{
		if(error) *error = PKError(PKEffectsErrorDomain, 
								   kPKEffectsErrorCannotAddEffectToAudioPlayer, 
								   [NSDictionary dictionaryWithObject:[NSMakeCollectable(e.CopyError()) autorelease] 
															   forKey:NSUnderlyingErrorKey], 
								   @"Could not add audio effect to player.");
	}
	
	return nil;
}

- (BOOL)removeEffect:(PKAudioEffect *)effect error:(NSError **)error
{
	NSParameterAssert(effect);
	
	NSUInteger indexOfEffect = [mAudioEffects indexOfObject:effect];
	NSAssert1((indexOfEffect != NSNotFound), 
			  @"Attempting to remove effect %@ that isn't part of the audio unit graph. 'Tisk, 'tisk.", effect);
	
	try
	{
		mPlaybackEngine->RemoveNode(effect.representedNode);
	}
	catch (RBException e)
	{
		if(error) *error = PKError(PKEffectsErrorDomain, 
								   kPKEffectsErrorCannotRemoveEffectFromAudioPlayer, 
								   [NSDictionary dictionaryWithObject:[NSMakeCollectable(e.CopyError()) autorelease] 
															   forKey:NSUnderlyingErrorKey], 
								   @"Could not remove the audio effect «%@» from player.", effect.title);
		
		return NO;
	}
	
	
	NSIndexSet *removedIndexes = [NSIndexSet indexSetWithIndex:indexOfEffect];
	[self willChange:NSKeyValueChangeRemoval valuesAtIndexes:removedIndexes forKey:@"effects"];
	
	[mAudioEffects removeObject:effect];
	
	[self didChange:NSKeyValueChangeRemoval valuesAtIndexes:removedIndexes forKey:@"effects"];
	
	return YES;
}

@synthesize effects = mAudioEffects;

#pragma mark -

#if 0
- (NSArray *)arrayRepresentationOfEffects
{
	NSMutableArray *savedEffects = [NSMutableArray array];
	for (PKAudioEffect *effect in self.effects)
	{
		[savedEffects addObject:[NSDictionary dictionaryWithObjectsAndKeys:
								 effect.title, @"Name",
								 PKAudioComponentDescriptionGetDictionaryRepresentation(effect.representedComponent),  @"ComponentDescription",
								 effect.classData, @"ClassData",
								 [NSNumber numberWithBool:effect.enabled], @"Enabled",
								 nil]];
	}
    
    return savedEffects;
}

- (BOOL)loadEffectsFromArrayRepresentation:(NSArray *)effects error:(NSError **)error
{
	NSParameterAssert(effects);
	
    for (NSDictionary *savedEffect in effects)
    {
        id classData = [savedEffect valueForKey:@"ClassData"];
        if(classData)
        {
			NSDictionary *savedDescription = [savedEffect valueForKey:@"ComponentDescription"];
			AudioComponentDescription description = PKAudioComponentDescriptionCreateFromDictionaryRepresentation(savedDescription);
			
            PKAudioEffect *effect = [self addEffectWithComponentDescription:&description error:error];
            if(!effect)
                return NO;
			
			effect.classData = classData;
			effect.enabled = [[savedEffect valueForKey:@"Enabled"] boolValue];
        }
    }
	
	return YES;
}
#endif

#pragma mark -

- (BOOL)removeAllEffects:(NSError **)error
{
	NSError *underlyingError = nil;
	while ([mAudioEffects count] > 0)
	{
		if(![self removeEffect:[mAudioEffects lastObject] error:&underlyingError])
			break;
	}
	
	if(underlyingError)
	{
		if(error) *error = PKError(PKEffectsErrorDomain, 
								   kPKEffectsErrorCannotRemoveAllEffects, 
								   [NSDictionary dictionaryWithObject:underlyingError forKey:NSUnderlyingErrorKey], 
								   @"Could not remove all effects from player.");
		return NO;
	}
	
	return YES;
}

@end
