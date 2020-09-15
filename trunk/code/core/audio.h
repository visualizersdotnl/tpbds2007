
// tpbds -- Audio support (using BASS audio library).

#ifndef _AUDIO_H_
#define _AUDIO_H_

// Returns number of available audio devices.
// If this returns 0, audio will still work through the default device.
unsigned int Audio_GetDeviceCount();

// Returns a zero-terminated string with a device description for iDevice.
// Returns NULL if iDevice is out of range.
const char *Audio_GetDeviceDescription(unsigned int iDevice);

// iDevice     - Valid device index or -1 for system default.
// BPM         - Can be -1 but must be a valid BPM for Auido_BeatSeek() or Rocket support to work properly.
// rowsPerBeat - Rocket track granularity (disregard if not using Rocket), typically 16 or 32.
bool Audio_Create(unsigned int iDevice, const std::string &mp3Path, HWND hWnd, float BPM, unsigned int rowsPerBeat = 1);
void Audio_Destroy();

void Audio_Start(bool loopMusic);
float Audio_Update(); // Returns stream playback time in seconds.

void Audio_Mute();
void Audio_UnMute();

// If BPM is supplied one can seek back and forth an amount of beats.
// A backwards seek automatically skips to the begin of the previous bar.
// Do not use when using Rocket!
void Audio_BeatSeek(bool seekBackwards, unsigned int numBeats);

// Hooks for Rocket.
// Bind them yourself using sync_set_callbacks().
void Audio_Rocket_Pause(void *, int bPause);
void Audio_Rocket_SetRow(void *, int row);
int Audio_Rocket_IsPlaying(void *);

// Returns row position for Rocket.
double Audio_Rocket_GetRow();

#endif // _AUDIO_H_	
