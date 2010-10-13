/*
 *  PKCoreAudioDecoder.h
 *  PKLite
 *
 *  Created by Peter MacWhinnie on 5/23/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef PKCoreAudioDecoder_h
#define PKCoreAudioDecoder_h 1

#include "PKDecoder.h"

class PK_VISIBILITY_HIDDEN PKCoreAudioDecoder : public PKDecoder
{
protected:
	
	ExtAudioFileRef mAudioFile;
	UInt64 mCurrentFrameInFile;
	AudioStreamBasicDescription mAudioStreamDescription;
	CFURLRef mFileLocation;
	
public:
#pragma mark Lifetime
	
	explicit PKCoreAudioDecoder(CFURLRef location);
	virtual ~PKCoreAudioDecoder();
	
#pragma mark -
#pragma mark Attributes
	
	virtual AudioStreamBasicDescription GetStreamFormat() const;
	virtual CFURLRef CopyLocation() const;
	
#pragma mark -
	
	virtual PKDecoder::FrameLocation GetTotalNumberOfFrames() const;
	
#pragma mark -
	
	virtual bool CanSeek() const;
	
	virtual PKDecoder::FrameLocation GetCurrentFrame() const;
	virtual void SetCurrentFrame(PKDecoder::FrameLocation currentFrame);
	
#pragma mark -
#pragma mark Decoding
	
	virtual UInt32 FillBuffers(AudioBufferList *buffers, UInt32 numberOfFrames) throw(RBException);
};

extern PKDecoder::Description PKCoreAudioDecoderDescription;

#endif /* PKCoreAudioDecoder_h */
