/*
 *  RBAtomic.h
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 7/8/09.
 *  Copyright 2009 Roundabout Software. All rights reserved.
 *
 */

#ifndef RBAtomic_h
#define RBAtomic_h 1

#include <CoreFoundation/CoreFoundation.h>
#include <libKern/OSAtomic.h>

#include <mach/semaphore.h>
#include <mach/task.h>

#include "RBObject.h"
#include "RBLockableObject.h"
#include "RBException.h"

/*!
 @class
 @abstract		This class represents a simple atomic counter.
 @discussion	This class is used to keep the number of active samples
				synchronized between scheduled audio slices in PKAudioPlayerEngine.
 */
class RBAtomicCounter : public RBLockableObject
{
	int32_t mValue;
public:
	/*!
	 @ctor
	 @abstract	Create an atomic counter with a default value of 0.
	 */
	RBAtomicCounter(int value = 0) :
		RBLockableObject("RBAtomicCounter"),
		mValue(value)
	{
	}
	
	/*!
	 @method
	 @abstract	Increment the receiver.
	 */
	void Increment()
	{
		OSAtomicIncrement32Barrier(&mValue);
	}
	
	/*!
	 @method
	 @abstract	Decrement the receiver.
	 */
	void Decrement()
	{
		OSAtomicDecrement32Barrier(&mValue);
	}
	
	/*!
	 @method
	 @abstract	Add n to the receiver.
	 */
	void Add(int32_t value)
	{
		OSAtomicAdd32Barrier(value, &mValue);
	}
	
	/*!
	 @method
	 @abstract	Set the value of the receiver.
	 */
	void SetValue(int32_t value)
	{
		OSAtomicCompareAndSwap32Barrier(mValue, value, &mValue);
	}
	
	/*!
	 @method
	 @abstract	Get the value of the receiver.
	 */
	int32_t GetValue() const
	{
		OSMemoryBarrier();
		return mValue;
	}
	
	virtual bool IsEqualTo(RBAtomicCounter *other)
	{
		return this->GetValue() == other->GetValue();
	}
};

#pragma mark -

/*!
 @class
 @abstract	RBAtomicBool is a drop in replacement for the built-in C++ bool type
			that uses atomic reads and writes.
 */
class RBAtomicBool : public RBLockableObject
{
protected:
	int32_t mValue;
	
public:
	//! @abstract	The default constructor.
	RBAtomicBool(bool value = false) :
		RBLockableObject("RBAtomicBool"),
		mValue(value? 1 : 0)
	{
	}
	
	//! @abstract	The copy constructor.
	RBAtomicBool(RBAtomicBool &other) :
		RBLockableObject("RBAtomicBool"),
		mValue(other.GetValue()? 1 : 0)
	{
	}
	
	//! @abstract	Set the value of an atomic bool.
	void SetValue(bool value)
	{
		int32_t newValue = value? 1 : 0;
		OSAtomicCompareAndSwap32Barrier(mValue, newValue, &mValue);
	}
	
	//! @abstract	Get the value of an atomic bool.
	bool GetValue() const
	{
		OSMemoryBarrier();
		return (mValue == 1)? true : false;
	}
	
	virtual bool IsEqualTo(RBAtomicBool *other)
	{
		return this->GetValue() == other->GetValue();
	}
	
#pragma mark -
	
	//! @abstract	Convert an atomic bool into a plain bool.
	operator bool() const
	{
		return GetValue();
	}
	
	//! @abstract	Assign a new value to an atomic bool.
	RBAtomicBool &operator=(bool value)
	{
		this->SetValue(value);
		return *this;
	}
	
	//! @abstract	Assign a new value to an atomic bool.
	RBAtomicBool &operator=(RBAtomicBool &value)
	{
		this->SetValue(value.GetValue());
		return *this;
	}
	
	//! @abstract	Whether or not the receiver is equal to another bool.
	bool operator==(RBAtomicBool &other) const
	{
		return GetValue() == other.GetValue();
	}
	
	//! @abstract	Whether or not the receiver is not equal to another bool.
	bool operator!=(RBAtomicBool &other) const
	{
		return GetValue() == other.GetValue();
	}
	
#pragma mark -
	
	//! @abstract	The True atomic bool constant.
	static const RBAtomicBool True;
	
	//! @abstract	The False atomic bool constant.
	static const RBAtomicBool False;
};

#pragma mark -

class RBSemaphore : public RBObject
{
protected:
	
	semaphore_t mMachSemaphore;
	RBAtomicCounter mValue;
	
public:
	
#pragma mark Constructor/Destructor
	
	RBSemaphore(const char *name = "");
	virtual ~RBSemaphore();
	
#pragma mark -
#pragma mark Signalling
	
	void Wait() throw(RBException);
	void Signal() throw(RBException);
	void SignalIf(bool(^predicate)(int value)) throw(RBException);
	
#pragma mark -
#pragma mark Properties
	
	int GetInstantaneousValue() const throw(RBException);
	semaphore_t GetMachSemaphore() const throw();
};

#endif /* RBAtomic_h */
