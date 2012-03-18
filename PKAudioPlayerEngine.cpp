/*
 *  PKAudioPlayerEngine.cpp
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 6/21/09.
 *  Copyright 2009 Roundabout Software. All rights reserved.
 *
 */

#include "PKAudioPlayerEngine.h"
#include <iostream>

#include "CAComponent.h"
#include "CAComponentDescription.h"
#include "CAAudioBufferList.h"
#include "CAStreamBasicDescription.h"

#include "PKScheduledDataSlice.h"
#include "PKTaskQueue.h"

#pragma mark Tools

static AudioBufferList *_AllocateBuffers(const CAStreamBasicDescription &streamFormat, UInt32 bufferSize)
{
	AudioBufferList *bufferList = NULL;
	if(streamFormat.IsInterleaved())
	{
		bufferList = CAAudioBufferList::Create(1);
		
		bufferList->mBuffers[0].mData = malloc(bufferSize);
		bufferList->mBuffers[0].mDataByteSize = bufferSize;
		bufferList->mBuffers[0].mNumberChannels = streamFormat.mChannelsPerFrame;
	}
	else
	{
		bufferList = CAAudioBufferList::Create(streamFormat.mChannelsPerFrame);
		
		for (UInt32 index = 0; index < bufferList->mNumberBuffers; index++)
		{
			bufferList->mBuffers[index].mData = malloc(bufferSize);
			bufferList->mBuffers[index].mDataByteSize = bufferSize;
			bufferList->mBuffers[index].mNumberChannels = 1;
		}
	}
	
	return bufferList;
}

static void _DeallocateBuffers(AudioBufferList *buffers)
{
	if(buffers)
	{
		for (int index = 0; index < buffers->mNumberBuffers; index++)
			free(buffers->mBuffers[index].mData);
		
		CAAudioBufferList::Destroy(buffers);
	}
}

#pragma mark -
#pragma mark PKAudioPlayerEngine

PKAudioPlayerEngine::~PKAudioPlayerEngine()
{
	//
	//	Remove `this` as an observer for the default output device.
	//	We don't need notifications anymore.
	//
	AudioObjectPropertyAddress address = {
		.mSelector = kAudioHardwarePropertyDefaultOutputDevice, 
		.mScope = kAudioObjectPropertyScopeGlobal, 
		.mElement = 0
	};
	AudioObjectRemovePropertyListener(kAudioObjectSystemObject, //in audioObjectID
									  &address, //in propertyAddressPtr
									  PKAudioPlayerEngine::DefaultAudioDeviceDidChangeListenerProc, //in listenerCallbackProc
									  this); //in listenerCallbackProcUserData
	
	
	if(mSortedDataSlicesForPausedProcessing)
	{
		CFRelease(mSortedDataSlicesForPausedProcessing);
		mSortedDataSlicesForPausedProcessing = NULL;
	}
	
	if(mAudioUnitGraph)
	{
		AUGraphClose(mAudioUnitGraph);
		DisposeAUGraph(mAudioUnitGraph);
		mAudioUnitGraph = NULL;
	}
	
	
	for (int index = 0; index < kNumberOfSlicesToKeepActive; index++)
		mDataSlices[index]->Release();
	
	
	mSchedulerQueue->Release();
	mSchedulerQueue = NULL;
}

PKAudioPlayerEngine::PKAudioPlayerEngine() throw(RBException) : 
	RBLockableObject("PKAudioPlayerEngine"),
	mSchedulerQueue(new PKTaskQueue("com.roundabout.playerkit.PKAudioPlayerEngine.mSchedulerQueue")),
	mSortedDataSlicesForPausedProcessing(NULL),
	mProcessingIsPaused(false),
	mErrorHasOccurredDuringProcessing(false),
	mErrorHandler(NULL),
	mEndOfPlaybackHandler(NULL),
	mPulseHandler(NULL),
	mOutputDeviceDidChangeHandler(NULL),
	mScheduleSliceFunctionHandler(NULL),
	mScheduleSliceFunctionHandlerUserData(NULL)
{
	//Initialize the AUGraph that's used to push audio to the sound system
	OSStatus error = noErr;
	
	error = NewAUGraph(&mAudioUnitGraph);
	RBAssertNoErr(error, CFSTR("NewAUGraph failed. Error %d."), error);
	
	CAComponentDescription outputComponent(kAudioUnitType_Output, 
										   kAudioUnitSubType_DefaultOutput, 
										   kAudioUnitManufacturer_Apple);
	error = AUGraphAddNode(mAudioUnitGraph, &outputComponent, &mOutputNode);
	RBAssertNoErr(error, CFSTR("AUGraphAddNode failed for output audio unit. Error %d."), error);
	
	CAComponentDescription schedulerComponent(kAudioUnitType_Generator, 
											  kAudioUnitSubType_ScheduledSoundPlayer, 
											  kAudioUnitManufacturer_Apple);
	error = AUGraphAddNode(mAudioUnitGraph, &schedulerComponent, &mScheduledAudioPlayerNode);
	RBAssertNoErr(error, CFSTR("AUGraphAddNode failed for scheduled audio player audio unit. Error %d."), error);
	
	error = AUGraphConnectNodeInput(mAudioUnitGraph, //in graph
									mScheduledAudioPlayerNode, //in sourceNode
									0, //in sourceNodeElement
									mOutputNode, //in destinationNode
									0); //in destinationNodeElement
	RBAssertNoErr(error, CFSTR("AUGraphConnectNodeInput failed to connect scheduled audio player and output audio unit. Error: %d."), error);
	
	error = AUGraphOpen(mAudioUnitGraph);
	RBAssertNoErr(error, CFSTR("AUGraphOpen failed, ohnoez. Error: %d."), error);
	
	
	//Pull out the default stream format.
	UInt32 size = sizeof(mStreamFormat);
	this->CopyPropertyValue((void **)&mStreamFormat, //out value
							&size, //inout valueSize
							kAudioUnitProperty_StreamFormat, //in propertyID
							kAudioUnitScope_Input, //in propertyScope
							mOutputNode); //in node
	
	
	//Set aside the scheduled audio player's audio unit
	mScheduledAudioPlayerUnit = this->GetAudioUnitForNode(mScheduledAudioPlayerNode);
	
	//We use a render notification observer to implement the PlayerKit pulse.
	AudioUnitAddRenderNotify(mScheduledAudioPlayerUnit, //in audioUnit 
							 &PKAudioPlayerEngine::RenderObserverCallback, //in proc
							 this); //in userData
	
	
	//
	//	We observe changes to the default output device because when
	//	the default output device changes the output node's stream format
	//	is reset by the system, wreaking havoc with the scheduled audio player
	//	and pretty much everything else.
	//
	AudioObjectPropertyAddress address = {
		.mSelector = kAudioHardwarePropertyDefaultOutputDevice, 
		.mScope = kAudioObjectPropertyScopeGlobal, 
		.mElement = 0
	};
	error = AudioObjectAddPropertyListener(kAudioObjectSystemObject, //in audioObjectID
										   &address, //in propertyAddressPtr
										   PKAudioPlayerEngine::DefaultAudioDeviceDidChangeListenerProc, //in listenerCallbackProc
										   this); //in listenerCallbackProcUserData
	RBAssertNoErr(error, CFSTR("AudioObjectAddPropertyListener failed. Error %d."), error);
	
	
	//Allocate the scheduled audio player slices that will be used during playback
	RBAtomicCounter *activeSlicesAtomicCounter = new RBAtomicCounter(0);
	for (int index = 0; index < kNumberOfSlicesToKeepActive; index++)
	{
		PKScheduledDataSlice *dataSlice = PKScheduledDataSlice::New(this, //in owner
																	activeSlicesAtomicCounter, //in activeSlicesAtomicCounter
																	NULL /* filled in later */, //in bufferList
																	0 /* filled in later */, //in numberOfFramesPerRead
																	mStreamFormat);
		dataSlice->mDataSliceProgressionNumber = index;
		
		mDataSlices[index] = dataSlice;
	}
	
	//Balance the retain cycles so the AtomicCounter goes away when it should
	activeSlicesAtomicCounter->Release();
}

