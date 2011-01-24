/*
 *  PlayerKitDefines.h
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 7/1/08.
 *  Copyright 2008 Roundabout Software. All rights reserved.
 *
 */

#import "CAStreamBasicDescription.h"

CFStringRef const PKEffectsErrorDomain = CFSTR("PKEffectsErrorDomain");
CFStringRef const PKPlaybackErrorDomain = CFSTR("PKPlaybackErrorDomain");

Float64 const kPKCanonicalSampleRate = 44100.0;
UInt64 const kPKCanonicalBaseBufferSize = (1024 * 10);

extern "C" Boolean PKStreamFormatIsCanonical(AudioStreamBasicDescription inFormat)
{
	CAStreamBasicDescription format(inFormat);
	return format.IsCanonical() && !format.IsInterleaved() && (format.mSampleRate == kPKCanonicalSampleRate);
}

#pragma mark -

extern "C" CFErrorRef PKCopyError(CFStringRef domain, CFIndex errorCode, CFDictionaryRef inUserInfo, CFStringRef format, ...)
{
	va_list formatList;
	va_start(formatList, format);
	
	CFStringRef errorString = CFStringCreateWithFormatAndArguments(kCFAllocatorDefault, NULL, format, formatList);
	
	va_end(formatList);
	
	CFMutableDictionaryRef userInfo = NULL;
	if(inUserInfo)
		userInfo = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, inUserInfo);
	else
		userInfo = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	CFDictionarySetValue(userInfo, kCFErrorLocalizedDescriptionKey, errorString);
	
	CFErrorRef error = CFErrorCreate(kCFAllocatorDefault, domain, errorCode, userInfo);
	
	CFRelease(errorString);
	CFRelease(userInfo);
	
	return error;
}
