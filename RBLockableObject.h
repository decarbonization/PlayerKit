/*
 *  RBLockableObject.h
 *  PlayerKit
 *
 *  Created by James MacWhinnie on 3/19/10.
 *  Copyright 2010 Roundabout Software. All rights reserved.
 *
 */

#ifndef RBLockableObject_h
#define RBLockableObject_h 1

#include "RBObject.h"
#include "RBException.h"

#include <pthread.h>

/*!
 @class
 @abstract	The RBLockableObject class provides a basic object which incorporates
 a simple recursive lock, allowing code to gain exclusive access to an object.
 */
class RBLockableObject : public RBObject
{
protected:
	mutable pthread_mutex_t mMutex;
	pthread_mutexattr_t mMutexAttributes;
	
public:
	
#pragma mark Constructor/Destructor
	
	/*!
	 @abstract	The constructor.
	 */
	RBLockableObject(const char *className = "RBLockableObject");
	
	/*!
	 @abstract	The destructor
	 */
	virtual ~RBLockableObject();
	
#pragma mark -
#pragma mark Locking
	
	/*!
	 @method
	 @abstract		This method should be called when a thread wishes gain exclusive access to an object.
	 @discussion	This method retains the receiver and locks its internal mutex. This method is recursive.
	 */
	void Acquire() const throw(RBException);
	
	/*!
	 @method
	 @abstract	Try to acquire exclusive access to the receiver for the current thread.
	 */
	bool TryToAcquire() const throw(RBException);
	
	/*!
	 @method
	 @abstract		This method should be called when code no longer requires
					exclusive interaction with the receiver.
	 @discussion	This method unlocks the receivers internal mutex, and releases the receiver.
	 */
	void Relinquish() const throw(RBException);
	
#pragma mark -
	
	class Acquisitor
	{
		const RBLockableObject *mObject;
		bool mHasAccess;
		
	public:
		Acquisitor(const RBLockableObject *object, bool useTry = false) :
		mObject(object)
		{
			if(useTry)
			{
				mHasAccess = object->TryToAcquire();
			}
			else
			{
				object->Acquire();
				mHasAccess = true;
			}
		}
		
		~Acquisitor()
		{
			if(mHasAccess)
				mObject->Relinquish();
		}
		
		bool HasAccess() const { return mHasAccess; }
		
		void Relinquish()
		{
			if(mHasAccess)
			{
				mObject->Relinquish();
				mHasAccess = false;
			}
		}
		
	private:
		Acquisitor(Acquisitor &object);
		Acquisitor &operator=(Acquisitor &object);
	};
	
private:
	RBLockableObject(RBLockableObject &object);
	RBLockableObject &operator=(RBLockableObject &object);
};

#endif /* RBLockableObject_h */
