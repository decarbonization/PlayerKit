/*
 *  PKAudioPlayer.h
 *  PlayerKit
 *
 *  Created by Peter MacWhinnie on 10/16/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef PKAudioPlayer_h
#define PKAudioPlayer_h 1

#import <CoreFoundation/CoreFoundation.h>

#pragma mark Constants

///The notification posted when an audio player encounters an error during playback.
///The notification is always posted on the main thread. The userInfo dictionary of
///the notification contains one key, CFSTR("Error"), which contains a CFErrorRef
///describing the problem. PKAudioPlayer has stopped playback when this notification
///is posted.
PK_EXTERN CFStringRef const PKAudioPlayerDidEncounterErrorNotification;

///The notification posted when an audio player finishes playing an audio file.
///The notification is always posted on the main thread. The userInfo dictionary
///of the notification contains one key, CFSTR("DidFinish"), which contains a CFBoolean
///indicating whether or not the audio file was played all the way through.
PK_EXTERN CFStringRef const PKAudioPlayerDidFinishPlayingNotification;

///The notification posted when an audio player has changed output devices.
PK_EXTERN CFStringRef const PKAudioPlayerDidChangeOutputDeviceNotification;

///The notification posted when an audio player discovers that another
///PlayerKit-powered application is playing music on the host computer.
PK_EXTERN CFStringRef const PKAudioPlayerDidEncounterOtherPlayerNotification;


///The different possible output destinations that can be returned by PKAudioPlayer.
typedef enum PKAudioPlayerOutputDestination {
	///PKAudioPlayer does not know where it is sending audio.
	kPKAudioPlayerOutputDestinationUnknown = 0, 
	
	///PKAudioPlayer is sending audio to headphones connected to the computer.
	kPKAudioPlayerOutputDestinationHeadphones = 'hdpn',
	
	///PKAudioPlayer is sending audio to the computer's internal speakers.
	kPKAudioPlayerOutputDestinationInternalSpeakers = 'ispk',
} PKAudioPlayerOutputDestination;

#pragma mark -
#pragma mark Utilities

///Indicates if the audio player is capable of playing a specified file.
///	\param	location	The location of the audio file. May be NULL.
///	\result	true if the audio file can be played; false otherwise.
PK_EXTERN Boolean PKAudioPlayerCanPlayFileAtLocation(CFURLRef location);

#pragma mark -
#pragma mark Lifecycle

///Initialize the audio player's internal state.
///	\param	outError	An object encapsulating a description of any errors that occurred. May be null. Must be freed by caller.
///	\result	true if initialization succeeds; false otherwise.
PK_EXTERN Boolean PKAudioPlayerInit(CFErrorRef *outError);

///Teardown the audio player's internal state.
///	\param	outError	An object encapsulating a description of any errors that occurred. May be null. Must be freed by caller.
///	\result	true if the teardown succeeds; false otherwise.
PK_EXTERN Boolean PKAudioPlayerTeardown(CFErrorRef *outError);

#pragma mark -
#pragma mark Controlling Playback

///Set the source file of the audio player.
///	\param	location	The location of the audio file to play.
///	\param	outError	An object encapsulating a description of any errors that occurred. May be null. Must be freed by caller.
///	\result	true if the file located at the passed in URL was loaded successfully; false otherwise.
PK_EXTERN Boolean PKAudioPlayerSetURL(CFURLRef location, CFErrorRef *outError);

///Returns the URL indicating the location of the source file of the audio player.
PK_EXTERN CFURLRef PKAudioPlayerCopyURL();

#pragma mark -

///Start playback in the audio player.
///	\param	outError	An object encapsulating a description of any errors that occurred. May be null. Must be freed by caller.
///	\result	true if playback could be started; false otherwise.
PK_EXTERN Boolean PKAudioPlayerPlay(CFErrorRef *outError);

///Stop playback in the audio player.
///	\param	postNotification	If true then the audio player will post a PKAudioPlayerDidFinishPlayingNotification.
///	\param	outError	An object encapsulating a description of any errors that occurred. May be null. Must be freed by caller.
///	\result	true if playback could be stopped; false otherwise.
PK_EXTERN Boolean PKAudioPlayerStop(Boolean postNotification, CFErrorRef *outError);

///Returns a Boolean indicating whether or not the audio player is currently playing something.
PK_EXTERN Boolean PKAudioPlayerIsPlaying();

#pragma mark -

///Pause playback in the audio player.
///	\param	outError	An object encapsulating a description of any errors that occurred. May be null. Must be freed by caller.
///	\result	true if playback could be paused; false otherwise.
PK_EXTERN Boolean PKAudioPlayerPause(CFErrorRef *outError);

///Resume playback after being paused in the audio player.
///	\param	outError	An object encapsulating a description of any errors that occurred. May be null. Must be freed by caller.
///	\result	true if playback could be resumed; false otherwise.
PK_EXTERN Boolean PKAudioPlayerResume(CFErrorRef *outError);

///Returns a Boolean indicating whether or not the audio player is currently paused.
PK_EXTERN Boolean PKAudioPlayerIsPaused();

#pragma mark -
#pragma mark Properties

///Set the volume level of the audio player. The scale is {0.0, 1.0}, the default value is 1.0.
PK_EXTERN Boolean PKAudioPlayerSetVolume(Float32 volume, CFErrorRef *outError);

///Returns the volume level of the audio player.  The scale is {0.0, 1.0}.
PK_EXTERN Float32 PKAudioPlayerGetVolume();

///Returns the average CPU usage of the audio player.
PK_EXTERN Float32 PKAudioPlayerGetAverageCPUUsage();

#pragma mark -

///The duration of the song the audio player is currently playing.
PK_EXTERN CFTimeInterval PKAudioPlayerGetDuration();

#pragma mark -

///Sets the location of playback in the song the audio player is playing.
PK_EXTERN Boolean PKAudioPlayerSetCurrentTime(CFTimeInterval currentTime, CFErrorRef *outError);

///Returns the current location of playback in the song the audio player is playing.
PK_EXTERN CFTimeInterval PKAudioPlayerGetCurrentTime();

#pragma mark -

///Returns the physical destination of audio sent from PKAudioPlayer to the user.
PK_EXTERN PKAudioPlayerOutputDestination PKAudioPlayerGetAudioOutputDestination(CFErrorRef *outError);

#pragma mark -

///Sets the pulse handler of the audio player.
///
///	\param	handler				The handler block of the audio player. May not be NULL.
///	\param	pulseHandlerQueue	The queue to invoke the handler block on. Defaults to the main queue.
///	\param	outError			On return, a pointer to an error object indicating if any issues occurred.
///
///The pulse is provided as a reliable mechanism to observe the passage of time during playback.
PK_EXTERN Boolean PKAudioPlayerSetPulseHandler(dispatch_block_t handler, dispatch_queue_t pulseHandlerQueue, CFErrorRef *outError);

///Returns the current pulse handler of the audio player.
///
///	\param	outPulseHandlerQueue	On return, this pointer will be set to the queue associated pulse handler. May be NULL.
///
///The pulse is provided as a reliable mechanism to observe the passage of time during playback.
PK_EXTERN dispatch_block_t PKAudioPlayerGetPulseHandler(dispatch_queue_t *outPulseHandlerQueue);

#endif /* PKAudioPlayer_h */
