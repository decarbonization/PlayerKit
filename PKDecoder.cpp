/*
 *  PKDecoder.cpp
 *  PKLite
 *
 *  Created by Peter MacWhinnie on 5/23/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#include "PKDecoder.h"
#include <dispatch/dispatch.h>
#include "PKCoreAudioDecoder.h"

#pragma mark PKDecoder

#pragma mark Class Cluster

static std::vector<PKDecoder::Description> &RegisteredDecoders()
{
	static std::vector<PKDecoder::Description> *registeredDecoders = NULL;
	if(!registeredDecoders)
	{
		registeredDecoders = new std::vector<PKDecoder::Description>();
		registeredDecoders->push_back(PKCoreAudioDecoderDescription);
	}
	
	return *registeredDecoders;
}

void PKDecoder::RegisterDecoder(const Description &decoderDescription)
{
	RegisteredDecoders().push_back(decoderDescription);
}

PKDecoder *PKDecoder::DecoderForURL(CFURLRef location) throw(RBException)
{
	std::vector<PKDecoder::Description> &decoders = RegisteredDecoders();
	for (std::vector<PKDecoder::Description>::const_iterator it = decoders.begin(); it != decoders.end(); it++)
	{
		PKDecoder::Description decoder = *it;
		if(decoder.CanDecode(location))
			return decoder.CreateInstance(location);
	}
	
	return NULL;
}

bool PKDecoder::CanDecodeURL(CFURLRef location) throw(RBException)
{
	std::vector<PKDecoder::Description> &decoders = RegisteredDecoders();
	for (std::vector<PKDecoder::Description>::const_iterator it = decoders.begin(); it != decoders.end(); it++)
	{
		PKDecoder::Description decoder = *it;
		if(decoder.CanDecode(location))
			return true;
	}
	
	return false;
}

#pragma mark -
#pragma mark Lifetime

PKDecoder::PKDecoder(const char *className) : 
	RBObject(className)
{
	
}

PKDecoder::~PKDecoder()
{
	
}
