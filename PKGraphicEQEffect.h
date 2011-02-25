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

///The opaque type used to represent PKGraphicEQEffect. Typed alias for PKAudioEffectRef.
typedef PKAudioEffectRef PKGraphicEQEffectRef;

#pragma mark -
#pragma mark Creation

///Create a graphic EQ effect.
PK_EXTERN PKGraphicEQEffectRef PKGraphicEQEffectCreate(CFErrorRef *outError);

#pragma mark -
#pragma mark Properties

///Sets whether or not a graphic EQ effect has 32 bands. If false, it has 10. Default is true.
PK_EXTERN Boolean PKGraphicEQEffectSetHas32Bands(PKGraphicEQEffectRef effect, Boolean has32Bands, CFErrorRef *outError);

///Returns whether or not a graphic EQ effect has 32 bands. If false, it has 10.
PK_EXTERN Boolean PKGraphicEQEffectHas32Bands(PKGraphicEQEffectRef effect);

#pragma mark -

///Sets the value of a specified band in a graphic EQ effect.
PK_EXTERN Boolean PKGraphicEQEffectSetValueOfBand(PKGraphicEQEffectRef effect, CFIndex bandIndex, AudioUnitParameterValue bandValue, CFErrorRef *outError);

///Returns the value of a specified band in a graphic EQ effect.
PK_EXTERN AudioUnitParameterValue PKGraphicEQEffectGetValueOfBand(PKGraphicEQEffectRef effect, CFIndex bandIndex);

///Iterates all of the bands of a graphic EQ effect.
PK_EXTERN void PKGraphicEQEffectIterateBands(PKGraphicEQEffectRef effect, void(^iterator)(AudioUnitParameterValue value, CFIndex index, Boolean *stop));

#endif /* PKGraphicEQEffect_h */
