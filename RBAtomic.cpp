/*
 *  RBAtomic.cpp
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 7/8/09.
 *  Copyright 2009 Roundabout Software. All rights reserved.
 *
 */

#include "RBAtomic.h"
#include <mach/mach_init.h>

const RBAtomicBool RBAtomicBool::True(true);
const RBAtomicBool RBAtomicBool::False(false);

#pragma mark RBSemaphore

#pragma mark Constructor/Destructor

RBSemaphore::RBSemaphore(const char *name) : 
	RBObject("RBSemaphore"),
	mMachSemaphore(NULL),
	mValue(0)
{
	int errorCode = semaphore_create(mach_task_self(), &mMachSemaphore, SYNC_POLICY_FIFO, 0);
	RBAssertNoErr(errorCode, CFSTR("semaphore_create failed with error %d"), errorCode);
}

RBSemaphore::~RBSemaphore()
{
	if(mMachSemaphore)
	{
		semaphore_destroy(mach_task_self(), mMachSemaphore);
		mMachSemaphore = NULL;
	}
}

#pragma mark -
#pragma mark Signalling

void RBSemaphore::Wait() throw(RBException)
{
	mValue.Increment();
	
	int errorCode = semaphore_wait(mMachSemaphore);
	RBAssertNoErr(errorCode, CFSTR("semaphore_wait failed with error code %d"), errorCode);
}

void RBSemaphore::Signal() throw(RBException)
{
	mValue.Decrement();
	
	int errorCode = semaphore_signal(mMachSemaphore);
	RBAssertNoErr(errorCode, CFSTR("semaphore_signal failed with error code %d"), errorCode);
}

void RBSemaphore::SignalIf(bool(^predicate)(int value)) throw(RBException)
{
	if(predicate(this->GetInstantaneousValue()))
	   this->Signal();
}

#pragma mark -
#pragma mark Properties

int RBSemaphore::GetInstantaneousValue() const throw(RBException)
{
	return mValue.GetValue();
}

semaphore_t RBSemaphore::GetMachSemaphore() const throw()
{
	return mMachSemaphore;
}
