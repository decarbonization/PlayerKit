/*
 *  PlayerKitDefines.h
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 7/1/08.
 *  Copyright 2008 Roundabout Software. All rights reserved.
 *
 */

#import "CAStreamBasicDescription.h"

#import <CoreAudioKit/CoreAudioKit.h>
#import <AudioUnit/AUCocoaUIView.h>

PKStringRef const PKDecodingErrorDomain = @"PKDecodingErrorDomain";

PKStringRef const PKInitializationErrorDomain = @"PKInitializationErrorDomain";
PKStringRef const PKEffectsErrorDomain = @"PKEffectsErrorDomain";
PKStringRef const PKPlaybackErrorDomain = @"PKPlaybackErrorDomain";

Float64 const kPKCanonicalSampleRate = 44100.0;
UInt32 const kPKCanonicalBaseBufferSize = 8192;

extern "C" Boolean PKStreamFormatIsCanonical(AudioStreamBasicDescription inFormat)
{
	CAStreamBasicDescription format(inFormat);
	return format.IsCanonical() && !format.IsInterleaved() && (format.mSampleRate == kPKCanonicalSampleRate);
}

extern "C" NSView *PKGetViewForAudioUnit(AudioUnit audioUnit, BOOL defaultToGenericView)
{
	NSCParameterAssert(audioUnit);
	
	//Look up the number of cocoa view classes available
	UInt32 dataSize = 0;
	Boolean isWritable = false;
	
	OSStatus error = AudioUnitGetPropertyInfo(audioUnit, //in audioUnit
											  kAudioUnitProperty_CocoaUI, //in propertyID
											  kAudioUnitScope_Global, //in propertyScope
											  0, //in elementID
											  &dataSize, //out dataSize
											  &isWritable); //out propertyIsWritable
	
	//
	//	We're being given the size for a varadic-size struct.
	//	Its last member is an array of CFStringRef's.
	//
	UInt32 numberOfClasses = (dataSize - sizeof(CFURLRef)) / sizeof(CFStringRef);
	
	//
	//	We first attempt to look up the audio unit's custom views.
	//
	NSView *view = nil;
	if((error == noErr) && (numberOfClasses > 0))
	{
		//
		//	We allocate our view info on the stack so we don't have to worry about
		//	cleaning it up if there's an exception raised.
		//
		AudioUnitCocoaViewInfo *cocoaViewInfo = (AudioUnitCocoaViewInfo *)alloca(dataSize);
		
		//Get the data.
		error = AudioUnitGetProperty(audioUnit, //in audioUnit
									 kAudioUnitProperty_CocoaUI, //in propertyID
									 kAudioUnitScope_Global, //in propertyScope
									 0, //in elementID
									 cocoaViewInfo, //out data
									 &dataSize); //inout dataSize
		if(error == noErr)
		{
			CFURLRef bundleURL = cocoaViewInfo->mCocoaAUViewBundleLocation;
			CFStringRef factoryClassName = cocoaViewInfo->mCocoaAUViewClass[0];
			
			@try
			{
				//We look up the bundle specified by the cocoa view info and attempt to use that.
				NSBundle *viewBundle = [NSBundle bundleWithURL:(NSURL *)bundleURL];
				if(viewBundle != nil)
				{
					//This will cause the bundle to load.
					Class factoryClass = [viewBundle classNamed:(NSString *)factoryClassName];
					
					//
					//	Its possible that the factory class will not actually
					//	implement this method so we check just in case.
					//
					if([factoryClass instancesRespondToSelector:@selector(uiViewForAudioUnit:withSize:)])
					{
						//Make a factory
						NSObject < AUCocoaUIBase > *factoryInstance = [[factoryClass new] autorelease];
						
						//and finally, make a view.
						view = [factoryInstance uiViewForAudioUnit:audioUnit withSize:NSZeroSize];
					}
				}
			}
			@finally
			{
				//Clean up after the view classes we were given.
				CFRelease(bundleURL);
				for (NSUInteger index = 0; index < numberOfClasses; index++)
					CFRelease(cocoaViewInfo->mCocoaAUViewClass[index]);
			}
		}
	}
	
	//
	//	If the audio unit does not provide a specialized view and we've been asked
	//	to default to a generic view then we create a generic view and return that.
	//
	if(!view && defaultToGenericView)
	{
		view = [[[AUGenericView alloc] initWithAudioUnit:audioUnit] autorelease];
		[(AUGenericView *)view setShowsExpertParameters:YES];
	}
	
	return view;
}

#pragma mark -

extern "C" NSError *PKError(NSString *domain, NSInteger errorCode, NSDictionary *inUserInfo, NSString *format, ...)
{
	va_list formatList;
	va_start(formatList, format);
	
	NSString *errorString = [[[NSString alloc] initWithFormat:format arguments:formatList] autorelease];
	
	va_end(formatList);
	
	NSMutableDictionary *userInfo = nil;
	if(inUserInfo)
		userInfo = [NSMutableDictionary dictionaryWithDictionary:inUserInfo];
	else
		userInfo = [NSMutableDictionary dictionary];
	[userInfo setObject:errorString forKey:NSLocalizedDescriptionKey];
	
	return [NSError errorWithDomain:domain code:errorCode userInfo:userInfo];
}

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

#pragma mark -

extern "C" NSString *PKStringFromDirtyClassName(NSString *className)
{
	//We remove everything before ": ", including ": " so the below code
	//can properly "prettify" the class name.
	NSRange colonSpaceRange = [className rangeOfString:@": "];
	if(colonSpaceRange.location != NSNotFound)
	{
		className = [className substringFromIndex:(colonSpaceRange.location + colonSpaceRange.length)];
	}
	
	//We ignore two-letter prefixes.
	NSUInteger startIndex = 0;
	if([className length] >= 3)
	{
		if(isupper([className characterAtIndex:0]) &&
		   isupper([className characterAtIndex:1]) &&
		   isupper([className characterAtIndex:2]))
		{
			startIndex = 2;
		}
	}
	
	//Here we split the string apart by capitol letters.
	NSMutableString *name = [NSMutableString string];
	for (NSUInteger index = startIndex; index < [className length]; index++)
	{
		unichar character = [className characterAtIndex:index];
		unichar lastCharacter = ((index - 1) > startIndex)? [className characterAtIndex:(index - 1)] : 0;
		if((index > startIndex) && isupper(character) && !isupper(lastCharacter))
		{
			[name appendString:@" "];
		}
		[name appendFormat:@"%c", character];
	}
	
	return name;
}
