/*
 *  PKScheduledDataSlice
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 9/22/09.
 *  Copyright 2009 Roundabout Software. All rights reserved.
 *
 */

#include "PKScheduledDataSlice.h"

#include "CAComponent.h"
#include "CAComponentDescription.h"
#include "CAAudioBufferList.h"
#include "CAStreamBasicDescription.h"

#include <iostream>

#pragma mark PKScheduledDataSlice

PKScheduledDataSlice::PKScheduledDataSlice(PKAudioPlayerEngine *owner, 
										   RBAtomicCounter *atomicCounter, 
										   AudioBufferList *bufferList, 
										   UInt32 numberOfFrames, 
										   const AudioStreamBasicDescription &streamFormat) :
	RBLockableObject("PKScheduledDataSlice"),
	mOwner(owner),
	mNumberOfActiveSlicesAtomicCounter(atomicCounter), 
	mBuffersStreamFormat(streamFormat), 
	mDataSliceProgressionNumber(0),
	mNumberOfFramesToRead(numberOfFrames),
	mBuffersHaveData(false),
	mInvalidated(false)
{
	mNumberOfActiveSlicesAtomicCounter->Retain();
	
	bzero(&mScheduledAudioSlice, sizeof(ScheduledAudioSlice));
	bzero(&mScheduledAudioSlice.mTimeStamp, sizeof(AudioTimeStamp));
	
	mScheduledAudioSlice.mBufferList = bufferList;
	mScheduledAudioSlice.mNumberFrames = numberOfFrames;
	mScheduledAudioSlice.mCompletionProc = ScheduledAudioSliceCompletionProc(&PKAudioPlayerEngine::ProcessorDidFinishSlice);
	mScheduledAudioSlice.mCompletionProcUserData = this;
}

PKScheduledDataSlice::~PKScheduledDataSlice()
{
	AudioBufferList *buffers = mScheduledAudioSlice.mBufferList;
	for (int index = 0; index < buffers->mNumberBuffers; index++)
		free(buffers->mBuffers[index].mData);
	
	CAAudioBufferList::Destroy(buffers);
	
	mNumberOfActiveSlicesAtomicCounter->Release();
}

#pragma mark -

long PKScheduledDataSlice::CalculateBufferOffsetInFramesFromSampleTime(Float64 sampleTime) const
{
	Float64 sliceTime = mScheduledAudioSlice.mTimeStamp.mSampleTime;
	Float64 sliceDuration = mScheduledAudioSlice.mTimeStamp.mSampleTime + mScheduledAudioSlice.mNumberFrames;
	
	if((sampleTime >= sliceTime) && (sampleTime <= sliceDuration))
		return long(mScheduledAudioSlice.mNumberFrames - (sliceDuration - sampleTime));
	
	return -1;
}

CFComparisonResult PKScheduledDataSlice::Comparator(PKScheduledDataSlice *left, PKScheduledDataSlice *right, PKAudioPlayerEngine *self)
{
	Float64 leftSampleTime = left->mScheduledAudioSlice.mTimeStamp.mSampleTime;
	Float64 rightSampleTime = right->mScheduledAudioSlice.mTimeStamp.mSampleTime;
	
	if(leftSampleTime < rightSampleTime)
		return kCFCompareLessThan;
	else if(leftSampleTime > rightSampleTime)
		return kCFCompareGreaterThan;
	
	return kCFCompareEqualTo;
}

void PKScheduledDataSlice::ScheduleSliceTaskProxy(PKScheduledDataSlice *self) throw(RBException)
{
	self->mOwner->ScheduleSlice(self);
}

void PKScheduledDataSlice::Reset()
{
	Acquisitor lock(this);
	
	memset(&mScheduledAudioSlice.mTimeStamp, 0, sizeof(AudioTimeStamp));
	
	mNumberOfActiveSlicesAtomicCounter->SetValue(0);
	mBuffersHaveData = false;
	mInvalidated = false;
}
