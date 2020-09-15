
// tpbds -- Global settings.

// These are build settings.
// Demo-specific settings are implemented by it's class.

#ifndef _GLOBAL_SETTINGS_H_
#define _GLOBAL_SETTINGS_H_

// Set to 1 for final build: disables all debug features.
#define FINAL_BUILD 0

#if !FINAL_BUILD

// Toggle to skip configuration dialog and assume hardcoded configuration (main.cpp).
#define SKIP_DIALOG 1

// Toggle to mute audio.
const bool kMuteAudio = false;

// Toggle to gracefully catch unhandled exceptions.
#define CRASH_GUARD 0

// Toggle to enable stepping through the audio bar-by-bar (defined by an amount of beats).
// - Seek using keyboard UP and DOWN.
// - Disable when using Rocket.
// - Only works if valid BPM is supplied.
#define AUDIO_STEP 0
const unsigned int kAudioStepNumBeats = 32;

// Toggle to tick the 'windowed' box in the configuration dialog by default.
const bool kPrefWindowed = true;  

#else

#define SKIP_DIALOG 0 // Show dialog.
#define CRASH_GUARD 1 // Crash guard enabled.
#define AUDIO_STEP  0 // Audio stepping disabled.

// Audio enabled.
const bool kMuteAudio  = false;

// Fullscreen preferred.
const bool kPrefWindowed = false;  

#endif // FINAL_BUILD

#endif // _GLOBAL_SETTINGS_H_