CFStringRef PKAudioPlayerEngine::CopyDescription()
{
	return CFStringCreateWithFormat(kCFAllocatorDefault, 
									NULL, 
									CFSTR("<%s:%p (%d nodes)>"), mClassName, this, this->GetNumberOfNodes());
}

#pragma mark -
#pragma mark Processing

OSStatus PKAudioPlayerEngine::DefaultAudioDeviceDidChangeListenerProc(AudioObjectID objectID, UInt32 numberAddresses, const AudioObjectPropertyAddress inAddresses[], void *userData)
{
	PKAudioPlayerEngine *self = (PKAudioPlayerEngine *)userData;
	
	try
	{
		Acquisitor lock(self);
		
		bool shouldResumeProcessing = false;
		if(self->IsRunning() && !self->mProcessingIsPaused)
		{
			self->StopGraph();
			
			self->PauseProcessing();
			shouldResumeProcessing = true;
		}
		
		AudioStreamBasicDescription streamFormat = self->GetStreamFormat();
		self->SetStreamFormat(streamFormat);
		
		if(shouldResumeProcessing)
		{
			self->ResumeProcessing(/* preserveExistingSampleBuffers = */ true);
			
			self->StartGraph();
		}
		
		if(self->mOutputDeviceDidChangeHandler)
			self->mOutputDeviceDidChangeHandler();
	}
	catch (RBException e)
	{
		CFShow(CFSTR("RBException raised in PKAudioPlayerEngine::DefaultAudioDeviceDidChangeListenerProc, reason: "));
		CFShow(e.GetReason());
		putc('\n', stderr);
		
		return e.GetCode();
	}
	catch (...)
	{
		std::cerr << "An unknown exception was raised in PKAudioPlayerEngine::DefaultAudioDeviceDidChangeListenerProc." << std::endl;
		return (-1);
	}
	
	return noErr;
}

OSStatus PKAudioPlayerEngine::RenderObserverCallback(void *userData, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
	if(PK_FLAG_IS_SET(*ioActionFlags, kAudioUnitRenderAction_PostRender))
	{
		PKAudioPlayerEngine *self = (PKAudioPlayerEngine *)userData;
		int64_t timeInSeconds = int64_t(inTimeStamp->mSampleTime / self->mStreamFormat.mSampleRate);
		if(timeInSeconds != self->mLastRenderSampleTime)
		{
			self->mLastRenderSampleTime = timeInSeconds;
			
			try
			{
				if(self->mPulseHandler)
					self->mPulseHandler();
			}
			catch (RBException e)
			{
				CFShow(CFSTR("RBException raised in PKAudioPlayerEngine::RenderCallback, reason: "));
				CFShow(e.GetReason());
				putc('\n', stderr);
				
				return e.GetCode();
			}
			catch (...)
			{
				std::cerr << "An unknown exception was raised in PKAudioPlayerEngine::RenderCallback." << std::endl;
				return (-1);
			}
		}
	}
	
	return noErr;
}

#pragma mark -

