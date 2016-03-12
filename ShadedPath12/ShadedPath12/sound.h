#pragma once
//#include "DXUT.h"
//#include <xact3.h>
//#include <xact3d3.h>

class CSoundManager;
class CSound;
class CStreamingSound;
class WorldObject;
class XApp;
//class Sound;

enum SoundCategory { MUSIC, EFFECT };

struct SoundDef {
	WAVEFORMATEXTENSIBLE wfx;
	XAUDIO2_BUFFER buffer;
	IXAudio2SourceVoice *voice;
	bool loop = false;
	SoundCategory category;
};


class Sound
{
public:
	Sound(void); 
	~Sound(void);
	int addWorldObject(WorldObject* wo, char *cueName);
	//XApp *xapp;
	unordered_map<string, SoundDef> sounds;
private:
	ComPtr<IXAudio2> xaudio2;
	IXAudio2MasteringVoice* masterVoice = NULL;
	IXAudio2SubmixVoice* submixVoiceBackground; // music channel
	IXAudio2SubmixVoice* submixVoiceEffect;     // effects for world objects
	XAUDIO2_VOICE_SENDS *sfxSendsListMusic;
	XAUDIO2_VOICE_SENDS *sfxSendsListEffect;
	wstring project_filename;
	X3DAUDIO_HANDLE x3dInstance;
	X3DAUDIO_DSP_SETTINGS dspSettings;
	X3DAUDIO_EMITTER emitter;
	X3DAUDIO_LISTENER listener;

	vector<WorldObject*> audibleWorldObjects;  // index used instead of passing WorldObject down to sound class

	int numDoNothingFrames;
	bool recalculateSound();

public:
	void Update();
	void openSoundFile(wstring soundFileName, string id, bool loop = false);
	void playSound(string id, SoundCategory category = EFFECT);
	void lowBackgroundMusicVolume(bool volumeDown = true);
};
