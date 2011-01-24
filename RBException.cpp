/*
 *  RBException.cpp
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 5/18/09.
 *  Copyright 2009 Roundabout Software. All rights reserved.
 *
 */

#include "RBException.h"
#include <iostream>

CFStringRef RBException::InternalInconsistencyDomain = CFSTR("InternalInconsistencyDomain");

void RBException::HandleFailureInFunction(const char *function, const char *file, int lineNumber, OSStatus erorrCode, CFStringRef description, ...)
{
	va_list format;
	va_start(format, description);
	CFStringRef reason = CFStringCreateWithFormatAndArguments(kCFAllocatorDefault, NULL, description, format);
	va_end(format);
	
	fprintf(stderr, "*** Assertion failure in function %s in file %s on line %d with reason ", function, file, lineNumber);
	
	//CFString does not like to be converted to a C String, so we
	//output it using the CoreFoundation mechanism for doing so.
	CFShow(reason);
	
	fprintf(stderr, "\n");
	
	RBException exception(RBException::InternalInconsistencyDomain, reason, erorrCode);
	
	//`exception` is taking ownership
	CFRelease(reason);
	
#if DEBUG
	Debugger();
#endif /* DEBUG */
	
	throw exception;
}

CFErrorRef RBException::CopyError() const
{
	const void *keys[] = { kCFErrorLocalizedDescriptionKey };
	const void *values[] = { this->GetReason() };
	return CFErrorCreateWithUserInfoKeysAndValues(kCFAllocatorDefault, //allocator
												  this->GetDomain(), //domain
												  this->GetCode(), //code
												  keys, //user info keys
												  values, //user info values
												  1); //user info pairs count
}
