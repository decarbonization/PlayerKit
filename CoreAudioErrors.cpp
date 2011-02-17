/*
 *  CoreAudioErrors.cpp
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 1/16/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "CoreAudioErrors.h"
#include <utility>

static const std::pair<OSStatus, CFStringRef> CoreAudioErrorTable[] = {
	/** General Errors **/
	
	std::make_pair(0, CFSTR("noErr")),
	std::make_pair(-50, CFSTR("paramErr")),
	
	
	/** For AudioUnit **/
	
	std::make_pair(-10879, CFSTR("kAudioUnitErr_InvalidProperty")), 
	std::make_pair(-10878, CFSTR("kAudioUnitErr_InvalidParameter")), 
	std::make_pair(-10877, CFSTR("kAudioUnitErr_InvalidElement")), 
	std::make_pair(-10876, CFSTR("kAudioUnitErr_NoConnection")), 
	std::make_pair(-10875, CFSTR("kAudioUnitErr_FailedInitialization")), 
	std::make_pair(-10874, CFSTR("kAudioUnitErr_TooManyFramesToProcess")), 
	std::make_pair(-10871, CFSTR("kAudioUnitErr_InvalidFile")), 
	std::make_pair(-10868, CFSTR("kAudioUnitErr_FormatNotSupported")), 
	std::make_pair(-10867, CFSTR("kAudioUnitErr_Uninitialized")), 
	std::make_pair(-10866, CFSTR("kAudioUnitErr_InvalidScope")), 
	std::make_pair(-10865, CFSTR("kAudioUnitErr_PropertyNotWritable")), 
	std::make_pair(-10863, CFSTR("kAudioUnitErr_CannotDoInCurrentContext")), 
	std::make_pair(-10851, CFSTR("kAudioUnitErr_InvalidPropertyValue")), 
	std::make_pair(-10850, CFSTR("kAudioUnitErr_PropertyNotInUse")), 
	std::make_pair(-10849, CFSTR("kAudioUnitErr_Initialized")), 
	std::make_pair(-10848, CFSTR("kAudioUnitErr_InvalidOfflineRender")), 
	std::make_pair(-10847, CFSTR("kAudioUnitErr_Unauthorized")), 
	
	
	/** For AUGraph **/
	
	std::make_pair(-10860, CFSTR("kAUGraphErr_NodeNotFound")), 
	std::make_pair(-10861, CFSTR("kAUGraphErr_InvalidConnection")), 
	std::make_pair(-10862, CFSTR("kAUGraphErr_OutputNodeErr")), 
	std::make_pair(-10863, CFSTR("kAUGraphErr_CannotDoInCurrentContext")), 
	std::make_pair(-10864, CFSTR("kAUGraphErr_InvalidAudioUnit")), 
	
	
	/** For Audio File **/
	
	std::make_pair('wht?', CFSTR("kAudioFileUnspecifiedError")), 
	std::make_pair('typ?', CFSTR("kAudioFileUnsupportedFileTypeError")), 
	std::make_pair('fmt?', CFSTR("kAudioFileUnsupportedDataFormatError")), 
	std::make_pair('pty?', CFSTR("kAudioFileUnsupportedPropertyError")), 
	std::make_pair('!siz', CFSTR("kAudioFileBadPropertySizeError")), 
	std::make_pair('prm?', CFSTR("kAudioFilePermissionsError")), 
	std::make_pair('optm', CFSTR("kAudioFileNotOptimizedError")), 
	std::make_pair('chk?', CFSTR("kAudioFileInvalidChunkError")), 
	std::make_pair('off?', CFSTR("kAudioFileDoesNotAllow64BitDataSizeError")), 
	std::make_pair('pck?', CFSTR("kAudioFileInvalidPacketOffsetError")), 
	std::make_pair('dta?', CFSTR("kAudioFileInvalidFileError")), 
	std::make_pair('op??', CFSTR("kAudioFileOperationNotSupportedError")), 
	
	
	/** For Audio Hardware **/
	
	std::make_pair('stop', CFSTR("kAudioHardwareNotRunningError")), 
	std::make_pair('what', CFSTR("kAudioHardwareUnspecifiedError")), 
	std::make_pair('who?', CFSTR("kAudioHardwareUnknownPropertyError")), 
	std::make_pair('!siz', CFSTR("kAudioHardwareBadPropertySizeError")), 
	std::make_pair('nope', CFSTR("kAudioHardwareIllegalOperationError")), 
	std::make_pair('!obj', CFSTR("kAudioHardwareBadObjectError")), 
	std::make_pair('!dev', CFSTR("kAudioHardwareBadDeviceError")), 
	std::make_pair('!str', CFSTR("kAudioHardwareBadStreamError")), 
	std::make_pair('unop', CFSTR("kAudioHardwareUnsupportedOperationError")), 
	std::make_pair('!dat', CFSTR("kAudioDeviceUnsupportedFormatError")), 
	std::make_pair('!hog', CFSTR("kAudioDevicePermissionsError")), 
};

PK_EXTERN PK_VISIBILITY_HIDDEN CFStringRef CoreAudioGetErrorName(OSStatus errorCode)
{
	for (int index = 0; index < sizeof(CoreAudioErrorTable) / sizeof(CoreAudioErrorTable[0]); index++)
	{
		const std::pair<OSStatus, CFStringRef> &error = CoreAudioErrorTable[index];
		if(error.first == errorCode)
			return error.second;
	}
	
	return CFSTR("(Unknown)");
}