void PKAudioPlayerEngine::ScheduleSlice(PKScheduledDataSlice *slice)
{
	if(!this->IsRunning() && !mProcessingIsPaused)
		return;
	
	Acquisitor lock(slice, true);
	
	if(!lock.HasAccess())
		return;
	
	//
	//	If the user data we're passed in is invalidated,
	//	its time to cease playback and move on with our lives.
	//
	if(slice->mInvalidated)
		return;
	
	CFErrorRef error = NULL;
	
	//Ask the delegate to give us some samples.
	UInt32 numberOfFramesRead = mScheduleSliceFunctionHandler(this, //in audioUnitGraph
															  slice->mScheduledAudioSlice.mBufferList, //in buffers
															  slice->mNumberOfFramesToRead, //in numberOfFrames
															  &error, //out Error
															  mScheduleSliceFunctionHandlerUserData); //in userData
	
	//
	//	If they don't give us any data, then we've either encountered
	//	a dreadful, dreadful error, or the end of their data source.
	//
	if(numberOfFramesRead == 0)
	{
		if(error)
		{
			Acquisitor lock(this);
			if(!mErrorHasOccurredDuringProcessing)
			{
				//Break the lock so that `slice` can be released in
				//PKAudioPlayerEngine::StopProcessing without erroring.
				lock.Relinquish();
				
				this->StopProcessing();
				this->StopGraph();
				
				mErrorHandler(error);
				
				mErrorHasOccurredDuringProcessing = true;
			}
		}
		else if(slice->mNumberOfActiveSlicesAtomicCounter->GetValue() == 0)
		{
			//Break the lock so that `slice` can be released in
			//PKAudioPlayerEngine::StopProcessing without erroring.
			lock.Relinquish();
			
			this->StopProcessing();
			this->StopGraph();
			
			mEndOfPlaybackHandler();
		}
		
		return;
	}
	
	FillOutAudioTimeStampWithSampleTime(slice->mScheduledAudioSlice.mTimeStamp, mCurrentSampleTime);
	
	//The sample time is just incremented by the number of samples read by the delegate.
#if __LP64__
	OSAtomicAdd64Barrier(numberOfFramesRead, &mCurrentSampleTime);
#else
	OSAtomicAdd32Barrier(numberOfFramesRead, &mCurrentSampleTime);
#endif /* __LP64__ */
	
	slice->mScheduledAudioSlice.mNumberFrames = numberOfFramesRead;
	
	//Make sure its noted that the processing data has information in its buffers.
	slice->mBuffersHaveData = true;
	
	//Here we actually schedule the data we got above.
	OSStatus errorCode = AudioUnitSetProperty(mScheduledAudioPlayerUnit, //in audioUnit
											  kAudioUnitProperty_ScheduleAudioSlice, //in propertyID
											  kAudioUnitScope_Global, //in scope
											  0, //in element
											  &slice->mScheduledAudioSlice, //io data
											  sizeof(ScheduledAudioSlice)); //in dataSize
	
	if(errorCode == noErr)
	{
		slice->mNumberOfActiveSlicesAtomicCounter->Increment();
	}
	else
	{
		this->StopProcessing();
		this->StopGraph();
		
		error = PKCopyError(PKPlaybackErrorDomain, 
							errorCode, 
							NULL, 
							CFSTR("Could not schedule samples for playback. Got error code %ld."), errorCode);
		
		mErrorHandler(error);
	}
}

void PKAudioPlayerEngine::ProcessorDidFinishSlice(PKScheduledDataSlice *dataSlice, ScheduledAudioSlice *bufferList) //called on com.apple.audio.IOThread.client
{
	PKAudioPlayerEngine *self = dataSlice->mOwner;
	if(!self->IsRunning() || (self->mProcessingIsPaused && dataSlice->mInvalidated))
		return;
	
	dataSlice->mNumberOfActiveSlicesAtomicCounter->Decrement();
	dataSlice->mBuffersHaveData = false;
	
	if((bufferList->mFlags & kScheduledAudioSliceFlag_Complete) == kScheduledAudioSliceFlag_Complete)
	{
		self->mSchedulerQueue->Async(PKTaskQueue::TaskProc(&PKScheduledDataSlice::ScheduleSliceTaskProxy), dataSlice);
	}
}

#pragma mark -
#pragma mark Properties

void PKAudioPlayerEngine::SetErrorHandler(ErrorHandler handler) throw()
{
	Acquisitor lock(this);
	
	if(mErrorHandler)
	{
		Block_release(mErrorHandler);
		mErrorHandler = NULL;
	}
	
	if(handler)
		mErrorHandler = Block_copy(handler);
}

PKAudioPlayerEngine::ErrorHandler PKAudioPlayerEngine::GetErrorHandler() const throw()
{
	Acquisitor lock(this);
	
	return mErrorHandler;
}

#pragma mark -

void PKAudioPlayerEngine::SetEndOfPlaybackHandler(EndOfPlaybackHandler handler) throw()
{
	Acquisitor lock(this);
	
	if(mEndOfPlaybackHandler)
	{
		Block_release(mEndOfPlaybackHandler);
		mEndOfPlaybackHandler = NULL;
	}
	
	if(handler)
		mEndOfPlaybackHandler = Block_copy(handler);
}

PKAudioPlayerEngine::EndOfPlaybackHandler PKAudioPlayerEngine::GetEndOfPlaybackHandler() const throw()
{
	Acquisitor lock(this);
	
	return mEndOfPlaybackHandler;
}

#pragma mark -

void PKAudioPlayerEngine::SetPulseHandler(PulseHandler handler) throw()
{
	Acquisitor lock(this);
	
	if(mPulseHandler)
	{
		Block_release(mPulseHandler);
		mPulseHandler = NULL;
	}
	
	if(handler)
		mPulseHandler = Block_copy(handler);
}

PKAudioPlayerEngine::PulseHandler PKAudioPlayerEngine::GetPulseHandler() const throw()
{
	Acquisitor lock(this);
	
	return mPulseHandler;
}

#pragma mark -

void PKAudioPlayerEngine::SetOutputDeviceDidChangeHandler(OutputDeviceDidChangeHandler handler) throw()
{
	Acquisitor lock(this);
	
	if(mOutputDeviceDidChangeHandler)
	{
		Block_release(mOutputDeviceDidChangeHandler);
		mOutputDeviceDidChangeHandler = NULL;
	}
	
	if(handler)
		mOutputDeviceDidChangeHandler = Block_copy(handler);
}

