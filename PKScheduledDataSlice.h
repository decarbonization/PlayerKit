/*
 *  PKScheduledDataSlice.h
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 9/22/09.
 *  Copyright 2009 Roundabout Software. All rights reserved.
 *
 */

#ifndef PKScheduledDataSlice_h
#define PKScheduledDataSlice_h 1

#include <AudioToolbox/AudioToolbox.h>

#include "RBObject.h"
#include "RBAtomic.h"
#include "PKAudioPlayerEngine.h"

/*!
 @class
 @abstract		This class is used to represent the state of a slice in PKAudioPlayerEngine's scheduled audio player system.
 @discussion	The PKScheduledDataSlice class is more or less designed to be used like a struct.
				It is expected that code that uses it will read and write to its members directly.
				However, it is important to 'Acquire' (that is, lock) a PKScheduledDataSlice object
				before reading or writing from it, and to 'Relinquish' (unlock) it after you are finished.
 */
PK_FINAL class PK_VISIBILITY_HIDDEN PKScheduledDataSlice : public RBLockableObject
{
public:
#pragma mark -
#pragma mark • Public
	/*!
	 @abstract	The owner of the processing data.
	 */
	/* weak */	PKAudioPlayerEngine *mOwner;
	
	/*!
	 @abstract		The atomic AtomicCounter that indicates the number of active slices currently
					in use. When this value reaches zero, playback is considered to be finished..
	 @discussion	This value is shared between many processing data objects.
	 */
	/* owner */	RBAtomicCounter *mNumberOfActiveSlicesAtomicCounter;
	
	/*!
	 @abstract	The number of the processing data. Multiple processing data
				objects often exist in parallel, and this number indicates
				which of a number of datas the 'receiver' is.
	 */
	/* n/a */	UInt32 mDataSliceProgressionNumber;
	
	/*!
	 @abstract	The scheduled audio slice of the processing data.
	 */
	/* owner */	ScheduledAudioSlice mScheduledAudioSlice;
	
	/*!
	 @abstract	The format of the buffers of the scheduled data slice.
	 */
	/* n/a */	AudioStreamBasicDescription mBuffersStreamFormat;
	
	/*!
	 @abstract	The number of frames to read when scheduling a slice.
	 */
	/* n/a */	UInt32 mNumberOfFramesToRead;
	
	/*!
	 @abstract		Whether or not the processing data has been invalidated.
	 @discussion	The processing loop will release a processing data object if it has been invalidated,
					thereby completing its lifecycle.
	 */
	/* n/a */	bool mInvalidated;
	
	/*!
	 @abstract	Whether or not this processing data has been populated with data.
	 */
	/* owner */	RBAtomicBool mBuffersHaveData;
	
	/*!
	 @abstract		Create a new scheduled data slice instance.
	 @discussion	This is the designated 'constructor' for PKAudioPlayerEngine.
	 */
	static PKScheduledDataSlice *New(PKAudioPlayerEngine *owner, 
									 RBAtomicCounter *atomicCounter, 
									 AudioBufferList *bufferList, 
									 UInt32 numberOfFrames,
									 const AudioStreamBasicDescription &streamFormat)
	{
		return (new PKScheduledDataSlice(owner, atomicCounter, bufferList, numberOfFrames, streamFormat));
	}
	
	/*!
	 @abstract		The destructor.
	 @discussion	The destructor will release any of its shared data and destroy its internal buffers.
	 */
	~PKScheduledDataSlice();
	
#pragma mark -
#pragma mark Utility Methods
	
	/*!
	 @abstract	Calculate the offset in the receiver's buffers relative to [global] sample time.
	 @param		sampleTime	A sample time relative to the slice's owners playback-start-time.
	 @result	-1 if the passed in sample time is not contained in the receiver; The offset of the buffers otherwise.
	 */
	long CalculateBufferOffsetInFramesFromSampleTime(Float64 sampleTime) const;
	
	/*!
	 @abstract	A comparator that sorts an array based on the scheduled location of slices.
	*/
	static CFComparisonResult Comparator(PKScheduledDataSlice *left, PKScheduledDataSlice *right, PKAudioPlayerEngine *self);
	
	/*!
	 @abstract	This static method is provided for use with PKTaskQueue. The data slice instance
				passed in as the first parameter is scheduled in the data slice's owner.
	 */
	static void ScheduleSliceTaskProxy(PKScheduledDataSlice *self) throw(RBException);
	
	/*!
	 @abstract	Reset the scheduled data slice's state.
	 */
	void Reset();
	
private:
#pragma mark -
#pragma mark • Private
	
	/*!
	 @abstract	The default constructor.
	 @param		owner			The owner of this processing data object. The owner of this object will
								ensure that it is properly released when the time comes.
	 @param		atomicCounter	The AtomicCounter to use to communicate the number of active samples between
								several DataSlice objects.
	 @param		bufferList		The buffers the DataSlice object is to use to schedule samples for
								playback. It takes over ownership of the buffers and will destroy them
								when it is destructed.
	 @param		numberOfFrames	The number of frames of data to write into the buffer list when scheduling
								a sample for playback.
	 @param		streamFormat	The audio stream format of the bufferList.
	 */
	explicit PKScheduledDataSlice(PKAudioPlayerEngine *owner, 
								  RBAtomicCounter *atomicCounter, 
								  AudioBufferList *bufferList, 
								  UInt32 numberOfFrames, 
								  const AudioStreamBasicDescription &streamFormat);
	
	/*!
	 @abstract	PKScheduledDataSlice cannot be copied.
	 */
	PKScheduledDataSlice(PKScheduledDataSlice &slice);
	
	/*!
	 @abstract	PKScheduledDataSlice cannot be copied.
	 */
	PKScheduledDataSlice &operator=(PKScheduledDataSlice &slice);
};

#endif /* PKScheduledDataSlice_h */
