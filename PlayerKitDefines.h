/*
 *  PlayerKitDefines.h
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 7/1/08.
 *  Copyright 2008 Roundabout Software. All rights reserved.
 *
 */

#ifdef __OBJC__
#	import <Cocoa/Cocoa.h>
#else
#	import <CoreFoundation/CoreFoundation.h>
#endif /* __OBJC__ */

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

#define PK_CONCAT_(a, b) a ## b
#define PK_CONCAT(a, b) PK_CONCAT_(a, b)

/*!
 @abstract	Whether or not a flag is set on a bitfield.
 @param		field	The field to check for the flag on.
 @param		flag	The flag.
 */
#define PK_FLAG_IS_SET(field, flag) ((flag & field) == flag)

#pragma mark -
#pragma mark Types

/*!
 @typedef	PKStringRef
 */
#if __OBJC__
typedef NSString * PKStringRef;
#else
typedef CFStringRef PKStringRef;
#endif /* __OBJC__ */

#pragma mark -
#pragma mark Constants

//Errors:

//! @abstract	The decoding error domain used by PlayerKit.
PK_EXTERN PKStringRef const PKDecodingErrorDomain;


//! @abstract	The initialization error domain used by PlayerKit.
PK_EXTERN PKStringRef const PKInitializationErrorDomain;

enum _PKInitializationErrorDomainErrors {
	//All errors should begin with 1000
	
	//! @abstract	The error given when the decoder cluster has no decoder available.
	kPKInitializationErrorNoDecoderForFile = (-100001),
	
	//! @abstract	The error given when fopen or another similar function fails.
	kPKInitializationErrorCouldNotCreateFileHandle = (-100002),
	
	//! @abstract	The error given when a file cannot be opened.
	kPKInitializationErrorCouldNotOpenFile = (-100003),
	
	//! @abstract	The error given when getting the stream format from a file fails.
	kPKInitializationErrorCouldNotGetFileStreamFormat = (-100004),
	
	//! @abstract	The error given when setting the stream format on an audio file object fails.
	kPKInitializationErrorCouldNotSetStreamFormat = (-100005),
	
	//! @abstract	The error given when a song cannot be played because the computer is not authorized.
	kPKInitializationErrorProtectedFileNotAuthorized = (-100006),
};

//! @abstract	The effects error domain used by PlayerKit.
PK_EXTERN PKStringRef const PKEffectsErrorDomain;

enum _PKEffectsErrorDomainErrors {
	//All errors should begin with 2000
	
	//! @abstract	The error returned when an effect cannot be added to a PKAudioPlayer.
	kPKEffectsErrorCannotAddEffectToAudioPlayer = (-200001),
	
	//! @abstract	The error returned when an effect cannot be removed from a PKAudioPlayer.
	kPKEffectsErrorCannotRemoveEffectFromAudioPlayer = (-200002),
	
	//! @abstract	The error returned when a PKAudioPlayer object cannot remove all of its effects.
	kPKEffectsErrorCannotRemoveAllEffects = (-200003),
};

//! @abstract	The playback error domain used by PlayerKit.
PK_EXTERN PKStringRef const PKPlaybackErrorDomain;
enum _PKPlaybackErrorDomainErrors {
	//All errors should begin with 3000
	