PKAudioPlayerEngine::OutputDeviceDidChangeHandler PKAudioPlayerEngine::GetOutputDeviceDidChangeHandler() const throw()
{
	Acquisitor lock(this);
	
	return mOutputDeviceDidChangeHandler;
}

#pragma mark -

void PKAudioPlayerEngine::SetScheduleSliceFunctionHandler(ScheduleSliceFunctionHandler handler) throw()
{
	Acquisitor lock(this);
	
	mScheduleSliceFunctionHandler = handler;
}

PKAudioPlayerEngine::ScheduleSliceFunctionHandler PKAudioPlayerEngine::GetScheduleSliceFunctionHandler() const throw()
{
	Acquisitor lock(this);
	
	return mScheduleSliceFunctionHandler;
}

void PKAudioPlayerEngine::SetScheduleSliceFunctionHandlerUserData(void *userData) throw()
{
	Acquisitor lock(this);
	
	mScheduleSliceFunctionHandlerUserData = userData;
}

void *PKAudioPlayerEngine::GetScheduleSliceFunctionHandlerUserData() const throw()
{
	Acquisitor lock(this);
	
	return mScheduleSliceFunctionHandlerUserData;
}

#pragma mark -

Float32 PKAudioPlayerEngine::GetAverageCPUUsage() const throw()
{
	Float32 cpuLoad = 0.0f;
	if(AUGraphGetMaxCPULoad(mAudioUnitGraph, &cpuLoad) == noErr)
		return cpuLoad;
	
	return 0.0f;
}

#pragma mark -

Float32 PKAudioPlayerEngine::GetVolume() const throw(RBException)
{
	AudioUnitParameterValue volumeValue = 0.0f;
	CopyParameterValue(&volumeValue, kHALOutputParam_Volume, kAudioUnitScope_Global, mOutputNode);
	return volumeValue;
}

void PKAudioPlayerEngine::SetVolume(Float32 volume) throw(RBException)
{
	SetParameterValue(volume, kHALOutputParam_Volume, kAudioUnitScope_Global, mOutputNode);
}

#pragma mark -
#pragma mark Graph Control

void PKAudioPlayerEngine::StartProcessing() throw(RBException)
{
	Acquisitor lock(this);
	
	//We need all of our handlers to be provided, or playback cannot function.
	RBAssert((mErrorHandler && mEndOfPlaybackHandler && mScheduleSliceFunctionHandler), 
			 CFSTR("One or more handlers has not been set."));
	
	//Firstly, we reset the processor so we don't end up with inconsistent behavior.
	AudioTimeStamp startTimeStamp;
	FillOutAudioTimeStampWithSampleTime(startTimeStamp, -1.0f);
	
	this->SetPropertyValue(&startTimeStamp, //in data
						   sizeof(startTimeStamp), //in dataSize
						   kAudioUnitProperty_ScheduleStartTimeStamp, //in propertyID
						   kAudioUnitScope_Global, //in scope
						   mScheduledAudioPlayerNode); //in node
	
	CAStreamBasicDescription graphFormat = this->GetStreamFormat();
	
	//Then we reset some internal state and setup the slices
	mCurrentSampleTime = 0;
	mErrorHasOccurredDuringProcessing = false;
	
	
	for (int index = 0; index < kNumberOfSlicesToKeepActive; index++)
	{
		PKScheduledDataSlice *dataSlice = mDataSlices[index];
		dataSlice->Acquire();
		
		dataSlice->Reset();
		if((dataSlice->mNumberOfFramesToRead > 0) && (mStreamFormat == dataSlice->mBuffersStreamFormat))
		{
			//Reset the buffer sizes; these sometimes get set to zero during playback.
			AudioBufferList *buffers = dataSlice->mScheduledAudioSlice.mBufferList;
			for (int bufferIndex = 0; bufferIndex < buffers->mNumberBuffers; bufferIndex++)
				buffers->mBuffers[bufferIndex].mDataByteSize = kPKCanonicalBaseBufferSize;
		}
		else
		{
			_DeallocateBuffers(dataSlice->mScheduledAudioSlice.mBufferList);
			dataSlice->mScheduledAudioSlice.mBufferList = _AllocateBuffers(mStreamFormat, kPKCanonicalBaseBufferSize);
			dataSlice->mNumberOfFramesToRead = (kPKCanonicalBaseBufferSize / graphFormat.mBytesPerPacket);
		}
		
		dataSlice->Relinquish();
		
		//
		//	We do all scheduling through the processing queue so we don't need to lock
		//	our delegate's data provider method. It is assumed that said method is non-reentrant.
		//
		mSchedulerQueue->Sync(PKTaskQueue::TaskProc(&PKScheduledDataSlice::ScheduleSliceTaskProxy), dataSlice);
	}
	
	mProcessingIsPaused = false;
}

void PKAudioPlayerEngine::StopProcessing() throw(RBException)
{
	Acquisitor lock(this);
	
	//We first invalidate all of the processing data objects currently in use.
	for (int index = 0; index < kNumberOfSlicesToKeepActive; index++)
	{
		PKScheduledDataSlice *dataSlice = mDataSlices[index];
		dataSlice->Acquire();
		dataSlice->Reset();
		dataSlice->Relinquish();
	}
	
	//
	//	Then we reset all of our audio units. This prevents previously rendered data
	//	that is cached within effects (like matrix reverb) from being replayed which
	//	is a fairly unpleasant experience.
	//
	UInt32 numberOfNodes = this->GetNumberOfNodes();
	for (UInt32 index = 0; index < numberOfNodes; index++)
	{
		AUNode node = this->GetNodeAtIndex(index);
		OSStatus errorCode = AudioUnitReset(this->GetAudioUnitForNode(node), kAudioUnitScope_Global, 0);
		RBAssertNoErr(errorCode, CFSTR("Could not reset node %ld, error %ld."), node, errorCode);
	}
	
	//Reset any paused state.
	mProcessingIsPaused = false;
	
	if(mSortedDataSlicesForPausedProcessing)
	{
		CFRelease(mSortedDataSlicesForPausedProcessing);
		mSortedDataSlicesForPausedProcessing = NULL;
	}
}

