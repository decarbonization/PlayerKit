/*
 *  PKAudioPlayerEngine.h
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 6/21/09.
 *  Copyright 2009 Roundabout Software. All rights reserved.
 *
 */

#ifndef PKAudioPlayerEngine_h
#define PKAudioPlayerEngine_h 1

#include <CoreFoundation/CoreFoundation.h>
#include <AudioToolbox/AudioToolbox.h>
#include <dispatch/dispatch.h>
#include <Block.h>

#include "RBObject.h"
#include "RBAtomic.h"
#include "RBException.h"

class PKScheduledDataSlice;
class PKTaskQueue;

#pragma mark -

/*!
 @class
 @abstract		This class manages the core of playback in PlayerKit.
				It maintains an AUGraph object, as well as a scheduled audio player.
 @discussion	The PKAudioPlayerEngine class manages almost every aspect of playback with the
				exception of decoding. It is an implementation detail of PlayerKit and should
				never be seen by the outside world.
 */
PK_FINAL class PK_VISIBILITY_HIDDEN PKAudioPlayerEngine : public RBLockableObject
{
public:
#pragma mark • Public
	/*!
	 @typedef
	 @abstract	The prototype an error handler block for PKAudioPlayerEngine should conform to.
	 @param		error	The error that the block should handle. The block is responsible for releasing this error.
	 */
	typedef void(^ErrorHandler)(CFErrorRef error);
	
	/*!
	 @typedef
	 @abstract	The prototype an end of playback handler block for PKAudioPlayerEngine should conform to.
	 */
	typedef void(^EndOfPlaybackHandler)();
	
	/*!
	 @typedef
	 @abstract	The prototype an output device did change handler block for PKAudioPlayerEngine should conform to.
	 */
	typedef void(^OutputDeviceDidChangeHandler)();
	
	/*!
	 @abstract	The prototype a pulse handler block for PKAudioPlayerEngine should conform to.
	 */
	typedef void(^PulseHandler)();
	
	/*!
	 @typedef
	 @abstract		The prototype a schedule slice handler function for PKAudioPlayerEngine should conform to.
	 @param			graph			The graph which is requesting the slices to be scheduled.
	 @param			ioBuffer		The buffer which the function should write its resultant data into.
	 @param			numberOfFrames	The number of frames of data to put into the buffer.
	 @param			outError		If an error occurs, a CFError object should be placed in this object.
	 @result	The number of frames read by the function. Should return 0 to indicate end of playback
					and should return 0 and fill outError to indicate an error.
	 */
	typedef UInt32(*ScheduleSliceFunctionHandler)(PKAudioPlayerEngine *graph, AudioBufferList *ioBuffer, UInt32 numberOfFrames, CFErrorRef *outError, void *userData);
	
private:
#pragma mark -
#pragma mark • Private
	
	/*!
	 @const			kPKNumberOfSlicesToKeepActive
	 @abstract		The number of data slices PKAudioPlayerEngine should keep active during the course of playback.
	 @discussion	Its value should be a power of two.
	 */
	enum {
		//
		//	This is inside of an anonymous enum because we want this enum to be confined to
		//	the scope of PKAudioPlayerEngine and we cannot simply use a const integer for this.
		//
		kNumberOfSlicesToKeepActive = 8
	};
	
	//Basic graph stuff
	/* owner */	AUGraph mAudioUnitGraph;
	/* weak */	AUNode mOutputNode;
	
	/* n/a */	AudioStreamBasicDescription mStreamFormat;
	
	//Callbacks
	/* owner */	ErrorHandler mErrorHandler;
	/* owner */	EndOfPlaybackHandler mEndOfPlaybackHandler;
	/* owner */ OutputDeviceDidChangeHandler mOutputDeviceDidChangeHandler;
	/* owner */ PulseHandler mPulseHandler;
	
	/* owner */	ScheduleSliceFunctionHandler mScheduleSliceFunctionHandler;
	/* n/a */	void *mScheduleSliceFunctionHandlerUserData;
	
	//Scheduled Audio Player things
	/* weak */	AUNode mScheduledAudioPlayerNode;
	/* weak */	AudioUnit mScheduledAudioPlayerUnit;
	
	//Processing
	/* owner */	PKScheduledDataSlice *mDataSlices[kNumberOfSlicesToKeepActive];
	
#ifdef __LP64__
	/* n/a */	int64_t mCurrentSampleTime;
#else
	/* n/a */	int32_t mCurrentSampleTime;
#endif /* __LP64__ */
	
	/* owner */	RBAtomicBool mProcessingIsPaused;
	/* owner */	RBAtomicBool mErrorHasOccurredDuringProcessing;
	/* owner */	CFMutableArrayRef mSortedDataSlicesForPausedProcessing;
	
	/* owner */	PKTaskQueue *mSchedulerQueue;
	
	/* n/a */	int64_t mLastRenderSampleTime;
	
#pragma mark Scheduling
	
	/*!
	 @abstract		Schedule a single slice for playback in the receiver's internal scheduled audio player.
	 @param			dataSlice	The user data that describes both the shared state of all scheduled slices, 
								as well as the individual state of the slice to be scheduled by this method.
	 @discussion	This method is a passive loop. It will schedule a slice for playback, retain the passed in
					processing data, and wait to be called again by the DidFinishSlice callback. It will clean
					up the processing data passed into it if it has been invalidated.
					
					It is only safe to call this method through the audio player engine's scheduler queue. Do
					not call this method directly.
	 */
	void ScheduleSlice(PKScheduledDataSlice *dataSlice);
	
	/*!
	 @abstract		This method is called when a slice has finished playing in a PKAudioPlayerEngine's scheduled audio player.
	 @discussion	This method will attempt to schedule another slice for playback using 'ScheduleSlice.'
	 */
	static void ProcessorDidFinishSlice(PKScheduledDataSlice *dataSlice, ScheduledAudioSlice *bufferList);
	
	/*!
	 @abstract		The listener proc for observing changes to the default output unit.
	 @param			propertyID	Ignored.
	 @param			userData	A pointer to the PKAudioPlayerEngine instance that asked to observe changes.
	 @result		Always noErr, ignored.
	 @discussion	This method will appropriately suspend processing and update the default output units stream format to prevent runtime errors.
	 */
	static OSStatus DefaultAudioDeviceDidChangeListenerProc(AudioObjectID objectID, UInt32 numberAddresses, const AudioObjectPropertyAddress inAddresses[], void *userData);
	
	/*!
	 @abstract		The render callback proc used to observe rendering of the scheduler audio unit.
	 @discussion	This method is used to implement the pulse interface.
	 */
	static OSStatus RenderObserverCallback(void *userData, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData);
	
	/*!
	 @abstract		PKScheduledDataSlice is a friend because we like it when it violates our encapsulation.
	 @discussion	PKScheduledDataSlice takes a pointer to our ProcessorDidFinishSlice member, as such it
					must be a friend or C++ gets all angry.
	 */
	friend class PKScheduledDataSlice;
	
#pragma mark -
	
#pragma mark -
#pragma mark Initialization
	
	/*!
	 @abstract	Initialize the receiver's internal AUGraph.
	 */
	void Initialize() throw(RBException);
	
	/*!
	 @abstract	Uninitialize the receiver's internal AUGraph.
	 */
	void Uninitialize() throw(RBException);
	
	/*!
	 @abstract	Returns whether or not the receiver's internal AUGraph is currently initialized.
	 */
	bool IsInitialized() const;
	
#pragma mark -
#pragma mark Constructors
	
	/*!
	 @abstract		The default constructor.
	 @discussion	This constructor is private so we can strictly control how
					PKAudioPlayerEngine is constructed and how it is subclassed.
	 */
	explicit PKAudioPlayerEngine() throw(RBException);
	
	/*!
	 @abstract	PKAudioPlayerEngine cannot be copied.
	 */
	PKAudioPlayerEngine(PKAudioPlayerEngine &graph);
	
	/*!
	 @abstract	PKAudioPlayerEngine cannot be copied.
	 */
	PKAudioPlayerEngine &operator=(PKAudioPlayerEngine &graph);
	
public:
#pragma mark -
#pragma mark • Public
	
	/*!
	 @abstract	The destructor.
	 */
	~PKAudioPlayerEngine();
	
	/*!
	 @abstract		Create a new audio player engine instance.
	 @discussion	This is the designated 'constructor' for PKAudioPlayerEngine.
	 */
	static PKAudioPlayerEngine *New() throw(RBException)
	{
		return (new PKAudioPlayerEngine);
	}
	
#pragma mark -
	
	/*!
	 @abstract	Copy the description of the receiver.
	 */
	CFStringRef CopyDescription();
	
	/*!
	 @abstract	Get the average CPU usage of the receiver.
	 */
	Float32 GetAverageCPUUsage() const throw();
	
#pragma mark -
#pragma mark Handlers
	
	/*!
	 @abstract		Set the error handler used by the receiver.
	 @discussion	This handler *may not* throw exceptions.
	*/
	void SetErrorHandler(ErrorHandler handler) throw();
	
	//! @abstract	Get the error handler used by the receiver.
	ErrorHandler GetErrorHandler() const throw();
	
#pragma mark -
	
	//! @abstract	Set the end of playback handler used by the receiver.
	void SetEndOfPlaybackHandler(EndOfPlaybackHandler handler) throw();
	
	//! @abstract	Get the end of playback handler used by the receiver.
	EndOfPlaybackHandler GetEndOfPlaybackHandler() const throw();
	
#pragma mark -
	
	/*!
	 @abstract		Set the end of output device changed handler used by the receiver.
	 @discussion	Any exceptions thrown in the context of this handler will be caught and ignored.
	*/
	void SetOutputDeviceDidChangeHandler(OutputDeviceDidChangeHandler handler) throw();
	
	//! @abstract	Get the end of output device changed handler used by the receiver.
	OutputDeviceDidChangeHandler GetOutputDeviceDidChangeHandler() const throw();
	
#pragma mark -
	
	/*!
	 @abstract		Set the pulse handler used by the receiver.
	 @discussion	The handler will be invoked from a background thread managed by core audio.
					Any exceptions thrown in the context of this handler will be caught and ignored.
	 */
	void SetPulseHandler(PulseHandler handler) throw();
	
	//! @abstract	Get the pulse handler used by the receiver.
	PulseHandler GetPulseHandler() const throw();
	
#pragma mark -
	
	/*!
	 @abstract		Set the schedule slice function handler used by the receiver.
	 @discussion	This handler *may not* throw exceptions.
	 */
	void SetScheduleSliceFunctionHandler(ScheduleSliceFunctionHandler handler) throw();
	
	//! @abstract	Get the schedule slice function handler used by the receiver.
	ScheduleSliceFunctionHandler GetScheduleSliceFunctionHandler() const throw();
	
	//! @abstract	Set the schedule slice function handler user data used by the receiver.
	void SetScheduleSliceFunctionHandlerUserData(void *userData) throw();
	
	//! @abstract	Set the schedule slice function handler user data used by the receiver.
	void *GetScheduleSliceFunctionHandlerUserData() const throw();
	
#pragma mark -
#pragma mark Volume
	
	/*!
	 @abstract	Get the volume level of the receiver.
	 */
	Float32 GetVolume() const throw(RBException);
	
	/*!
	 @abstract	Set the volume level of the receiver.
	 */
	void SetVolume(Float32 volume) throw(RBException);
	
#pragma mark -
#pragma mark Controlling Processing
	
	/*!
	 @abstract	Start the processing loop and populate the scheduled audio player with samples to play.
	 */
	void StartProcessing() throw(RBException);
	
	/*!
	 @abstract	Stop the processing loop and clean up all scheduled samples.
	 */
	void StopProcessing() throw(RBException);
	
	/*!
	 @abstract		Pause processing temporarily. This method is designed to make seeking possible for the
					audio unit graph's delegate as it resets the scheduled audio player and drops
					all samples currently scheduled for playback.
	 @discussion	A call to this method should _always_ be balanced with a call to 'ResumeProcessing.'
					Before calling 'PauseProcessing,' one should call 'Acquire' on the receiver
					to ensure that it will not be modified while processing is paused.
	 */
	void PauseProcessing() throw(RBException);
	
	/*!
	 @abstract		Resume processing. This method is designed to balance the actions of 'PauseProcessing,'
					it will repopulate the scheduled audio player with samples and reset the sample timing
					information.
	 @param			preserveExistingSampleBuffers	If true, ResumeProcessing will attempt to preserve
													any data that is in the scheduled slices, if not
													possible it will generate new data.
	 @discussion	After calling 'ResumeProcessing,' one should call Relinquish on the receiver
					to allow modifications to be possible again. Failing to do so (assuming one
					called 'Acquire' before) will result in deadlocks.
	 */
	void ResumeProcessing(bool preserveExistingSampleBuffers = false) throw(RBException);
	
#pragma mark -
#pragma mark Starting/Stopping Graph
	
	/*!
	 @abstract	Whether or not the graph in the receiver is running.
	 */
	bool IsRunning() const throw();
	
	/*!
	 @abstract		Start the receiver's internal AUGraph.
	 @discussion	A call to this method is generally followed by a call to StartProcessing.
					This method is guaranteed to never raise an exception.
	 */
	void StartGraph() throw(RBException);
	
	/*!
	 @abstract		Stop the receiver's internal AUGraph.
	 @discussion	A call to this method is generally followed by a call to StopProcessing.
					This method is guaranteed to never raise an exception.
	 */
	void StopGraph() throw(RBException);
	
#pragma mark -
#pragma mark Node Interaction
	
	/*!
	 @abstract	Get the number of nodes in the receiver's AUGraph.
	 */
	UInt32 GetNumberOfNodes() const throw(RBException);
	
	/*!
	 @abstract	Get a node by index in the receiver's AUGraph.
	 */
	AUNode GetNodeAtIndex(UInt32 index) const throw(RBException);
	
	/*!
	 @abstract	Get the internal AudioUnit for a node in the receiver's AUGraph.
	 */
	AudioUnit GetAudioUnitForNode(AUNode node) const throw(RBException);
	
	/*!
	 @abstract	Get the title for a node in the receiver's AUGraph.
	 */
	CFStringRef CopyTitleForNode(AUNode node) const throw(RBException);
	
	/*!
	 @abstract	Get the component description for a node in the receiver's AUGraph.
	 */
	AudioComponentDescription GetComponentDescriptionForNode(AUNode node) const throw(RBException);
	
#pragma mark -
	
	/*!
	 @abstract		Set the stream format used for the data passed into the receiver through the delegate.
	 @discussion	This method will set the stream format for all of the nodes in the receiver's AUGraph,
					and will raise if any of the nodes do not support the format.
	 */
	void SetStreamFormat(const AudioStreamBasicDescription &inDescription) throw(RBException);
	
	/*!
	 @abstract		Get the stream format of the receiver.
	 */
	AudioStreamBasicDescription GetStreamFormat() const;
	
#pragma mark -
#pragma mark Property/Parameter Setters/Getters
	
	/*!
	 @abstract	Update the value of a property for a node in the receiver's AUGraph.
	 @param		inData			The new value to be assigned to the property.
	 @param		inSize			The size of the value.
	 @param		inPropertyID	The name of the property the value is being assigned to.
	 @param		inScope			The scope of the property the value is being applied to.
	 @param		node			The node that this value is being applied to.
	 */
	void SetPropertyValue(const void *inData, UInt32 inSize, AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, AUNode node, AudioUnitElement element = 0) throw(RBException);
	
	/*!
	 @abstract	Copy the value of a property for a node in the receiver's AUGraph.
	 @param		outValue		On return, a reference to the value of the property.
	 @param		ioSize			On input, the size of the buffer provided for outValue, on return, the size of outValue.
	 @param		inPropertyID	The name of the property to copy the value from.
	 @param		inScope			The scope of the property to copy the value from.
	 @param		node			The node that this value is being copied from.
	 */
	void CopyPropertyValue(void **outValue, UInt32 *ioSize, AudioUnitPropertyID inPropertyID, AudioUnitScope inScope, AUNode node, AudioUnitElement element = 0) const throw(RBException);
	
	/*!
	 @abstract	Update the value of a parameter for a node in the receiver's AUGraph.
	 @param		inData							The new value to be assigned to the parameter.
	 @param		inPropertyID					The name of the parameter the value is being assigned to.
	 @param		inScope							The scope of the parameter the value is being applied to.
	 @param		node							The node that this value is being applied to.
	 @param		inBufferOffsetInNumberOfFrames	The buffer offset in frames that this parameter is being applied to. Default is 0.
	 */
	void SetParameterValue(AudioUnitParameterValue inData, AudioUnitParameterID inPropertyID, AudioUnitScope inScope, AUNode node, UInt32 inBufferOffsetInNumberOfFrames = 0) throw(RBException);
	
	/*!
	 @abstract	Copy the value of a parameter for a node in the receiver's AUGraph.
	 @param		outValue		On return, a reference to the value of the parameter.
	 @param		inPropertyID	The name of the parameter to copy the value from.
	 @param		inScope			The scope of the parameter to copy the value from.
	 @param		node			The node that this value is being copied from.
	 */
	void CopyParameterValue(AudioUnitParameterValue *outValue, AudioUnitParameterID inPropertyID, AudioUnitScope inScope, AUNode node) const throw(RBException);
	
#pragma mark -
#pragma mark Adding/Removing Nodes
	
	/*!
	 @abstract		Tell the receiver's internal AUGraph to update.
	 @discussion	It is typically not necessary to call this method. PKAudioPlayerEngine will call this method
					whenever it is necessary to do so, and additional calls might yield errors.
	 */
	bool Update() throw(RBException);
	
	/*!
	 @abstract		Add a new node to the receiver's internal AUGraph.
	 @discussion	This method will properly inject the new node in the receiver so that it does not
					interfere with the receiver's internal scheduled audio player, and so that its
					effect on the audio the graph produces is heard by the user.
	 */
	AUNode AddNode(const AudioComponentDescription *inDescription) throw(RBException);
	
	/*!
	 @abstract		Remove a node from the receiver's internal AUGraph.
	 @discussion	This method will do all of the dirty work of making sure the receiver's
					AUGraph is in a consistent state, even if removing the node fails.
	 */
	void RemoveNode(AUNode node) throw(RBException);
};

#endif /* PKAudioPlayerEngine_h */
