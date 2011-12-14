/*
 *  PKCoreAudioDecoder.cpp
 *  PKLite
 *
 *  Created by Peter MacWhinnie on 5/23/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "PKCoreAudioDecoder.h"
#include "CAStreamBasicDescription.h"

#pragma mark PKCoreAudioDecoder

#pragma mark Lifetime

PKCoreAudioDecoder::PKCoreAudioDecoder(CFURLRef location) : 
	PKDecoder("PKCoreAudioDecoder"),
	mAudioFile(NULL), 
	mCurrentFrameInFile(0), 
	mAudioStreamDescription(), 
	mFileLocation(CFURLRef(CFRetain(location)))
{
	OSStatus status = ExtAudioFileOpenURL(location, &mAudioFile);
	RBAssertNoErr(status, CFSTR("ExtAudioFileOpenURL failed with error code %ld."), status);
	
	AudioStreamBasicDescription fileDataFormat;
	UInt32 dataSize = sizeof(fileDataFormat);
	status = ExtAudioFileGetProperty(mAudioFile, kExtAudioFileProperty_FileDataFormat, &dataSize, &fileDataFormat);
	RBAssertNoErr(status, CFSTR("ExtAudioFileGetProperty(kExtAudioFileProperty_FileDataFormat) failed with error code %ld."), status);
	
	CAStreamBasicDescription format;
	format.mSampleRate = kPKCanonicalSampleRate;
	format.SetCanonical(2, false);
	status = ExtAudioFileSetProperty(mAudioFile, kExtAudioFileProperty_ClientDataFormat, sizeof(format), &format);
	RBAssertNoErr(status, CFSTR("ExtAudioFileSetProperty(kExtAudioFileProperty_ClientDataFormat) failed with error code %ld."), status);
	
	mAudioStreamDescription = format;
}

PKCoreAudioDecoder::~PKCoreAudioDecoder()
{
	if(mAudioFile)
	{
		ExtAudioFileDispose(mAudioFile);
		mAudioFile = NULL;
	}
	
	if(mFileLocation)
	{
		CFRelease(mFileLocation);
		mFileLocation = NULL;
	}
}

#pragma mark -
#pragma mark Attributes

AudioStreamBasicDescription PKCoreAudioDecoder::GetStreamFormat() const
{
	return mAudioStreamDescription;
}

CFURLRef PKCoreAudioDecoder::CopyLocation() const
{
	return CFURLRef(CFRetain(mFileLocation));
}

#pragma mark -

PKDecoder::FrameLocation PKCoreAudioDecoder::GetTotalNumberOfFrames() const
{
	SInt64 length;
    UInt32 size = sizeof(length);
	if(ExtAudioFileGetProperty(mAudioFile, kExtAudioFileProperty_FileLengthFrames, &size, &length) == noErr)
		return length;
	
	return 0;
}

#pragma mark -

bool PKCoreAudioDecoder::CanSeek() const
{
	return true;
}

PKDecoder::FrameLocation PKCoreAudioDecoder::GetCurrentFrame() const
{
	return mCurrentFrameInFile;
}

void PKCoreAudioDecoder::SetCurrentFrame(PKDecoder::FrameLocation currentFrame)
{
	if(currentFrame <= this->GetTotalNumberOfFrames())
	{
		if(ExtAudioFileSeek(mAudioFile, currentFrame) == noErr)
			mCurrentFrameInFile = currentFrame;
	}
}

#pragma mark -
#pragma mark Decoding

UInt32 PKCoreAudioDecoder::FillBuffers(AudioBufferList *buffers, UInt32 numberOfFrames) throw(RBException)
{
	OSStatus errorCode = ExtAudioFileRead(mAudioFile, &numberOfFrames, buffers);
	RBAssertNoErr(errorCode, CFSTR("ExtAudioFileRead failed with error %ld."), errorCode);
	
	mCurrentFrameInFile += numberOfFrames;
	
	return numberOfFrames;
}

#pragma mark -
#pragma mark Description

static bool CanDecode(CFURLRef fileLocation)
{
	AudioFileID audioFile;
	if((AudioFileOpenURL(fileLocation, kAudioFileReadPermission, 0, &audioFile) == noErr) && audioFile)
	{
		return (AudioFileClose(audioFile) == noErr);
	}
	
	return false;
}

static CFArrayRef CopySupportedTypes()
{
	CFArrayRef types = NULL;
	UInt32 size = sizeof(CFArrayRef);
	OSStatus error = AudioFileGetGlobalInfo(kAudioFileGlobalInfo_AllUTIs, 0, NULL, &size, &types);
	RBAssertNoErr(error, CFSTR("AudioFileGetGlobalInfo failed with error code %ld."), error);
	
	return types;
}

static PKDecoder *CreateInstance(CFURLRef fileLocation) throw(RBException)
{
	return new PKCoreAudioDecoder(fileLocation);
}

PKDecoder::Description PKCoreAudioDecoderDescription = { &CanDecode, &CopySupportedTypes, &CreateInstance };