#pragma mark -

void PKAudioPlayerEngine::PauseProcessing() throw(RBException)
{
	Acquisitor lock(this);
	
	if(mProcessingIsPaused)
		return;
	
	//This must be set before we do anything else.
	mProcessingIsPaused = true;
	
	if(this->IsRunning())
		return;
	
	AudioTimeStamp currentPlayTime;
	UInt32 size = sizeof(currentPlayTime);
	this->CopyPropertyValue((void **)&currentPlayTime, 
							&size, 
							kAudioUnitProperty_CurrentPlayTime, 
							kAudioUnitScope_Global, 
							mScheduledAudioPlayerNode);
	
	mSortedDataSlicesForPausedProcessing = CFArrayCreateMutable(kCFAllocatorDefault, 0, NULL);
	AudioStreamBasicDescription streamFormat = this->GetStreamFormat();
	void *temporaryBuffer = NULL;
	
	//We 'acquire' all of the processing datas.
	for (int index = 0; index < kNumberOfSlicesToKeepActive; index++)
	{
		PKScheduledDataSlice *dataSlice = mDataSlices[index];
		dataSlice->Acquire();
		
		//We invalidate the processing data so all existing scheduling will cease for this sample.
		dataSlice->mInvalidated = true;
		
		//
		//	If the time stamp of the data slice is less than the current play time, we mark it as having no data.
		//
		if(dataSlice->mScheduledAudioSlice.mTimeStamp.mSampleTime <= currentPlayTime.mSampleTime)
		{
			dataSlice->mBuffersHaveData = false;
		}
		
		//
		//	If the data slice reports that its already begun rendering, we reschedule it.
		//	But first, we need to modify its data so that the user only hears audio they haven't before.
		//
		else if((dataSlice->mScheduledAudioSlice.mFlags & kScheduledAudioSliceFlag_BeganToRender) == kScheduledAudioSliceFlag_BeganToRender)
		{
			//
			//	We calculate the offset in the data slice's internal buffer that we need to
			//	move to. If it returns 0 or -1 (signaling the slice doesn't contain the sample time)
			//	we simply mark it as having no data below.
			//
			long bufferOffsetInFrames = dataSlice->CalculateBufferOffsetInFramesFromSampleTime(currentPlayTime.mSampleTime);
			if(bufferOffsetInFrames > 0)
			{
				//
				//	The temporary buffer is shared between operations and is only
				//	allocated if it is required.
				//
				if(!temporaryBuffer)
				{
					temporaryBuffer = calloc(streamFormat.mBytesPerPacket, kPKCanonicalBaseBufferSize);
					
					//We need this buffer, so if it can't be allocated we explode.
					RBAssert((temporaryBuffer != NULL), 
							 CFSTR("Could not allocate temporary buffer of size %d for PauseProcessing."), kPKCanonicalBaseBufferSize * streamFormat.mBytesPerPacket);
				}
				
				//Calculate the buffer offset in bytes.
				long bufferOffsetInBytes = bufferOffsetInFrames * streamFormat.mBytesPerPacket;
				
				//Enumerate each of the data slice's internal audio buffers and muck about with their data.
				AudioBufferList *buffers = dataSlice->mScheduledAudioSlice.mBufferList;
				for (int bufferIndex = 0; bufferIndex < buffers->mNumberBuffers; bufferIndex++)
				{
					AudioBuffer audioBuffer = buffers->mBuffers[bufferIndex];
					
					//
					//	We need to drop any data that the user has already heard from this buffer.
					//	How we do this is we copy the audio buffer's data [starting at the buffer offset
					//	reported] into the temporary buffer, then we copy the data back from the
					//	temporary buffer into the audio buffer, effectively moving the buffer forward.
					//
					long newBufferSize = audioBuffer.mDataByteSize - bufferOffsetInBytes;
					if(!memcpy(temporaryBuffer, //destination
							   ((char *)(audioBuffer.mData) + bufferOffsetInBytes), //source
							   newBufferSize)) //sourceSize
					{
						free(temporaryBuffer);
						
						RBAssert(false, CFSTR("Could not copy data from audio buffer to temporary buffer for PauseProcessing. Something tells me the apocalypse is upon us. Or we're on the iPhone. Same thing."));
					}
					
					if(!memcpy(audioBuffer.mData, //destination
							   temporaryBuffer, //source
							   newBufferSize)) //sourceSize
					{
						free(temporaryBuffer);
						
						RBAssert(false, CFSTR("Could not copy data from temporary buffer to audio buffer for PauseProcessing. Ohnoz, we can't resume playback now."));
					}
					
					audioBuffer.mDataByteSize = newBufferSize;
				}
				
				dataSlice->mBuffersHaveData = true;
			}
			else
			{
				dataSlice->mBuffersHaveData = false;
			}
		}
		
		//
		//	If the data slice hasn't been rendered already (either partially
		//	or completely) we mark it as having data and move on.
		//
		else
		{
			dataSlice->mBuffersHaveData = true;
		}
		
		dataSlice->Relinquish();
		
		//
		//	We move all of the data slice's to a temporary array.
		//	What are we going to do with this array? Why we're going to sort it!
		//
		CFArrayAppendValue(mSortedDataSlicesForPausedProcessing, dataSlice);
	}
	
	if(temporaryBuffer)
		free(temporaryBuffer);
	
	
	//We have to reset this, or we'll end up breaking end of playback notifications.
	mDataSlices[0]->mNumberOfActiveSlicesAtomicCounter->SetValue(0);
	
	
	//Sort the data slices by their scheduled time.
	CFArraySortValues(mSortedDataSlicesForPausedProcessing, 
					  CFRangeMake(0, CFArrayGetCount(mSortedDataSlicesForPausedProcessing)), 
					  (CFComparatorFunction)PKScheduledDataSlice::Comparator, 
					  this);
	
	
	//Reset the scheduled audio player.
	AudioUnitReset(mScheduledAudioPlayerUnit, kAudioUnitScope_Global, 0);
	
	//Reset the sample time so playback resumes immediately.
	mCurrentSampleTime = 0;
}

