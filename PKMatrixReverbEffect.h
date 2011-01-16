/*
 *  PKMatrixReverbEffect.h
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 1/12/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef PKMatrixReverbEffect_h
#define PKMatrixReverbEffect_h 1

#import <PlayerKit/PKAudioEffect.h>

///The opaque type used to represent PKMatrixReverbEffect. Typed alias for PKAudioEffectRef.
typedef PKAudioEffectRef PKMatrixReverbEffectRef;

#pragma mark -
#pragma mark Creation

///Create a Matrix Reverb effect.
PK_EXTERN PKMatrixReverbEffectRef PKMatrixReverbEffectCreate(CFErrorRef *outError);

#pragma mark -
#pragma mark Properties

///Sets the amount of reverb to apply.
PK_EXTERN Boolean PKMatrixReverbEffectSetAmount(PKMatrixReverbEffectRef effect, AudioUnitParameterValue value, CFErrorRef *outError);
///Gets the amount of reverb to apply.
PK_EXTERN AudioUnitParameterValue PKMatrixReverbEffectGetAmount(PKMatrixReverbEffectRef effect);

#endif /* PKMatrixReverbEffect_h */