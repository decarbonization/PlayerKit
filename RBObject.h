//
//  RBObject.h
//  PlayerKit
//
//  Created by Peter MacWhinnie on 6/23/09.
//  Copyright 2009 Roundabout Software. All rights reserved.
//

#ifndef RBObject_h
#define RBObject_h 1

#include <CoreFoundation/CoreFoundation.h>
#include <libKern/OSAtomic.h>
#include <iostream>

#ifndef PK_FINAL
#	define PK_FINAL
#endif /* PK_FINAL */

#ifndef PK_PURE_VIRTUAL
#	define PK_PURE_VIRTUAL =0
#endif /* PK_PURE_VIRTUAL */

#if !defined(PK_VISIBILITY_PUBLIC) && !defined(PK_VISIBILITY_HIDDEN)
#	if __GNUC__ >= 4
#		define PK_VISIBILITY_PUBLIC __attribute__((visibility("default")))
#		define PK_VISIBILITY_HIDDEN  __attribute__((visibility("hidden")))
#	else
#		define PK_VISIBILITY_PUBLIC
#		define PK_VISIBILITY_HIDDEN
#	endif /* __GNUC__ >= 4 */
#endif /* !defined(PK_VISIBILITY_PUBLIC) && !defined(PK_VISIBILITY_HIDDEN) */

/*!
 @class
 @abstract	The RBObject class provides a common base for objects in PlayerKit.
 */
class RBObject
{
protected:
	const char *mClassName;
	mutable int32_t mRetainCount;
	
public:
	/*!
	 @ctor
	 @abstract		The default constructor.
	 @param			className	The name of class this object belongs to.
	 @discussion	This constructor should only be used by subclasses of RBObject,
	 which should pass in their name so RBObject can provide an
	 appropriate description..
	 */
	RBObject(const char *className = "RBObject") :
	mRetainCount(1),
	mClassName(strdup(className))
	{
		
	}
	
	virtual ~RBObject()
	{
		if(mClassName)
		{
			free((void *)(mClassName));
			mClassName = NULL;
		}
	}
	
#pragma mark Retain Counts
	
	/*!
	 @method
	 @abstract	The retain count of the object.
	 */
	int32_t GetRetainCount() const { return mRetainCount; }
	
	/*!
	 @method
	 @abstract	Retain the receiver. This method is thread safe.
	 */
	void Retain() const
	{
		OSAtomicIncrement32((volatile int32_t *)&mRetainCount);
	}
	
	/*!
	 @method
	 @abstract	Release the receiver, deleting it its retain count is 0. This method is thread safe.
	 */
	void Release() const
	{
		OSAtomicDecrement32((volatile int32_t *)&mRetainCount);
		if(mRetainCount == 0)
			delete this;
	}
	
#pragma mark Describing
	
	/*!
	 @method
	 @abstract	Copy the description of the receiver.
	 */
	virtual CFStringRef CopyDescription()
	{
		return CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("<%s:%p>"), mClassName, this);
	}
	
	/*!
	 @method
	 @abstract	Get the class name of the receiver.
	 @discussion	The receiver owns the value returned by this method.
	 */
	virtual const char *GetClassName() const { return mClassName; }
	
#pragma mark Equality
	
	/*!
	 @method
	 @abstract	Whether or not the receiver is equal to another RBObject.
	 */
	virtual bool IsEqualTo(RBObject *other)
	{
		return this == other;
	}
	
private:
	RBObject(RBObject &object);
	RBObject &operator=(RBObject &object);
};

#endif /* RBObject_h */
