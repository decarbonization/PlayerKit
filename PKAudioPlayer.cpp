/*
 *  PKAudioPlayer.cpp
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 10/16/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#import "PKAudioPlayer.h"
#import "PKAudioPlayerInternal.h"
#import <libKern/OSAtomic.h>

#import "CAAudioBufferList.h"
#import "CAStreamBasicDescription.h"

#import "RBLockableObject.h"

///The singleton instance of the audio player state.
PK_VISIBILITY_HIDDEN RBLockableObject AudioPlayerStateLock;
PK_VISIBILITY_HIDDEN PKAudioPlayer AudioPlayerState = { /* Initialized in PKAudioPlayerInit */ };
PK_VISIBILITY_HIDDEN volatile int32_t AudioPlayerStateInitCount = 0;

#pragma mark Constants

PK_EXTERN CFStringRef const PKAudioPlayerDidEncounterErrorNotification = CFSTR("PKAudioPlayerDidEncounterErrorNotification");
PK_EXTERN CFStringRef const PKAudioPlayerDidFinishPlayingNotification = CFSTR("PKAudioPlayerDidFinishPlayingNotification");
PK_EXTERN CFStringRef const PKAudioPlayerDidChangeOutputDeviceNotification = CFSTR("PKAudioPlayerDidChangeOutputDeviceNotification");
PK_EXTERN CFStringRef const PKAudioPlayerDidEncounterOtherPlayerNotification = CFSTR("PKAudioPlayerDidEncounterOtherPlayerNotification");

#pragma mark -

static CFStringRef const PKAudioPlayerDidBroadcastPresenceNotification = CFSTR("com.roundabout.PKAudioPlayerDidBroadcastPresenceNotification");

static void PKAudioPlayerDidBroadcastPresence(CFNotificationCenterRef center, 
											  void *observer, 
											  CFStringRef name, 
											  const void *object, 
											  CFDictionaryRef userInfo);

#pragma mark -
#pragma mark Lifecycle

PK_EXTERN Boolean PKAudioPlayerInit(CFErrorRef *outError)
{
	if(OSMemoryBarrier(), AudioPlayerStateInitCount > 0)
	{
		OSAtomicIncrement32Barrier(&AudioPlayerStateInitCount);
		return true;
	}
	
	RBLockableObject::Acquisitor lock(&AudioPlayerStateLock);
	
	try
	{
		AudioPlayerState.engine = PKAudioPlayerEngine::New();
		
		AudioPlayerState.engine->SetErrorHandler(^(CFErrorRef error) {
			
			try
			{
				if(AudioPlayerState.engine->IsRunning())
					AudioPlayerState.engine->StopGraph();
				
				AudioPlayerState.engine->StopProcessing();
				
				AudioPlayerState.isPaused = false;
				PKAudioPlayerSetCurrentTime(0.0, NULL);
			}
			catch (RBException e)
			{
				std::cerr << "An exception was raised while resetting PKAudioPlayer's internal state after an error occurred {";
				CFErrorRef tempError = e.CopyError();
				CFShow(tempError);
				CFRelease(tempError);
				std::cerr << "}. It will be ignored." << std::endl;
			}
			
			dispatch_async(dispatch_get_main_queue(), ^{
				CFDictionaryRef userInfo = CFDICT({ CFSTR("Error") }, { error });
				
				CFNotificationCenterPostNotification(CFNotificationCenterGetLocalCenter(), 
													 PKAudioPlayerDidEncounterErrorNotification, 
													 NULL, 
													 userInfo, 
													 true);
				
				CFRelease(userInfo);
				CFRelease(error);
			});
			
		});
		
		AudioPlayerState.engine->SetEndOfPlaybackHandler(^{
			
			dispatch_async(dispatch_get_main_queue(), ^{
				CFDictionaryRef userInfo = CFDICT({ CFSTR("DidFinish") }, { kCFBooleanTrue });
				
				CFNotificationCenterPostNotification(CFNotificationCenterGetLocalCenter(), 
													 PKAudioPlayerDidFinishPlayingNotification, 
													 NULL, 
													 userInfo, 
													 true);
				
				CFRelease(userInfo);
			});
			
		});
		
		AudioPlayerState.engine->SetOutputDeviceDidChangeHandler(^{
			
			dispatch_async(dispatch_get_main_queue(), ^{
				CFNotificationCenterPostNotification(CFNotificationCenterGetLocalCenter(), 
													 PKAudioPlayerDidChangeOutputDeviceNotification, 
													 NULL, 
													 NULL, 
													 true);
			});
			
		});
		
		CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(), 
										NULL, 
										CFNotificationCallback(&PKAudioPlayerDidBroadcastPresence), 
										PKAudioPlayerDidBroadcastPresenceNotification, 
										NULL, 
										CFNotificationSuspensionBehaviorDeliverImmediately);
		
		CFUUIDRef sessionUUID = CFUUIDCreate(kCFAllocatorDefault);
		AudioPlayerState.sessionID = CFUUIDCreateString(kCFAllocatorDefault, sessionUUID);
		CFRelease(sessionUUID);
	}
	catch (RBException e)
	{
		if(outError) *outError = e.CopyError();
		
		return false;
	}
	
	OSAtomicIncrement32Barrier(&AudioPlayerStateInitCount);
	
	return true;
}

