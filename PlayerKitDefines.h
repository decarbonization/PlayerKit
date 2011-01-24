/*
 *  PlayerKitDefines.h
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 7/1/08.
 *  Copyright 2008 Roundabout Software. All rights reserved.
 *
 */

#import <CoreFoundation/CoreFoundation.h>
#import <AudioToolbox/AudioToolbox.h>

#ifndef PlayerKitDefines_h_
#define PlayerKitDefines_h_ 1

#pragma mark Symbols

#if __cplusplus
#	define PK_EXTERN extern "C"
#else
#	define PK_EXTERN extern
#endif /* __cplusplus */

#define PK_INLINE static inline

#if __cplusplus
#	ifndef PK_FINAL
#		define PK_FINAL
#	endif /* PK_FINAL */
#	ifndef PK_PURE_VIRTUAL
#		define PK_PURE_VIRTUAL =0
#	endif /* PK_PURE_VIRTUAL */
#endif /* __cplusplus */

#if !defined(PK_VISIBILITY_PUBLIC) && !defined(PK_VISIBILITY_HIDDEN)
#	if __GNUC__ >= 4
#		define PK_VISIBILITY_PUBLIC __attribute__((visibility("default")))
#		define PK_VISIBILITY_HIDDEN  __attribute__((visibility("hidden")))
#	else
#		define PK_VISIBILITY_PUBLIC
#		define PK_VISIBILITY_HIDDEN
#	endif /* __GNUC__ >= 4 */
#endif /* !defined(PK_VISIBILITY_PUBLIC) && !defined(PK_VISIBILITY_HIDDEN) */

#pragma mark -

/*!
 @abstract	Whether or not a flag is set on a bitfield.
 @param		field	The field to check for the flag on.
 @param		flag	The flag.
 */
#define PK_FLAG_IS_SET(field, flag) ((flag & field) == flag)

#pragma mark -
#pragma mark Constants

//Errors:

//! @abstract	The effects error domain used by PlayerKit.
PK_EXTERN CFStringRef const PKEffectsErrorDomain;

//! @abstract	The playback error domain used by PlayerKit.
PK_EXTERN CFStringRef const PKPlaybackErrorDomain;
enum _PKPlaybackErrorDomainErrors {
	//All errors should begin with 3000
	
	//! @abstract	The error given when an incompatible stream format is used in PKAudioPlayer.
	kPKPlaybackErrorIncompatibleStreamFormat = (-300001),
};

#pragma mark -

/*!
 @global	kPKCanonicalSampleRate
 @abstract	The canonical sample rate used in PlayerKit.
 */
PK_EXTERN Float64 const kPKCanonicalSampleRate;

/*!
 @global	kPKCanonicalBaseBufferSize
 @abstract	The canonical buffer size used in PlayerKit.
 */
PK_EXTERN UInt64 const kPKCanonicalBaseBufferSize;

#pragma mark -
#pragma mark Error Handling

/*!
 @abstract	Returns a new CFError instance with a specified domain, error code, userInfo, and description.
 @param		domain		The domain of the error.
 @param		errorCode	The code of the error.
 @param		userInfo	Any additional user info to attach to the error instance.
 @param		format		A format string.
 @param		...			A comma separated list of arguments to substitute into `format`.
 */
PK_EXTERN CFErrorRef PKCopyError(CFStringRef domain, CFIndex errorCode, CFDictionaryRef userInfo, CFStringRef format, ...);

/*!
 @function
 @abstract	Returns whether or not a specified audio stream format is canonical in the context of PKAudioPlayerEngine.
 */
PK_EXTERN Boolean PKStreamFormatIsCanonical(AudioStreamBasicDescription format);

#endif /* PlayerKitDefines_h_ */