void PKAudioPlayerEngine::ResumeProcessing(bool preserveExistingSampleBuffers) throw(RBException)
{
	Acquisitor lock(this);
	
	if(!mProcessingIsPaused)
		return;
	
	RBAssert((mSortedDataSlicesForPausedProcessing != NULL), 
			 CFSTR("Attempting to resume processing when no data slices have been saved. You shouldn't be doing that."));
	
	CFMutableArrayRef dataSlicesToReschedule = CFArrayCreateMutable(kCFAllocatorDefault, 0, NULL);
	
	for (int index = 0; index < CFArrayGetCount(mSortedDataSlicesForPausedProcessing); index++)
	{
		PKScheduledDataSlice *dataSlice = (PKScheduledDataSlice *)CFArrayGetValueAtIndex(mSortedDataSlicesForPausedProcessing, index);
		dataSlice->Acquire();
		
		//We reset the invalidated status of this processing data so we can reuse it.
		dataSlice->mInvalidated = false;
		
		if(preserveExistingSampleBuffers)
		{
			if(dataSlice->mBuffersHaveData)
			{
				//
				//	The sample time is just incremented by the
				//	number of samples read by the delegate.
				//
				FillOutAudioTimeStampWithSampleTime(dataSlice->mScheduledAudioSlice.mTimeStamp, mCurrentSampleTime);
#if __LP64__
				OSAtomicAdd64Barrier(dataSlice->mScheduledAudioSlice.mNumberFrames, &mCurrentSampleTime);
#else
				OSAtomicAdd32Barrier(dataSlice->mScheduledAudioSlice.mNumberFrames, &mCurrentSampleTime);
#endif /* __LP64__ */
				
				//
				//	Here we actually schedule the data we got above.
				//	I don't know why this is done through a property.
				//
				OSStatus errorCode = AudioUnitSetProperty(mScheduledAudioPlayerUnit, //in audioUnit
														  kAudioUnitProperty_ScheduleAudioSlice, //in propertyID
														  kAudioUnitScope_Global, //in scope
														  0, //in element
														  &dataSlice->mScheduledAudioSlice, //io data
														  sizeof(ScheduledAudioSlice)); //in dataSize
				if(errorCode != noErr)
				{
					dataSlice->Relinquish();
					
					RBAssertNoErr(errorCode, CFSTR("AudioUnitSetProperty(kAudioUnitProperty_ScheduleAudioSlice) failed with error %d."), errorCode);
				}
				else
				{
					dataSlice->mNumberOfActiveSlicesAtomicCounter->Increment();
					
					dataSlice->Relinquish();
				}
			}
			else
			{
				dataSlice->Relinquish();
				CFArrayAppendValue(dataSlicesToReschedule, dataSlice);
			}
		}
		else
		{
			dataSlice->Relinquish();
			
			//
			//	We do all scheduling through the processing queue so we don't need to lock
			//	our delegate's data provider method. It is assumed that said method is non-reentrant.
			//
			mSchedulerQueue->Sync(PKTaskQueue::TaskProc(&PKScheduledDataSlice::ScheduleSliceTaskProxy), dataSlice);
		}
	}
	
	for (int index = 0; index < CFArrayGetCount(dataSlicesToReschedule); index++)
	{
		PKScheduledDataSlice *dataSlice = (PKScheduledDataSlice *)CFArrayGetValueAtIndex(dataSlicesToReschedule, index);
		
		//
		//	We do all scheduling through the processing queue so we don't need to lock
		//	our delegate's data provider method. It is assumed that said method is non-reentrant.
		//
		mSchedulerQueue->Sync(PKTaskQueue::TaskProc(&PKScheduledDataSlice::ScheduleSliceTaskProxy), dataSlice);
	}
	
	CFRelease(dataSlicesToReschedule);
	
	CFRelease(mSortedDataSlicesForPausedProcessing);
	mSortedDataSlicesForPausedProcessing = NULL;
	
	AudioTimeStamp startTimeStamp;
	FillOutAudioTimeStampWithSampleTime(startTimeStamp, -1.0f);
	
	this->SetPropertyValue(&startTimeStamp, //in data
						   sizeof(startTimeStamp), //in dataSize
						   kAudioUnitProperty_ScheduleStartTimeStamp, //in propertyID
						   kAudioUnitScope_Global, //in scope
						   mScheduledAudioPlayerNode); //in node
	
	mProcessingIsPaused = false;
	mErrorHasOccurredDuringProcessing = false;
}

#pragma mark -

bool PKAudioPlayerEngine::IsRunning() const throw()
{
	Boolean isRunning = false;
	if(AUGraphIsRunning(mAudioUnitGraph, &isRunning) == noErr)
		return isRunning;
	
	return false;
}

void PKAudioPlayerEngine::StartGraph() throw(RBException)
{
	Acquisitor lock(this);
	
	OSStatus error = AUGraphStart(mAudioUnitGraph);
	RBAssertNoErr(error, CFSTR("Could not start audio graph. Error code %ld."), error);
}

void PKAudioPlayerEngine::StopGraph() throw(RBException)
{
	Acquisitor lock(this);
	
	OSStatus error = AUGraphStop(mAudioUnitGraph);
	RBAssertNoErr(error, CFSTR("Could not stop audio graph. Error code %ld."), error);
}

#pragma mark -
#pragma mark Node Interaction

UInt32 PKAudioPlayerEngine::GetNumberOfNodes() const throw(RBException)
{
	UInt32 numberOfNodes = 0;
	OSStatus error = AUGraphGetNodeCount(mAudioUnitGraph, &numberOfNodes);
	RBAssertNoErr(error, CFSTR("AUGraphGetNodeCount failed. Error %d."), error);
	
	return numberOfNodes;
}