PK_EXTERN Boolean PKAudioPlayerTeardown(CFErrorRef *outError)
{
	if(OSMemoryBarrier(), AudioPlayerStateInitCount > 0)
	{
		if(OSAtomicDecrement32(&AudioPlayerStateInitCount) != 0)
			return true;
	}
	
	RBLockableObject::Acquisitor lock(&AudioPlayerStateLock);
	
	try
	{
		if(PKAudioPlayerIsPlaying() && !PKAudioPlayerStop(outError))
			return false;
		
		if(AudioPlayerState.engine)
			delete AudioPlayerState.engine;
		
		if(AudioPlayerState.decoderConverter)
		{
			AudioConverterDispose(AudioPlayerState.decoderConverter);
			AudioPlayerState.decoderConverter = NULL;
		}
		
		if(AudioPlayerState.decoderConverterBuffers)
		{
			for (int index = 0; index < AudioPlayerState.decoderConverterBuffers->mNumberBuffers; index++)
			{
				free(AudioPlayerState.decoderConverterBuffers->mBuffers[index].mData);
			}
			
			CAAudioBufferList::Destroy(AudioPlayerState.decoderConverterBuffers);
			AudioPlayerState.decoderConverterBuffers = NULL;
		}
		
		if(AudioPlayerState.decoder)
		{
			AudioPlayerState.decoder->Release();
			AudioPlayerState.decoder = NULL;
		}
		
		if(AudioPlayerState.sessionID)
		{
			CFRelease(AudioPlayerState.sessionID);
			AudioPlayerState.sessionID = NULL;
		}
	}
	catch (RBException e)
	{
		if(outError) *outError = e.CopyError();
		
		return false;
	}
	
	memset(&AudioPlayerState, 0, sizeof(AudioPlayerState));
	
	return true;
}

#pragma mark -
#pragma mark Notifications

static void PKAudioPlayerDidBroadcastPresence(CFNotificationCenterRef center, 
											  void *observer, 
											  CFStringRef name, 
											  const void *object, 
											  CFDictionaryRef userInfo)
{
	if(CFEqual(AudioPlayerState.sessionID, CFStringRef(object)))
	   return;
	
	dispatch_async(dispatch_get_main_queue(), ^{
		CFNotificationCenterPostNotification(CFNotificationCenterGetLocalCenter(), 
											 PKAudioPlayerDidEncounterOtherPlayerNotification, 
											 NULL, 
											 NULL, 
											 true);
	});
}

#pragma mark -
#pragma mark Playback Callbacks

