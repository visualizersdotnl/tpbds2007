
// tpbds -- Audio support (using BASS audio library).

#include "main.h"
#include "../../3rdparty/bass/c/bass.h"

static HSTREAM s_hMP3 = NULL;

static float s_BPM; // Can be -1 only if not using Audio_BeatSeek() or Rocket.
static float s_rocketRowRate; // Only relevant if using Rocket.

unsigned int Audio_GetDeviceCount()
{
	unsigned int devCount = 0;
	while (BASS_GetDeviceDescription(devCount + 1) != NULL) ++devCount; // Only enumarate *actual* devices (>= 1).
	return devCount;
}

const char *Audio_GetDeviceDescription(unsigned int iDevice)
{
	return BASS_GetDeviceDescription(iDevice + 1);
}

bool Audio_Create(unsigned int iDevice, const std::string &mp3Path, HWND hWnd, float BPM, unsigned int rowsPerBeat /* = 1 */)
{
	TPB_ASSERT(iDevice == -1 || iDevice < Audio_GetDeviceCount());
	TPB_ASSERT(hWnd != NULL);
	TPB_ASSERT(BPM > 0.f || BPM == -1.f);

	if (BPM == -1.f)
	{
		s_BPM = -1.f;
		s_rocketRowRate = 0.f;
	}
	else
	{
		s_BPM = BPM;
		TPB_ASSERT(rowsPerBeat > 0);
		s_rocketRowRate = BPM / 60.f * (float) rowsPerBeat;
	}

	// Bass device IDs:
	//  0 = No sound (causes functionality to be limited, so -1 is the better pick).
	// -1 = Default.
	// >0 = As enumerated.
	if (!BASS_Init((iDevice == -1) ? -1 : iDevice + 1, 44100, BASS_DEVICE_LATENCY, hWnd, NULL))
	{ 
		switch (BASS_ErrorGetCode())
		{
		case BASS_ERROR_DEVICE:
		case BASS_ERROR_ALREADY:
		case BASS_ERROR_NO3D:
		case BASS_ERROR_UNKNOWN:
		case BASS_ERROR_MEM:
			TPB_ASSERT(0);

		case BASS_ERROR_DRIVER:
		case BASS_ERROR_FORMAT:
			SetLastError("Can not initialize BASS audio library @ 44.1 kHz.");
			return false;
		}
	}

	s_hMP3 = BASS_StreamCreateFile(FALSE, mp3Path.c_str(), 0, 0, 0 /* BASS_UNICODE */);
	if (s_hMP3 == NULL)
	{
		switch (BASS_ErrorGetCode())
		{
		case BASS_ERROR_INIT:
		case BASS_ERROR_NOTAVAIL:
		case BASS_ERROR_ILLPARAM:
		case BASS_ERROR_NO3D:
		case BASS_ERROR_FILEFORM:
		case BASS_ERROR_CODEC:
		case BASS_ERROR_FORMAT:
		case BASS_ERROR_SPEAKER:
		case BASS_ERROR_MEM:
			TPB_ASSERT(0);

		case BASS_ERROR_FILEOPEN:
		case BASS_ERROR_UNKNOWN:			
			SetLastError("Can not load MP3: " + mp3Path);
			return false;
		}
	}

	return true;
}

void Audio_Destroy()
{
	BASS_Free();
}

void Audio_Start(bool loopMusic)
{
	TPB_ASSERT(s_hMP3 != NULL);
	BASS_ChannelPlay(s_hMP3, FALSE);
	if (loopMusic) BASS_ChannelSetFlags(s_hMP3, BASS_SAMPLE_LOOP);
}

float Audio_Update()
{
	TPB_ASSERT(s_hMP3 != NULL);

	// This is supposed to yield better playback load balancing.
	BASS_Update();

	const QWORD chanPos = BASS_ChannelGetPosition(s_hMP3);
	const float secPos = BASS_ChannelBytes2Seconds(s_hMP3, chanPos);
	return secPos;
}

void Audio_Mute()
{
	TPB_ASSERT(s_hMP3 != NULL);
	BASS_ChannelSetAttributes(s_hMP3, -1, 0, -101);
}

void Audio_UnMute()
{
	TPB_ASSERT(s_hMP3 != NULL);
	BASS_ChannelSetAttributes(s_hMP3, -1, 100, -101);
}

void Audio_BeatSeek(bool seekBackwards, unsigned int numBeats)
{
	TPB_ASSERT(s_hMP3 != NULL && s_BPM != -1.f);
	if (numBeats > 0)
	{
		const QWORD chanPos = BASS_ChannelGetPosition(s_hMP3);
		const float secPos = BASS_ChannelBytes2Seconds(s_hMP3, chanPos);

		const float beatLenInSecs = 60.f / s_BPM;
		const unsigned int curBeat = (unsigned int) floorf(secPos / beatLenInSecs);
		
		unsigned int newBeat;
		if (!seekBackwards)
		{
			// Skip to next bar.
			// Code below automatically loops if we go past stream length.
			const unsigned int beatsToSkip = numBeats - curBeat % numBeats;
			newBeat = curBeat + beatsToSkip;
		}
		else
		{
			// Skip to beginning of previous bar. 
			newBeat = curBeat - curBeat % numBeats;
			if (newBeat >= numBeats)
				newBeat -= numBeats;
			else
				newBeat = 0; // Clamp.
		}
	
		float newSecPos = (float) newBeat * beatLenInSecs;

		const QWORD chanLenBytes = BASS_ChannelGetLength(s_hMP3);
		const float chanLenSecs = BASS_ChannelBytes2Seconds(s_hMP3, chanLenBytes);
		if (newSecPos > chanLenSecs) newSecPos = 0.f; // Loop!

		// Convert to bytes and set.
		QWORD newChanPos = BASS_ChannelSeconds2Bytes(s_hMP3, newSecPos);
		BASS_ChannelSetPosition(s_hMP3, newChanPos);		
	}
}

void Audio_Rocket_Pause(void *, int bPause)
{
	TPB_ASSERT(s_hMP3 != NULL);
	if (bPause)
		BASS_ChannelPause(s_hMP3);
	else
		BASS_ChannelPlay(s_hMP3, false);
}

void Audio_Rocket_SetRow(void *, int row)
{
	TPB_ASSERT(s_hMP3 != NULL && s_BPM != -1.f);
	const QWORD newChanPos = BASS_ChannelSeconds2Bytes(s_hMP3, (float) row / s_rocketRowRate);
	BASS_ChannelSetPosition(s_hMP3, newChanPos);
}

int Audio_Rocket_IsPlaying(void *)
{
	TPB_ASSERT(s_hMP3 != NULL);
	return BASS_ACTIVE_PLAYING == BASS_ChannelIsActive(s_hMP3);
}

double Audio_Rocket_GetRow()
{
	TPB_ASSERT(s_hMP3 != NULL && s_BPM != -1.f);
	const QWORD chanPos = BASS_ChannelGetPosition(s_hMP3);
	const float secPos = BASS_ChannelBytes2Seconds(s_hMP3, chanPos);
	return (double) floorf(secPos * s_rocketRowRate);
}