AUNode PKAudioPlayerEngine::GetNodeAtIndex(UInt32 index) const throw(RBException)
{
	AUNode node = 0;
	OSStatus error = AUGraphGetIndNode(mAudioUnitGraph, index, &node);
	RBAssertNoErr(error, CFSTR("AUGraphGetIndNode failed. Error %d."), error);
	
	return node;
}

AudioUnit PKAudioPlayerEngine::GetAudioUnitForNode(AUNode node) const throw(RBException)
{
	RBParameterAssert(node);
	
	AudioUnit audioUnit = NULL;
	OSStatus error = AUGraphNodeInfo(mAudioUnitGraph, node, NULL, &audioUnit);
	RBAssertNoErr(error, CFSTR("AUGraphNodeInfo failed for node %d. Error %d."), node, error);
	
	return audioUnit;
}

CFStringRef PKAudioPlayerEngine::CopyTitleForNode(AUNode node) const throw(RBException)
{
	RBParameterAssert(node);
	
	AudioUnit audioUnit = GetAudioUnitForNode(node);
	
	CFStringRef componentName = NULL;
	OSStatus error = AudioComponentCopyName(AudioComponentInstanceGetComponent(audioUnit), &componentName);
	RBAssertNoErr(error, CFSTR("AudioComponentCopyName failed for node %d. Error %d."), node, error);
	
	return componentName;
}

AudioComponentDescription PKAudioPlayerEngine::GetComponentDescriptionForNode(AUNode node) const throw(RBException)
{
	RBParameterAssert(node);
	
	AudioComponentDescription description;
	bzero(&description, sizeof(description));
	OSStatus error = AUGraphNodeInfo(mAudioUnitGraph, node, &description, NULL);
	RBAssertNoErr(error, CFSTR("AudioComponentCopyName failed for node %d. Error %d."), node, error);
	
	//Even if AUGraphNodeInfo fails, description is already filled with zeros.
	return description;
}

#pragma mark -

void PKAudioPlayerEngine::Initialize() throw(RBException)
{
	Acquisitor lock(this);
	
	OSStatus error = AUGraphInitialize(mAudioUnitGraph);
	RBAssertNoErr(error, CFSTR("AUGraphInitialize failed. Error: %d"), error);
}

void PKAudioPlayerEngine::Uninitialize() throw(RBException)
{
	Acquisitor lock(this);
	
	OSStatus error = AUGraphUninitialize(mAudioUnitGraph);
	RBAssertNoErr(error, CFSTR("AUGraphUninitialize failed. Error: %d"), error);
}

bool PKAudioPlayerEngine::IsInitialized() const
{
	Acquisitor lock(this);
	
	Boolean isInitialized = false;
	AUGraphIsInitialized(mAudioUnitGraph, &isInitialized);
	
	return isInitialized;
}

void PKAudioPlayerEngine::SetStreamFormat(const AudioStreamBasicDescription &inDescription) throw(RBException)
{
	Acquisitor lock(this);
	
	if(this->IsInitialized())
		this->Uninitialize();
	
	mStreamFormat = inDescription;
	
	UInt32 count = this->GetNumberOfNodes();
	for (UInt32 index = 0; index < count; index++)
	{
		AUNode node = this->GetNodeAtIndex(index);
		
		AudioUnitScope scope = kAudioUnitScope_Input;
		if(node == mScheduledAudioPlayerNode)
			scope = kAudioUnitScope_Output;
		
		this->SetPropertyValue(&mStreamFormat, //in value
							   sizeof(mStreamFormat), //in valueSize
							   kAudioUnitProperty_StreamFormat, //in propertyID
							   scope, //in propertyScope
							   node); //in node
	}
	
	this->Initialize();
	this->Update();
}

AudioStreamBasicDescription PKAudioPlayerEngine::GetStreamFormat() const
{
	Acquisitor lock(this);
	
	return mStreamFormat;
}

#pragma mark -

void PKAudioPlayerEngine::SetPropertyValue(const void *inData, UInt32 inSize, AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, AUNode node, AudioUnitElement element) throw(RBException)
{
	AudioUnit audioUnit = GetAudioUnitForNode(node);
	OSStatus error = AudioUnitSetProperty(audioUnit, 
										  inPropertyID, 
										  inScope, 
										  element, 
										  inData? inData : NULL, 
										  inData? inSize : 0);
	RBAssertNoErr(error, CFSTR("AudioUnitSetParameter failed. Error: %d."), error);
}

void PKAudioPlayerEngine::CopyPropertyValue(void **outValue, UInt32 *ioSize, AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, AUNode node, AudioUnitElement element) const throw(RBException)
{
	AudioUnit audioUnit = GetAudioUnitForNode(node);
	OSStatus error = AudioUnitGetProperty(audioUnit, inPropertyID, inScope, element, outValue, ioSize);
	RBAssertNoErr(error, CFSTR("AudioUnitGetProperty failed. Error: %d."), error);
}

void PKAudioPlayerEngine::SetParameterValue(AudioUnitParameterValue inData, AudioUnitParameterID inPropertyID, AudioUnitScope inScope, AUNode node, UInt32 inBufferOffsetInNumberOfFrames) throw(RBException)
{
	AudioUnit audioUnit = GetAudioUnitForNode(node);
	OSStatus error = AudioUnitSetParameter(audioUnit, inPropertyID, inScope, 0, inData, inBufferOffsetInNumberOfFrames);
	RBAssertNoErr(error, CFSTR("AudioUnitSetParameter failed. Error: %d."), error);
}