static UInt32 PKAudioPlayerScheduleSlice(PKAudioPlayerEngine *graph, AudioBufferList *ioBuffer, UInt32 numberOfFramesToRead, CFErrorRef *error, void *userData)
{
	try
	{
		return AudioPlayerState.decoder->FillBuffers(ioBuffer, numberOfFramesToRead);
	}
	catch (RBException e)
	{
		if(error) *error = e.CopyError();
		
		return 0;
	}
}

struct PKAudioPlayerConverterState
{
	CFErrorRef mError;
};

static OSStatus PKAudioPlayerConverterCallback(AudioConverterRef inAudioConverter, UInt32 *ioNumberDataPackets, AudioBufferList *ioData, AudioStreamPacketDescription **outDataPacketDescription, void *inUserData)
{
	PKAudioPlayerConverterState *sharedState = (PKAudioPlayerConverterState *)inUserData;
	
	for (int index = 0; index < AudioPlayerState.decoderConverterBuffers->mNumberBuffers; index++)
		ioData->mBuffers[index] = AudioPlayerState.decoderConverterBuffers->mBuffers[index];
	
	try
	{
		*ioNumberDataPackets = AudioPlayerState.decoder->FillBuffers(ioData, *ioNumberDataPackets);
		
		if(*ioNumberDataPackets == 0)
		{
			//This is necessary or AudioConverter will continue to call this function
			//producing garbage that the user then has to hear. We don't want that to happen.
			for (int index = 0; index < AudioPlayerState.decoderConverterBuffers->mNumberBuffers; index++)
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
	UInt32 ioNumberOfFramesForConverter = numberOfFramesToRead;
	PKAudioPlayerConverterState converterState = { mError: NULL };
	OSStatus errorCode = AudioConverterFillComplexBuffer(AudioPlayerState.decoderConverter, //in audioConverter
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
#pragma mark Controlling Playback

static void __PKAudioPlayerSetupAudioConverter(CAStreamBasicDescription *sourceFormat, CAStreamBasicDescription *resultFormat) throw(RBException)
{
	resultFormat->mSampleRate = kPKCanonicalSampleRate;
	resultFormat->SetCanonical(2, false);
	
	OSStatus errorCode = AudioConverterNew(sourceFormat, resultFormat, &AudioPlayerState.decoderConverter);
	RBAssert((errorCode == noErr), CFSTR("AudioConverterNew failed. Error: %d."), errorCode);
	
	UInt32 baseBufferSize = kPKCanonicalBaseBufferSize;
	if(sourceFormat->IsInterleaved())
	{
		UInt32 bufferSize = (baseBufferSize * sourceFormat->SampleWordSize());
		AudioPlayerState.decoderConverterBuffers = CAAudioBufferList::Create(1);
		AudioPlayerState.decoderConverterBuffers->mBuffers[0].mData = malloc(bufferSize);
		AudioPlayerState.decoderConverterBuffers->mBuffers[0].mDataByteSize = bufferSize;
		AudioPlayerState.decoderConverterBuffers->mBuffers[0].mNumberChannels = bufferSize;
	}
	else
	{
		AudioPlayerState.decoderConverterBuffers = CAAudioBufferList::Create(sourceFormat->mChannelsPerFrame);
		UInt32 bufferSize = (baseBufferSize * sourceFormat->mBytesPerPacket) / sourceFormat->mChannelsPerFrame;
		
		for (int index = 0; index < AudioPlayerState.decoderConverterBuffers->mNumberBuffers; index++)
		{
			AudioBuffer &buffer = AudioPlayerState.decoderConverterBuffers->mBuffers[index];
			buffer.mData = malloc(bufferSize);
			buffer.mDataByteSize = bufferSize;
			buffer.mNumberChannels = 1;
		}
	}
}

PK_EXTERN Boolean PKAudioPlayerSetDecoder(PKDecoder *decoder, CFErrorRef *outError)
{
	if(decoder == AudioPlayerState.decoder)
		return true;
	
	if(PKAudioPlayerIsPlaying() && !PKAudioPlayerStop(outError))
		return false;
	
	RBLockableObject::Acquisitor lock(&AudioPlayerStateLock);
	
	if(AudioPlayerState.decoder)
	{
		AudioPlayerState.decoder->Release();
		AudioPlayerState.decoder = NULL;
	}
	
	if(AudioPlayerState.decoderConverter)
	{
		AudioConverterDispose(AudioPlayerState.decoderConverter);
		AudioPlayerState.decoderConverter = NULL;
	}
	
	if(AudioPlayerState.decoderConverterBuffers)
	{
		for (int index = 0; index < AudioPlayerState.decoderConverterBuffers->mNumberBuffers; index++)
		{
			free(AudioPlayerState.decoderConverterBuffers->mBuffers[index].mData);
		}
		
		CAAudioBufferList::Destroy(AudioPlayerState.decoderConverterBuffers);
		AudioPlayerState.decoderConverterBuffers = NULL;
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
			
			AudioPlayerState.decoderConverter = nil;
			AudioPlayerState.decoderConverterBuffers = nil;
			
			AudioPlayerState.engine->SetScheduleSliceFunctionHandler(PKAudioPlayerScheduleSlice);
		}
		else
		{
			__PKAudioPlayerSetupAudioConverter(&nativeFormat, &audioFormat);
			AudioPlayerState.engine->SetScheduleSliceFunctionHandler(PKAudioPlayerScheduleSliceWithConverter);
		}
		
		AudioPlayerState.decoder = decoder;
		
		try
		{
			AudioPlayerState.engine->SetStreamFormat(audioFormat);
		}
		catch (RBException e)
		{
			AudioPlayerState.decoder = NULL;
			
			if(outError)
			{
				CFErrorRef underlyingError = e.CopyError();
				
				CFDictionaryRef userInfo = CFDICT({ kCFErrorUnderlyingErrorKey }, { underlyingError });
				CFRelease(underlyingError);
				
				CFURLRef decoderLocation = decoder->CopyLocation();
				*outError = PKCopyError(PKPlaybackErrorDomain, 
										kPKPlaybackErrorIncompatibleStreamFormat, 
										userInfo, 
										CFSTR("The stream format of the file %@ cannot be decoded."), decoderLocation);
				CFRelease(decoderLocation);
				CFRelease(userInfo);
			}
			
			return false;
		}
	}
	
	return true;
}

PK_EXTERN PKDecoder *PKAudioPlayerGetDecoder()
{
	RBLockableObject::Acquisitor lock(&AudioPlayerStateLock);
	
	return AudioPlayerState.decoder;
}

#pragma mark -

PK_EXTERN Boolean PKAudioPlayerSetURL(CFURLRef location, CFErrorRef *outError)
{
	RBLockableObject::Acquisitor lock(&AudioPlayerStateLock);
	
	try
	{
		if(location)
		{
			PKDecoder *decoder = PKDecoder::DecoderForURL(location);
			RBAssert((decoder != NULL), CFSTR("Could not find decoder for {%@}."), location);
			
			return PKAudioPlayerSetDecoder(decoder, outError);
		}
		else
		{
			return PKAudioPlayerSetDecoder(NULL, outError);
		}
	}
	catch (RBException e)
	{
		if(outError) *outError = e.CopyError();
		
		return false;
	}
	
	return true;
}

PK_EXTERN CFURLRef PKAudioPlayerCopyURL()
{
	RBLockableObject::Acquisitor lock(&AudioPlayerStateLock);
	
	return AudioPlayerState.decoder->CopyLocation();
}

#pragma mark -

PK_EXTERN Boolean PKAudioPlayerPlay(CFErrorRef *outError)
{
	if(PKAudioPlayerIsPlaying())
		return true;
	
	if(PKAudioPlayerIsPaused())
		return PKAudioPlayerResume(outError);
	
	RBLockableObject::Acquisitor lock(&AudioPlayerStateLock);
	
	if(OSMemoryBarrier(), !AudioPlayerState.hasBroadcastedPresence)
	{
		CFNotificationCenterPostNotification(CFNotificationCenterGetDistributedCenter(), 
											 PKAudioPlayerDidBroadcastPresenceNotification, 
											 AudioPlayerState.sessionID, 
											 NULL, 
											 true);
		
		OSAtomicCompareAndSwap32Barrier(AudioPlayerState.hasBroadcastedPresence, 1, &AudioPlayerState.hasBroadcastedPresence);
	}
	
	try
	{
		if(!AudioPlayerState.engine->IsRunning())
		{
			AudioPlayerState.engine->StartGraph();
			AudioPlayerState.engine->StartProcessing();
		}
	}
	catch (RBException e)
	{
		if(outError) *outError = e.CopyError();
		return false;
	}
	
	return true;
}

PK_EXTERN Boolean PKAudioPlayerStop(CFErrorRef *outError)
{
	if(!PKAudioPlayerIsPlaying() && !PKAudioPlayerIsPaused())
		return true;
	
	RBLockableObject::Acquisitor lock(&AudioPlayerStateLock);
	
	try
	{
		if(AudioPlayerState.engine->IsRunning())
			AudioPlayerState.engine->StopGraph();
		
		AudioPlayerState.engine->StopProcessing();
		
		AudioPlayerState.isPaused = false;
		PKAudioPlayerSetCurrentTime(0.0, NULL);
		
		dispatch_async(dispatch_get_main_queue(), ^{
			CFDictionaryRef userInfo = CFDICT({ CFSTR("DidFinish") }, { kCFBooleanFalse });
			
			CFNotificationCenterPostNotification(CFNotificationCenterGetLocalCenter(), 
												 PKAudioPlayerDidFinishPlayingNotification, 
												 NULL, 
												 userInfo, 
												 true);
			
			CFRelease(userInfo);
		});
	}
	catch (RBException e)
	{
		if(outError) *outError = e.CopyError();
	}
	
	return true;
}

PK_EXTERN Boolean PKAudioPlayerIsPlaying()
{
	RBLockableObject::Acquisitor lock(&AudioPlayerStateLock);
	
	return AudioPlayerState.engine->IsRunning();
}

#pragma mark -

PK_EXTERN Boolean PKAudioPlayerPause(CFErrorRef *outError)
{
	PKAudioPlayerEngine *engine = AudioPlayerState.engine;
	
	if(OSMemoryBarrier(), AudioPlayerState.isPaused)
		return true;
	
	RBLockableObject::Acquisitor lock(&AudioPlayerStateLock);
	
	//
	//	If we're not playing, we do nothing. Why? Because attempting to
	//	resume processing when we aren't paused will cause some problems.
	//
	if(PKAudioPlayerIsPlaying())
	{
		try
		{
			engine->StopGraph();
			engine->PauseProcessing();
		}
		catch (RBException e)
		{
			if(outError) *outError = e.CopyError();
			
			return false;
		}
		
		OSAtomicCompareAndSwap32Barrier(AudioPlayerState.isPaused, true, &AudioPlayerState.isPaused);
	}
	
	return true;
}

PK_EXTERN Boolean PKAudioPlayerResume(CFErrorRef *outError)
{
	PKAudioPlayerEngine *engine = AudioPlayerState.engine;
	
	RBLockableObject::Acquisitor lock(&AudioPlayerStateLock);
	
	if(OSMemoryBarrier(), AudioPlayerState.isPaused)
	{
		OSAtomicCompareAndSwap32Barrier(AudioPlayerState.isPaused, false, &AudioPlayerState.isPaused);
		
		try
		{
			engine->ResumeProcessing(/* preserveExistingSampleBuffers = */true);
			engine->StartGraph();
		}
		catch (RBException e)
		{
			if(outError) *outError = e.CopyError();
			
			return false;
		}
	}
	else
	{
		return PKAudioPlayerPlay(outError);
	}
	
	return true;
}

PK_EXTERN Boolean PKAudioPlayerIsPaused()
{
	return OSMemoryBarrier(), AudioPlayerState.isPaused;
}

#pragma mark -
#pragma mark Properties

PK_EXTERN void PKAudioPlayerSetPausesOnOutputDeviceChanges(Boolean pausesOnOutputDeviceChange)
{
	AudioPlayerState.engine->SetPausesOnOutputDeviceChanges(pausesOnOutputDeviceChange);
}

PK_EXTERN Boolean PKAudioPlayerGetPausesOnOutputDeviceChanges()
{
	return AudioPlayerState.engine->GetPausesOnOutputDeviceChanges();
}

#pragma mark -

PK_EXTERN Boolean PKAudioPlayerSetVolume(Float32 volume, CFErrorRef *outError)
{
	try
	{
		AudioPlayerState.engine->SetVolume(volume);
	}
	catch (RBException e)
	{
		if(outError) *outError = e.CopyError();
		
		return false;
	}
	
	return true;
}

PK_EXTERN Float32 PKAudioPlayerGetVolume()
{
	try
	{
		return AudioPlayerState.engine->GetVolume();
	}
	catch (RBException e)
	{
		std::cerr << "***Warning: Could not get volume for PKAudioPlayer." << std::endl;
	}
	
	return 1.0;
}

PK_EXTERN Float32 PKAudioPlayerGetAverageCPUUsage()
{
	return AudioPlayerState.engine->GetAverageCPUUsage();
}

#pragma mark -

PK_EXTERN CFTimeInterval PKAudioPlayerGetDuration()
{
	RBLockableObject::Acquisitor lock(&AudioPlayerStateLock);
	
	PKDecoder *decoder = AudioPlayerState.decoder;
	if(decoder)
		return decoder->GetTotalNumberOfFrames() / decoder->GetStreamFormat().mSampleRate;
	
	return 0.0;
}

#pragma mark -

PK_EXTERN Boolean PKAudioPlayerSetCurrentTime(CFTimeInterval currentTime, CFErrorRef *outError)
{
	RBLockableObject::Acquisitor lock(&AudioPlayerStateLock);
	
	PKAudioPlayerEngine *engine = AudioPlayerState.engine;
	PKDecoder *decoder = AudioPlayerState.decoder;
	
	if(decoder && decoder->CanSeek())
	{
		Boolean shouldRestartGraph = PKAudioPlayerIsPlaying();
		try
		{
			if(shouldRestartGraph)
			{
				engine->StopGraph();
				
				engine->Acquire();
				engine->PauseProcessing();
			}
			
			decoder->SetCurrentFrame(currentTime * decoder->GetStreamFormat().mSampleRate);
			
			if(shouldRestartGraph)
			{
				engine->ResumeProcessing();
				engine->Relinquish();
				
				engine->StartGraph();
			}
		}
		catch (RBException e)
		{
			//
			//	We prevent deadlocks here by relinquishing our exclusive access to the audio player engine.
			//	We only relinquish if we should be starting and stopping the graph and there are
			//	one or more locks active on the playback engine.
			//
			if(shouldRestartGraph)
				engine->Relinquish();
			
			if(outError) *outError = e.CopyError();
			
			return false;
		}
		
		return true;
	}
	
	return false;
}

PK_EXTERN CFTimeInterval PKAudioPlayerGetCurrentTime()
{
	RBLockableObject::Acquisitor lock(&AudioPlayerStateLock);
	
	PKDecoder *decoder = AudioPlayerState.decoder;
	if(decoder)
		return decoder->GetCurrentFrame() / decoder->GetStreamFormat().mSampleRate;
	
	return 0.0;
}
