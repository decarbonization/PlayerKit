/*
 *  RBException.h
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 5/18/09.
 *  Copyright 2009 Roundabout Software. All rights reserved.
 *
 */

#ifndef RBException_h
#define RBException_h 1

#include <CoreFoundation/CoreFoundation.h>
#include "RBObject.h"

/*!
 @defined	RBAssert
 @abstract	Generates an exception if a given condition is false.
 @param		condition	The condition to evaluate for truthiness.
 @param		description	The message to print if the assertion fails.
 @param		...			The format data associated with the message.
 */
#define RBAssert(condition, description, ...) ({ if(!condition) RBException::HandleFailureInFunction(__PRETTY_FUNCTION__, __FILE__, __LINE__, RBException::DefaultErrorCode, description, ##__VA_ARGS__); })

/*!
 @defined	RBAssertNoErr
 @abstract	Generates an exception if a given error code does not equal noErr.
 @param		error		The error code to check.
 @param		description	The message to print if the assertion fails.
 @param		...			The foramt data associated with the message.
 */
#define RBAssertNoErr(error, description, ...) ({ if(error != noErr) RBException::HandleFailureInFunction(__PRETTY_FUNCTION__, __FILE__, __LINE__, error, description, ##__VA_ARGS__); })

/*!
 @defined	RBParameterAssert
 @abstract	Generates an exception if the parameter given is null.
 */
#define RBParameterAssert(parameter) RBAssert(parameter, CFSTR("Invalid parameter not satisfying: %s"), #parameter);

#pragma mark -

/*!
 @class		RBException
 @abstract	This is the exception used by any C++ PlayerKit class when it needs
 to report a fatal error.
 */
class RBException : public RBObject
{
protected:
	//ivars
	/* retain */ CFStringRef mDomain;
	/* retain */ CFStringRef mReason;
	OSStatus mCode;
	
public:
	/*!
	 @const	DefaultErrorCode
	 @abstract	The default error code used by RBException.
	 */
	static OSStatus const DefaultErrorCode = (-1);
	
	/*!
	 @const		InternalInconsistencyDomain
	 @abstract	Domain of an exception that occurs when an internal assertion
	 fails and implies an unexpected condition within the called code.
	 */
	static CFStringRef InternalInconsistencyDomain;
	
	/*! @ignore */ //Used by RBAssert
	static void HandleFailureInFunction(const char *function, const char *file, int lineNumber, OSStatus erorrCode, CFStringRef description, ...);
	
	/*!
	 @method	RBException
	 @abstract	The default constructor for RBException.
	 @param		domain	The domain of the exception. Defaults to internal inconsistency.
	 @param		reason	The reason for the exception. Defaults to an empty string.
	 */
	RBException(CFStringRef domain = InternalInconsistencyDomain, 
				CFStringRef reason = CFSTR("(No Reason Given)"), 
				OSStatus code = DefaultErrorCode) :
		RBObject("RBException")
	{
		//C++ sucks, so these explicit casts
		//are actually necessary.
		mDomain = (CFStringRef)CFRetain(domain);
		mReason = (CFStringRef)CFRetain(reason);
		mCode = code;
	}
	
	//! @abstract The destructor
	~RBException()
	{
		CFRelease(mDomain);
		CFRelease(mReason);
	}
	
	//
	//	These incredibly ugly methods are the only way
	//	I could get this exception class to work when
	//	thrown on the stack. Yay C++.
	//
	RBException(const RBException &inException) :
		RBObject("RBException")
	{
		//C++ sucks, so these explicit casts
		//are actually necessary.
		mDomain = (CFStringRef)CFRetain(inException.mDomain);
		mReason = (CFStringRef)CFRetain(inException.mReason);
		mCode = inException.mCode;
	}
	
	RBException& operator=(const RBException &inException)
	{
		mDomain = (CFStringRef)CFRetain(inException.mDomain);
		mReason = (CFStringRef)CFRetain(inException.mReason);
		mCode = inException.mCode;
		
		return *this;
	}
	
	/*!
	 @method
	 @abstract	Get the domain of an exception.
	 */
	CFStringRef GetDomain() const { return mDomain; }
	
	/*!
	 @method
	 @abstract	Get the reason for an exception to be thrown.
	 */
	CFStringRef GetReason() const { return mReason; }
	
	/*!
	 @method
	 @abstract	Get the code associated with the exception.
	 */
	OSStatus GetCode() const { return mCode; }
	
	//! @abstract Copy out a CFErrorRef
	CFErrorRef CopyError() const;
	
private:
	/*! @ignore */
	RBException() : RBObject("RBException") {};
};

#endif /* RBException_h */
