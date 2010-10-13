/*
 *  PKTaskQueue.h
 *  PlayerKit
 *
 *  Created by James MacWhinnie on 3/19/10.
 *  Copyright 2010 Roundabout Software. All rights reserved.
 *
 */

#ifndef PKTaskQueue_h
#define PKTaskQueue_h

#include <queue>
#include <Block.h>

#include "RBObject.h"
#include "RBAtomic.h"
#include "RBException.h"

class CAPThread;

/*!
 @class
 @abstract		The PKTaskQueue class is a simple FIFO task queue that executes tasks on a secondary worker thread.
 @discussion	Internally PKTaskQueue is simply an std::queue, an RBSemaphore, and a CAPThread. When a task
				is added to a queue, it is injected into the std::queue of the receiver, and the RBSemaphore
				is signalled to wake up the CAPThread.
 */
class PKTaskQueue : public RBLockableObject
{
public:
#pragma mark -Public
	
	/*!
	 @typedef
	 @abstract		The prototype functions must match to be used as tasks in PKTaskQueue.
	 @discussion	Any C++ exceptions thrown by a TaskProc will be consumed and ignored.
	 */
	typedef void(*TaskProc)(void *userInfo);
	
	/*!
	 @typedef
	 @abstract		The prototype blocks must match to be used as tasks in PKTaskQueue.
	 @discussion	Any C++ exceptions thrown by a TaskBlock will be consumed and ignored.
	 */
	typedef void(^TaskBlock)();
	
protected:
#pragma mark -Protected
	
	/*!
	 @class
	 @abstract	The QueuedTask functor class is used to represent tasks in PKTaskQueue's task-queue.
	 */
	class QueuedTask : public RBObject
	{
	protected:
		
		TaskProc mImplementation;
		void *mUserInfo;
		RBSemaphore *mSignalSemaphore;
		
	public:
		
#pragma mark Constructor/Destructor
		
		/*!
		 @abstract		The one and only constructor.
		 @param			implementation	The function that provides the implementation of this task
		 @param			userInfo		The value to pass to `implementation`
		 @param			isSynchronous	Whether or not this task is to be executed synchronously
		 @discussion	Synchronous tasks have an internal semaphore that is signalled after their implementation
						is applied. When a synchronous task is queued, `WaitForApplication` should be called on it.
		 */
		QueuedTask(TaskProc implementaton, void *userInfo, bool isSynchronous) : 
			RBObject("PKTaskQueue::QueuedTask"), 
			mImplementation(implementaton), 
			mUserInfo(userInfo), 
			mSignalSemaphore(NULL)
		{
			if(isSynchronous)
				mSignalSemaphore = new RBSemaphore("PKTaskQueue::QueuedTask::mSignalSemaphore");
		}
		
		/*!
		 @abstract		The one and only destructor.
		 @discussion	If the task is synchronous this will signal its semaphore to prevent deadlock.
		 */
		~QueuedTask()
		{
			if(mSignalSemaphore)
			{
				mSignalSemaphore->SignalIf(^(int value) { return value > 1; });
				delete mSignalSemaphore;
			}
		}
		
#pragma mark -
#pragma mark Overloads
		
		/*!
		 @method
		 @abstract		Apply the queued task's implementation.
		 @discussion	If the receiver is synchronous, this will cause the receiver's semaphore to be signalled.
		 */
		void operator()()
		{
			mImplementation(mUserInfo);
			
			if(mSignalSemaphore)
				mSignalSemaphore->SignalIf(^(int value) { return value > 1; });
		}
		
#pragma mark -
#pragma mark Waiting
		
		/*!
		 @method
		 @abstract		Block the calling thread until the receiver is applied.
		 @discussion	This method does nothing for asynchronous tasks.
		 */
		void WaitForApplication() const throw(RBException)
		{
			if(mSignalSemaphore && (mSignalSemaphore->GetInstantaneousValue() == 1))
				mSignalSemaphore->Wait();
		}
	};
	
	/* owner */	const char *mName;
	
	/* n/a */	std::queue</* strong */ QueuedTask *> mQueueTasks;
	/* n/a */	RBAtomicBool mIsCancelled;
	
	/* n/a */	RBSemaphore mSleepSemaphore;
	/* weak */	CAPThread *mProcessingThread;
	
#pragma mark -
#pragma mark Processing Thread
	
	/*!
	 @method
	 @abstract	Start the receiver's processing thread if it isn't already running.
	 */
	void StartProcessingThread();
	
	/*!
	 @method
	 @abstract	Stop the receiver's processing thread if it's running.
	 */
	void StopProcessingThread();
	
#pragma mark -
	
	/*!
	 @method
	 @abstract		The implementation of the processing thread.
	 @discussion	This method processes tasks for task queues.
	 */
	static void *ProcessingThreadCallback(PKTaskQueue *self);
	
public:
#pragma mark -Public
	
#pragma mark Constructor/Destructor
	
	/*!
	 @abstract	The one and only constructor.
	 */
	PKTaskQueue(const char *name = "");
	
	/*!
	 @abstract	The destructor.
	 */
	virtual ~PKTaskQueue();
	
#pragma mark -
#pragma mark Describing PKTaskQueue
	
	virtual CFStringRef CopyDescription();
	
#pragma mark -
#pragma mark Queuing Tasks
	
	/*!
	 @method
	 @abstract		Queue up a task function for asynchronous execution
	 @param			proc		The function to execute asynchronously. May not be NULL.
	 @param			userInfo	The value to pass as the function's parameter.
	 @discussion	If this method is called and no other tasks have been queued in the receiver, the receiver's
					worker thread will be fired as a side effect of the method invocation.
	 */
	void Async(TaskProc proc, void *userInfo);
	
	/*!
	 @method
	 @abstract		Queue up a task block for asynchronous execution
	 @param			block		The block to execute asynchronously. May not be NULL.
	 @discussion	If this method is called and no other tasks have been queued in the receiver, the receiver's
					worker thread will be fired as a side effect of the method invocation.
	 */
	void Async(TaskBlock block);
	
#pragma mark -
	
	/*!
	 @method
	 @abstract		Queue up a task function for synchronous execution
	 @param			proc		The function to execute on the receiver's worker thread. May not be NULL.
	 @param			userInfo	The value to pass as the function's parameter.
	 @discussion	If this method is called and no other tasks have been queued in the receiver, the receiver's
					worker thread will be fired as a side effect of the method invocation.
	 */
	void Sync(TaskProc proc, void *userInfo);
	
	/*!
	 @method
	 @abstract		Queue up a task block for synchronous execution
	 @param			proc		The block to execute on the receiver's worker thread. May not be NULL.
	 @discussion	If this method is called and no other tasks have been queued in the receiver, the receiver's
					worker thread will be fired as a side effect of the method invocation.
	 */
	void Sync(TaskBlock block);
	
};

#endif /* PKTaskQueue_h */
