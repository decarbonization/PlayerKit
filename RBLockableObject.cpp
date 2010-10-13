/*
 *  RBLockableObject.cpp
 *  PlayerKit
 *
 *  Created by James MacWhinnie on 3/19/10.
 *  Copyright 2010 Roundabout Software. All rights reserved.
 *
 */

#include "RBLockableObject.h"

RBLockableObject::RBLockableObject(const char *className) :
	RBObject(className),
	mMutex((pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER)
{
	int errorCode = pthread_mutexattr_init(&mMutexAttributes);
	RBAssertNoErr(errorCode, CFSTR("pthread_mutexattr_init failed with error code %d"), errorCode);
	
	errorCode = pthread_mutexattr_settype(&mMutexAttributes, PTHREAD_MUTEX_RECURSIVE);
	RBAssertNoErr(errorCode, CFSTR("pthread_mutexattr_settype failed with error code %d"), errorCode);
	
	errorCode = pthread_mutex_init(&mMutex, &mMutexAttributes);
	RBAssertNoErr(errorCode, CFSTR("pthread_mutex_init failed with error code %d"), errorCode);
}

RBLockableObject::~RBLockableObject()
{
	int errorCode = pthread_mutex_destroy(&mMutex);
	RBAssertNoErr(errorCode, CFSTR("pthread_mutexattr_destroy failed with error code %d"), errorCode);
	
	errorCode = pthread_mutexattr_destroy(&mMutexAttributes);
	RBAssertNoErr(errorCode, CFSTR("pthread_mutexattr_destroy failed with error code %d"), errorCode);
}

#pragma mark -
#pragma mark Locking

void RBLockableObject::Acquire() const throw(RBException)
{
	int errorCode = pthread_mutex_lock(&mMutex);
	RBAssertNoErr(errorCode, CFSTR("pthread_mutex_unlock failed with error %d"), errorCode);
	
	this->Retain();
}

bool RBLockableObject::TryToAcquire() const throw(RBException)
{
	int errorCode = pthread_mutex_trylock(&mMutex);
	if(errorCode == noErr)
	{
		this->Retain();
		return true;
	}
	else if(errorCode != EBUSY)
	{
		RBAssertNoErr(errorCode, CFSTR("pthread_mutex_trylock failed with error %d"), errorCode);
	}
	
	return false;
}

void RBLockableObject::Relinquish() const throw(RBException)
{
	int errorCode = pthread_mutex_unlock(&mMutex);
	
	//We eat EPERM errors because this method is a no-op when the receiver is not locked.
	if(errorCode != 0 && errorCode != EPERM)
		RBAssertNoErr(errorCode, CFSTR("pthread_mutex_unlock failed with error %d"), errorCode);
	
	this->Release();
}
