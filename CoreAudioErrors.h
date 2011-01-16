/*
 *  CoreAudioErrors.h
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 1/16/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef CoreAudioErrors_h
#define CoreAudioErrors_h 1

///Returns the name of a specified error code.
PK_EXTERN PK_VISIBILITY_HIDDEN CFStringRef CoreAudioGetErrorName(OSStatus errorCode);

#endif /* CoreAudioErrors_h */