	//! @abstract	The error given when an incompatible stream format is used in PKAudioPlayer.
	kPNPlaybackErrorIncompatibleStreamFormat = (-300001),
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
PK_EXTERN UInt32 const kPKCanonicalBaseBufferSize;

#pragma mark -
#pragma mark Error Handling

#if __OBJC__

/*!
 @abstract	Returns a new autoreleased NSError instance with a specified domain, error code, userInfo, and description.
 @param		domain		The domain of the error.
 @param		errorCode	The code of the error.
 @param		userInfo	Any additional user info to attach to the error instance.
 @param		format		A format string.
 @param		...			A comma separated list of arguments to substitute into `format`.
 */
PK_EXTERN NSError *PKError(NSString *domain, NSInteger errorCode, NSDictionary *userInfo, NSString *format, ...);

#endif /* __OBJC__ */

/*!
 @abstract	Returns a new CFError instance with a specified domain, error code, userInfo, and description.
 @param		domain		The domain of the error.
 @param		errorCode	The code of the error.
 @param		userInfo	Any additional user info to attach to the error instance.
 @param		format		A format string.
 @param		...			A comma separated list of arguments to substitute into `format`.
 */
PK_EXTERN CFErrorRef PKCopyError(CFStringRef domain, CFIndex errorCode, CFDictionaryRef userInfo, CFStringRef format, ...);

#pragma mark -
#pragma mark Tools

#if __OBJC__
/*!
 @abstract	This function takes a "dirty class name" like NSMutableString
			and turns it into something user friendly, like "Mutable String"
 */
PK_EXTERN NSString *PKStringFromDirtyClassName(NSString *className);

#pragma mark -

/*!
 @function
 @abstract	Create a dictionary representation of a specified audio component description.
 */
PK_INLINE NSDictionary *PKAudioComponentDescriptionGetDictionaryRepresentation(AudioComponentDescription description)
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
			[NSNumber numberWithLong:description.componentType], @"componentType", 
			[NSNumber numberWithLong:description.componentSubType], @"componentSubType", 
			[NSNumber numberWithLong:description.componentManufacturer], @"componentManufacturer", 
			[NSNumber numberWithInt:description.componentFlags], @"componentFlags",
			[NSNumber numberWithInt:description.componentFlagsMask], @"componentFlagsMask", 
			nil];
}

/*!
 @function
 @abstract	Create an audio component description from a specified dictionary.
 @param		dictionary	A dictionary which contains values for the keys "componentType", "componentSubType", and "componentManufacturer". May not be nil.
 @result	A component description value constructed from the specified dictionary.
 */
PK_INLINE AudioComponentDescription PKAudioComponentDescriptionCreateFromDictionaryRepresentation(NSDictionary *dictionary)
{
	NSCParameterAssert(dictionary);
	
	NSArray *dictionaryKeys = [dictionary allKeys];
	NSCAssert1(([dictionaryKeys containsObject:@"componentType"] && 
				[dictionaryKeys containsObject:@"componentSubType"] && 
				[dictionaryKeys containsObject:@"componentManufacturer"]), 
			   @"AudioComponentDescription dictionary %@ does not contain all required keys.", dictionary);
	
	AudioComponentDescription componentDescription;
	
	componentDescription.componentType = [[dictionary objectForKey:@"componentType"] longValue];
	componentDescription.componentSubType = [[dictionary objectForKey:@"componentSubType"] longValue];
	componentDescription.componentManufacturer = [[dictionary objectForKey:@"componentManufacturer"] longValue];
	componentDescription.componentFlags = [[dictionary objectForKey:@"componentFlags"] intValue];
	componentDescription.componentFlagsMask = [[dictionary objectForKey:@"componentFlagsMask"] intValue];
	
	return componentDescription;
}
#endif /* __OBJC__ */

#pragma mark -

/*!
 @function
 @abstract	Returns whether or not a specified audio stream format is canonical in the context of PKAudioPlayerEngine.
 */
PK_EXTERN Boolean PKStreamFormatIsCanonical(AudioStreamBasicDescription format);

#if __OBJC__

/*!
 @function
 @abstract	Get a view to edit the parameters of a specified audio unit.
 @param		audioUnit				The audio unit to find a view for. May not be nil.
 @param		defaultToGenericView	Whether or not a generic view should be returned if a specialized one cannot be found.
 @result	A new autoreleased view.
 */
PK_EXTERN NSView *PKGetViewForAudioUnit(AudioUnit audioUnit, BOOL defaultToGenericView);

#endif /* __OBJC__ */

#endif /* PlayerKitDefines_h_ */
