/*
 *  PKGraphicEQEffect.h
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 10/16/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef PKGraphicEQEffect_h
#define PKGraphicEQEffect_h 1

#import <PlayerKit/PKAudioEffect.h>

#pragma mark Creation

///Create a graphic EQ effect.
PK_EXTERN PKAudioEffectRef PKGraphicEQEffectCreate(CFErrorRef *outError);

#pragma mark -
#pragma mark Properties

///Sets whether or not a graphic EQ effect has 32 bands. If false, it has 12. Default is true.
PK_EXTERN Boolean PKGraphicEQEffectSetHas32Bands(PKAudioEffectRef effect, Boolean has32Bands, CFErrorRef *outError);

///Returns whether or not a graphic EQ effect has 32 bands. If false, it has 12.
PK_EXTERN Boolean PKGraphicEQEffectHas32Bands(PKAudioEffectRef effect);

#pragma mark -

///Sets the value of a specified band in a graphic EQ effect.
PK_EXTERN Boolean PKGraphicEQEffectSetValueOfBand(PKAudioEffectRef effect, CFIndex bandIndex, AudioUnitParameterValue bandValue, CFErrorRef *outError);

///Returns the value of a specified band in a graphic EQ effect.
PK_EXTERN AudioUnitParameterValue PKGraphicEQEffectGetValueOfBand(PKAudioEffectRef effect, CFIndex bandIndex);

#endif /* PKGraphicEQEffect_h */