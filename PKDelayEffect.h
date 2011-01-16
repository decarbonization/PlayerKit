/*
 *  PKDelayEffect.h
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 1/14/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef PKDelayEffect_h
#define PKDelayEffect_h 1

#import <PlayerKit/PKAudioEffect.h>

///The opaque type used to represent PKDelayEffect. Typed alias for PKAudioEffectRef.
typedef PKAudioEffectRef PKDelayEffectRef;

#pragma mark -
#pragma mark Creation

///Create a Delay effect.
PK_EXTERN PKDelayEffectRef PKDelayEffectCreate(CFErrorRef *outError);

#pragma mark -
#pragma mark Properties

///Sets the amount of delayed sound that is audible.
PK_EXTERN Boolean PKDelayEffectSetAmount(PKDelayEffectRef effect, AudioUnitParameterValue value, CFErrorRef *outError);

///Gets the amount of delayed sound that is audible.
PK_EXTERN AudioUnitParameterValue PKDelayEffectGetAmount(PKDelayEffectRef effect);

#pragma mark -

///Sets the delay time.
PK_EXTERN Boolean PKDelayEffectSetDelayTime(PKDelayEffectRef effect, AudioUnitParameterValue value, CFErrorRef *outError);

///Gets the delay time.
PK_EXTERN AudioUnitParameterValue PKDelayEffectGetDelayTime(PKDelayEffectRef effect);

#pragma mark -

///Sets the feedback.
PK_EXTERN Boolean PKDelayEffectSetFeedback(PKDelayEffectRef effect, AudioUnitParameterValue value, CFErrorRef *outError);

///Gets the feedback.
PK_EXTERN AudioUnitParameterValue PKDelayEffectGetFeedback(PKDelayEffectRef effect);

#pragma mark -

///Sets the lopass cutoff.
PK_EXTERN Boolean PKDelayEffectSetLopassCutoff(PKDelayEffectRef effect, AudioUnitParameterValue value, CFErrorRef *outError);

///Gets the lopass cutoff.
PK_EXTERN AudioUnitParameterValue PKDelayEffectGetLopassCutoff(PKDelayEffectRef effect);

#endif /* PKDelayEffect_h */
