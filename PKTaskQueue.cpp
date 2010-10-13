/*
 *  PKTaskQueue.cpp
 *  PlayerKit
 *
 *  Created by James MacWhinnie on 3/19/10.
 *  Copyright 2010 Roundabout Software. All rights reserved.
 *
 */

#include "PKTaskQueue.h"
#include "CAPThread.h"
#include <objc/objc-auto.h>

#pragma mark PKTaskQueue

static void TaskBlockForwarder(void *userInfo)
{
	PKTaskQueue::TaskBlock block = (PKTaskQueue::TaskBlock)userInfo;
	block();
	Block_release(block);
}

#pragma mark -
#pragma mark Constructor/Destructor

PKTaskQueue::PKTaskQueue(const char *name) :
	RBLockableObject("PKTaskQueue"), 
	
	mName(strdup(name)), 
	
	mQueueTasks(), 
	mIsCancelled(false), 
	
	mSleepSemaphore("PKTaskQueue::mSleepSemaphore"), 
	mProcessingThread(NULL)
{
	
}

PKTaskQueue::~PKTaskQueue()
{
	free((void *)(mName));
	mName = NULL;
}

#pragma mark -
#pragma mark Describing PKTaskQueue

CFStringRef PKTaskQueue::CopyDescription()
{
	return CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("<%s:%p %s>"), mClassName, this, mName);
}

#pragma mark -
#pragma mark Processing Thread

void PKTaskQueue::StartProcessingThread()
{
	Acquisitor lock(this);
	mIsCancelled = false;
	
	if(!mProcessingThread)
	{
		mProcessingThread = new CAPThread(CAPThread::ThreadRoutine(&ProcessingThreadCallback), //in threadRoutine
										  this, //in threadRoutineUserInfo
										  CAPThread::kMaxThreadPriority, //in priority
										  true, //in isPriorityFixed?
										  true); //in shouldAutoDelete?
		mProcessingThread->Start();
	}
}

void PKTaskQueue::StopProcessingThread()
{
	mIsCancelled = true;
	mSleepSemaphore.SignalIf(^(int value) { return value > 1; });
}

#pragma mark -

void *PKTaskQueue::ProcessingThreadCallback(PKTaskQueue *self)
{
	objc_registerThreadWithCollector();
	
	self->Retain();
	
	pthread_setname_np(self->mName);
	
	while (!self->mIsCancelled)
	{
		while (self->mQueueTasks.size() > 0)
		{
			QueuedTask *topTask = NULL;
			{
				Acquisitor lock(self);
				
				topTask = self->mQueueTasks.front();
				self->mQueueTasks.pop();
			}
			
			try
			{
				(*topTask)();
			}
			catch (...)
			{
				std::cerr << "An unexpected exception was raised in the PKTaskQueue named " << self->mName << std::endl;
			}
			
			topTask->Release();
		}
		
		try
		{
			self->mSleepSemaphore.Wait();
		}
		catch (RBException e)
		{
			std::cerr << "mSleepSemaphore.Wait failed with error code " << e.GetCode() << ". Aborting." << std::endl;
		}
	}
	
	self->mProcessingThread = NULL;
	self->Release();
	
	objc_unregisterThreadWithCollector();
	
	return NULL;
}

#pragma mark -
#pragma mark Queuing Tasks

void PKTaskQueue::Async(TaskProc proc, void *userInfo)
{
	RBParameterAssert(proc);
	
	Acquisitor lock(this);
	
	//Start the processing thread if it hasn't been already
	if(!mProcessingThread)
		this->StartProcessingThread();
	
	QueuedTask *task = new QueuedTask(proc, userInfo, true);
	mQueueTasks.push(task);
	
	mSleepSemaphore.SignalIf(^(int value) { return value > 0; });
}

void PKTaskQueue::Async(TaskBlock block)
{
	RBParameterAssert(block);
	
	this->Async(&TaskBlockForwarder, Block_copy(block));
}

#pragma mark -

void PKTaskQueue::Sync(TaskProc proc, void *userInfo)
{
	RBParameterAssert(proc);
	
	Acquisitor lock(this);
	
	//Start the processing thread if it hasn't been already
	if(!mProcessingThread)
		this->StartProcessingThread();
	
	QueuedTask *task = new QueuedTask(proc, userInfo, true);
	
	//We retain the task so it can't be destroyed out from underneath us if it's processed and released
	task->Retain();
	
	mQueueTasks.push(task);
	
	try
	{
		mSleepSemaphore.SignalIf(^(int value) { return value > 0; });
		
		//We relinquish `lock` before waiting for a signal so we don't cause a deadlock
		lock.Relinquish();
		
		task->WaitForApplication();
	}
	catch (...)
	{
		//Balance the above retain
		task->Release();
		throw;
	}
	
	//Balance the above retain
	task->Release();
}

void PKTaskQueue::Sync(TaskBlock block)
{
	RBParameterAssert(block);
	
	this->Sync(&TaskBlockForwarder, Block_copy(block));
}
