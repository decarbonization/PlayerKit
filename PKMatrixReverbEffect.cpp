/*
 *  PKMatrixReverbEffect.cpp
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 1/12/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "PKMatrixReverbEffect.h"
#include "CAComponentDescription.h"
#include <iostream>

#pragma mark Creation

PK_EXTERN PKMatrixReverbEffectRef PKMatrixReverbEffectCreate(CFErrorRef *outError)
{
	AudioComponentDescription matrixReverbDescription = {
		kAudioUnitType_Effect, 
		kAudioUnitSubType_MatrixReverb, 
		kAudioUnitManufacturer_Apple, 
	};
	
	return PKAudioEffectCreate(matrixReverbDescription, outError);
}

#pragma mark -
#pragma mark Properties

PKAUDIOEFFECT_SYNTHESIZE_PARAMETER(MatrixReverb, DryWetMix, kReverbParam_DryWetMix);
PKAUDIOEFFECT_SYNTHESIZE_PARAMETER(MatrixReverb, SmallLargeMix, kReverbParam_SmallLargeMix);
PKAUDIOEFFECT_SYNTHESIZE_PARAMETER(MatrixReverb, SmallSize, kReverbParam_SmallSize);
PKAUDIOEFFECT_SYNTHESIZE_PARAMETER(MatrixReverb, LargeSize, kReverbParam_LargeSize);
PKAUDIOEFFECT_SYNTHESIZE_PARAMETER(MatrixReverb, PreDelay, kReverbParam_PreDelay);
PKAUDIOEFFECT_SYNTHESIZE_PARAMETER(MatrixReverb, LargeDelay, kReverbParam_LargeDelay);
PKAUDIOEFFECT_SYNTHESIZE_PARAMETER(MatrixReverb, SmallDensity, kReverbParam_SmallDensity);
PKAUDIOEFFECT_SYNTHESIZE_PARAMETER(MatrixReverb, LargeDensity, kReverbParam_LargeDensity);
PKAUDIOEFFECT_SYNTHESIZE_PARAMETER(MatrixReverb, LargeDelayRange, kReverbParam_LargeDelayRange);
PKAUDIOEFFECT_SYNTHESIZE_PARAMETER(MatrixReverb, LargeBrightness, kReverbParam_LargeBrightness);
PKAUDIOEFFECT_SYNTHESIZE_PARAMETER(MatrixReverb, SmallDelayRange, kReverbParam_SmallDelayRange);
PKAUDIOEFFECT_SYNTHESIZE_PARAMETER(MatrixReverb, ModulationRate, kReverbParam_ModulationRate);
PKAUDIOEFFECT_SYNTHESIZE_PARAMETER(MatrixReverb, ModulationDepth, kReverbParam_ModulationDepth);
PKAUDIOEFFECT_SYNTHESIZE_PARAMETER(MatrixReverb, FilterFrequency, kReverbParam_FilterFrequency);
PKAUDIOEFFECT_SYNTHESIZE_PARAMETER(MatrixReverb, FilterBandwidth, kReverbParam_FilterBandwidth);
PKAUDIOEFFECT_SYNTHESIZE_PARAMETER(MatrixReverb, FilterGain, kReverbParam_FilterGain);