void PKAudioPlayerEngine::CopyParameterValue(AudioUnitParameterValue *outValue, AudioUnitParameterID inPropertyID, AudioUnitScope inScope, AUNode node) const throw(RBException)
{
	AudioUnit audioUnit = GetAudioUnitForNode(node);
	OSStatus error = AudioUnitGetParameter(audioUnit, inPropertyID, inScope, 0, outValue);
	RBAssertNoErr(error, CFSTR("AudioUnitGetParameter failed. Error: %d."), error);
}

#pragma mark -
#pragma mark Graph Manipulation

bool PKAudioPlayerEngine::Update() throw(RBException)
{
	Acquisitor lock(this);
	
	Boolean isUpdated = false;
	OSStatus error = AUGraphUpdate(mAudioUnitGraph, &isUpdated);
	RBAssertNoErr(error, CFSTR("AUGraphUpdate failed. Error: %d."), error);
	
	return isUpdated;
}

AUNode PKAudioPlayerEngine::AddNode(const AudioComponentDescription *inDescription) throw(RBException)
{
	RBParameterAssert(inDescription);
	
	Acquisitor lock(this);
	
	bool graphWasRunning = this->IsRunning();
	if(graphWasRunning)
	{
		this->StopGraph();
		this->PauseProcessing();
	}
	
	bool graphWasInitialized = this->IsInitialized();
	if(graphWasInitialized)
		this->Uninitialize();
	
	OSStatus error = noErr;
	AUNode newNode = 0;
	
	error = AUGraphAddNode(mAudioUnitGraph, inDescription, &newNode);
	RBAssertNoErr(error, CFSTR("AUGraphAddNode failed. Error %d."), error);
	
	try
	{
		this->SetPropertyValue(&mStreamFormat, //in value
							   sizeof(mStreamFormat), //in valueSize
							   kAudioUnitProperty_StreamFormat, //in propertyID
							   kAudioUnitScope_Input, //in propertyScope
							   newNode); //in node
	}
	catch (...)
	{
		//If we can't set the stream format, its not supported by the audio unit.
		//We simply remove the node we just added, and rethrow this exception.
		AUGraphRemoveNode(mAudioUnitGraph, newNode);
		throw;
	}
	
	AUNode outputNode = this->GetNodeAtIndex(0);
	AUNode nodeAfterOutputNode = this->GetNodeAtIndex(this->GetNumberOfNodes() - 2);
	
	error = AUGraphDisconnectNodeInput(mAudioUnitGraph, outputNode, 0);
	RBAssertNoErr(error, CFSTR("AUGraphDisconnectNodeInput failed for outputNode. Error %d."), error);
	
	error = AUGraphConnectNodeInput(mAudioUnitGraph, newNode, 0, outputNode, 0);
	RBAssertNoErr(error, CFSTR("AUGraphConnectNodeInput failed for conenction between newNode to outputNode. Error %d."), error);
	
	error = AUGraphConnectNodeInput(mAudioUnitGraph, nodeAfterOutputNode, 0, newNode, 0);
	if(error != noErr)
	{
		AUGraphRemoveNode(mAudioUnitGraph, newNode);
		newNode = NULL;
		
		//We do this because, for whatever reason, a failed node connection
		//results in the stream format being reset to factory default.
		this->SetStreamFormat(mStreamFormat);
		
		this->Update();
		
		RBAssertNoErr(error, CFSTR("AUGraphConnectNodeInput failed for conenction between nodeAfterOutputNode to newNode. Error %d."), error);
		
		return NULL;
	}
	
	this->Initialize();
	
	if(graphWasRunning)
	{
		this->ResumeProcessing(/* preserveExistingSampleBuffers = */ true);
		this->StartGraph();
	}
	
	return newNode;
}

void PKAudioPlayerEngine::RemoveNode(AUNode node) throw(RBException)
{
	RBParameterAssert(node);
	RBAssert((node != mOutputNode) && (node != mScheduledAudioPlayerNode), 
			 CFSTR("Attempted to remove output/scheduled audio player node."));
	
	Acquisitor lock(this);
	
	bool graphWasRunning = this->IsRunning();
	if(graphWasRunning)
	{
		this->StopGraph();
		this->PauseProcessing();
	}
	
	//Get the number of interactions
	UInt32 numberOfInteractions = 0;
	AUGraphCountNodeInteractions(mAudioUnitGraph, node, &numberOfInteractions);
	
	//Get the interactions
	AUNodeInteraction interactions[numberOfInteractions]; //This is _not_ Std C++.
	OSStatus error = AUGraphGetNodeInteractions(mAudioUnitGraph, node, &numberOfInteractions, interactions);
	if(error == noErr)
	{
		//Enumerate the interaction so we can get the nodes before
		//and after the node passed to us for re-connection.
		AUNode previousNode = -1;
		AUNode nextNode = -1;
		for (int index = 0; index < numberOfInteractions; index++)
		{
			AUNodeInteraction interaction = interactions[index];
			if(interaction.nodeInteractionType == kAUNodeInteraction_Connection)
			{
				AudioUnitNodeConnection connection = interaction.nodeInteraction.connection;
				if(node == connection.destNode)
				{
					previousNode = connection.sourceNode;
				}
				else if(node == connection.sourceNode)
				{
					nextNode = connection.destNode;
				}
			}
		}
		
		//Break existing node input connections
		AUGraphDisconnectNodeInput(mAudioUnitGraph, node, 0);
		AUGraphDisconnectNodeInput(mAudioUnitGraph, nextNode, 0);
		
		//Remove our node
		AUGraphRemoveNode(mAudioUnitGraph, node);
		
		//Connect the nodes captured above, if possible
		if((previousNode != -1) && (nextNode != -1))
		{
			error = AUGraphConnectNodeInput(mAudioUnitGraph, previousNode, 0, nextNode, 0);
			RBAssertNoErr(error, CFSTR("Could not establish necessary connection to maintain the integrity of the AUGraph. Error %d."), error);
		}
	}
	
	if(graphWasRunning)
	{
		this->ResumeProcessing(/* preserveExistingSampleBuffers = */ true);
		this->StartGraph();
	}
}
