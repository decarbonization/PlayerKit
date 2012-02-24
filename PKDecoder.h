/*
 *  PKDecoder.h
 *  PKLite
 *
 *  Created by Peter MacWhinnie on 5/23/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include <PlayerKit/RBObject.h>
#include <PlayerKit/RBException.h>
#include <CoreFoundation/CoreFoundation.h>
#include <AudioToolbox/AudioToolbox.h>
#include <vector>

#ifndef PKDecoder_h
#define PKDecoder_h 1

/*!
 @abstract	The PKDecoder class encapsulates the decoder class cluster in PlayerKit
			by providing a base interface and a class collection.
 */
class PK_VISIBILITY_PUBLIC PKDecoder : public RBObject
{
#pragma mark Class Cluster
	
public:
	
	struct Description
	{
		/*!
		 @abstract	A function pointer that specifies whether or not a decoder can decode a file at a specified location.
		 */
		bool(*CanDecode)(CFURLRef fileLocation);
		
		/*!
		 @abstract	A function pointer that returns a CFArray instance of CFStrings whose contents describe the UTIs that the decoder can decode.
		 */
		CFArrayRef (*CopySupportedTypes)();
		
		/*!
		 @abstract	A function pointer that returns a new instance of a decoder, or throws an RBException instance if the decoder cannot be created.
		 */
		PKDecoder *(*CreateInstance)(CFURLRef fileLocation) throw(RBException);
	};
	
	/*!
	 @abstract	Registers a decoder for use in the PKDecoder cluster.
	 */
	static void RegisterDecoder(const Description &decoderDescription);
	
	/*!
	 @abstract	Returns a decoder that can decode a specified URL.
	 @param		location	The location of the file to find a decoder for. Required.
	 @result	A decoder for `location` if one can be found; NULL otherwise.
	 */
	static PKDecoder *DecoderForURL(CFURLRef location) throw(RBException);
	
	/*!
	 @abstract	Returns a Bool indicating whether or not a specified file can be decoded.
	 @param		location	The location of the file to find a decoder for. Optional.
	 @result	true if the `location` can be decoded; false otherwise.
	 */
	static bool CanDecodeURL(CFURLRef location) throw(RBException);
	
#pragma mark -
#pragma mark Types
	
public:
	
	/*!
	 @abstract	The type used to describe frame locations.
	 */
	typedef unsigned long long FrameLocation;
	
#pragma mark -
#pragma mark Lifecycle
	
	/*!
	 @abstract	Construct the decoder passing in the name of the decoder subclass.
	 */
	explicit PKDecoder(const char *className = "PKDecoder");
	
	/*!
	 @abstract	Destruct the decoder.
	 */
	virtual ~PKDecoder();
	
#pragma mark -
#pragma mark Attributes
	
	/*!
	 @abstract	Returns the decoder's stream format.
	 */
	virtual AudioStreamBasicDescription GetStreamFormat() const PK_PURE_VIRTUAL;
	
	/*!
	 @abstract	Returns a copy of the decoder's location.
	 */
	virtual CFURLRef CopyLocation() const PK_PURE_VIRTUAL;
	
#pragma mark -
	
	/*!
	 @abstract	Returns the total number of frames the decoder can decode.
	 */
	virtual FrameLocation GetTotalNumberOfFrames() const PK_PURE_VIRTUAL;
	
#pragma mark -
	
	/*!
	 @abstract	Returns whether or not a decoder can seek.
	 */
	virtual bool CanSeek() const PK_PURE_VIRTUAL;
	
	/*!
	 @abstract	Get the frame the decoder is currently decoding.
	 */
	virtual FrameLocation GetCurrentFrame() const PK_PURE_VIRTUAL;
	
	/*!
	 @abstract	Set the frame the decoder is currently decoding.
	 */
	virtual void SetCurrentFrame(FrameLocation currentFrame) PK_PURE_VIRTUAL;
	
#pragma mark -
#pragma mark Decoding
	
	/*!
	 @abstract		Populate a specified audio buffer list with decoded linear PCM data.
	 @param			buffers			The buffers to populate with decoded linear PCM data. Required.
	 @param			numberOfFrames	The maximum number of frames to decode into the buffer.
	 @result		The total number of frames decoded into the passed in buffers.
	 @discussion	This method throws instances of RBException when errors occur.
	 */
	virtual UInt32 FillBuffers(AudioBufferList *buffers, UInt32 numberOfFrames) throw(RBException) PK_PURE_VIRTUAL;
};

#endif /* PKDecoder_h */
