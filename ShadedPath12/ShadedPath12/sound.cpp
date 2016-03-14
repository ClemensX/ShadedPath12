#include "stdafx.h"

// always assume 2 source channels, TODO ? check
#define SRC_CHANNEL_COUNT 2

Sound::Sound(void)
{
	numDoNothingFrames = 1;
	HRESULT hr;
#ifdef BUILD_WIN7
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ThrowIfFailed(hr);
#else
	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ThrowIfFailed(hr);
#endif
	UINT32 flags = 0;
#if defined (_DEBUG)
	//flags |= XAUDIO2_DEBUG_ENGINE;
#endif
	hr = XAudio2Create(&xaudio2, flags, XAUDIO2_DEFAULT_PROCESSOR);
	ThrowIfFailed(hr);
	hr = xaudio2->CreateMasteringVoice(&masterVoice);
	ThrowIfFailed(hr); 
	ThrowIfFailed(xaudio2->CreateSubmixVoice(&submixVoiceBackground, 2, 44100, 0, 0, 0, 0));
	static XAUDIO2_SEND_DESCRIPTOR SFXSendMusic = { 0, submixVoiceBackground };
	static XAUDIO2_VOICE_SENDS SFXSendListMusic = { 1, &SFXSendMusic };
	sfxSendsListMusic = &SFXSendListMusic;
	ThrowIfFailed(xaudio2->CreateSubmixVoice(&submixVoiceEffect, 2, 44100, 0, 0, 0, 0));
	static XAUDIO2_SEND_DESCRIPTOR SFXSendEffect = { 0, submixVoiceEffect };
	static XAUDIO2_VOICE_SENDS SFXSendListEffect = { 1, &SFXSendEffect };
	sfxSendsListEffect = &SFXSendListEffect;
	//UINT32 count;
	//xaudio2->GetDeviceCount(&count);
	//Log("xaudio2 device count == " << count << endl);
	// use decice 0 (== global default sudio device)
#ifdef BUILD_WIN7
	XAUDIO2_DEVICE_DETAILS deviceDetails;
	ThrowIfFailed(xaudio2->GetDeviceDetails(0, &deviceDetails));
	DWORD channelMask = deviceDetails.OutputFormat.dwChannelMask;
	UINT32 dstChannelCount = deviceDetails.OutputFormat.Format.nChannels;
#else
	DWORD channelMask;
	masterVoice->GetChannelMask(&channelMask);
	XAUDIO2_VOICE_DETAILS deviceDetails;
	masterVoice->GetVoiceDetails(&deviceDetails);
	UINT32 dstChannelCount = deviceDetails.InputChannels;
#endif
	X3DAudioInitialize(channelMask, X3DAUDIO_SPEED_OF_SOUND, x3dInstance);

	// setup DSP
	ZeroMemory(&dspSettings, sizeof(dspSettings));
	dspSettings.SrcChannelCount = SRC_CHANNEL_COUNT;
	dspSettings.DstChannelCount = dstChannelCount;
	dspSettings.pMatrixCoefficients = new FLOAT32[dspSettings.SrcChannelCount * dspSettings.DstChannelCount];
	FLOAT32 *azimuths = new FLOAT32[dspSettings.DstChannelCount];

	// setup emitter and listener
	ZeroMemory(&listener, sizeof(X3DAUDIO_LISTENER));
	listener.OrientFront.z = 1.0f;
	listener.OrientTop.y = 1.0f;

	ZeroMemory(&emitter, sizeof(X3DAUDIO_EMITTER));
	emitter.pCone = NULL;
	emitter.OrientFront.z = 1.0f;
	emitter.OrientTop.y = 1.0f;
	emitter.ChannelCount = dspSettings.DstChannelCount;
	emitter.ChannelRadius = 1.0f;
	emitter.pChannelAzimuths = azimuths;
	emitter.pVolumeCurve = NULL;
	emitter.pLFECurve = NULL;
	emitter.pLPFDirectCurve = NULL;
	emitter.pLPFReverbCurve = NULL;
	emitter.pReverbCurve = NULL;
	emitter.CurveDistanceScaler = 8.0f; // was 2.0 / 1.0 TODO: changable via app
	emitter.DopplerScaler = NULL;
}

Sound::~Sound(void)
{
	for (auto s : sounds) {
		if (s.second.voice) {
			s.second.voice->DestroyVoice();
		}
		delete s.second.buffer.pAudioData;
	}
	submixVoiceBackground->DestroyVoice();
	submixVoiceEffect->DestroyVoice();
	masterVoice->DestroyVoice();
	delete dspSettings.pMatrixCoefficients;
	delete emitter.pChannelAzimuths;
}

void Sound::Update() {
	Camera* cam = &xapp().camera;;
	//HRESULT hr;
	if (cam && recalculateSound()) {
		// update 3d sounds
		listener.OrientFront.x = cam->look.x;
		listener.OrientFront.y = cam->look.y;
		listener.OrientFront.z = cam->look.z;
		listener.OrientTop.x = cam->up.x;
		listener.OrientTop.y = cam->up.y;
		listener.OrientTop.z = cam->up.z;
		listener.Position.x = cam->pos.x;
		listener.Position.y = cam->pos.y;
		listener.Position.z = cam->pos.z;
		for (int i = 0; i < audibleWorldObjects.size(); i++) {
			WorldObject *wo = audibleWorldObjects[i];
			if (wo->soundDef == nullptr) {
				continue;  // nothing to play right now
			}
			emitter.Position.x = wo->pos().x;
			emitter.Position.y = wo->pos().y;
			emitter.Position.z = wo->pos().z;
			emitter.ChannelCount = wo->soundDef->wfx.Format.nChannels;
			dspSettings.SrcChannelCount = emitter.ChannelCount;
			//emitter.pChannelAzimuths[0] = 4.71f;
			//emitter.pChannelAzimuths[1] = 1.57f;
			X3DAudioCalculate(x3dInstance, &listener, &emitter, X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER | X3DAUDIO_CALCULATE_LPF_DIRECT | X3DAUDIO_CALCULATE_REVERB, &dspSettings);
			float distance = dspSettings.EmitterToListenerDistance;
			float maxDistance = (float)wo->maxListeningDistance;
			//Log(" distance " << dspSettings.EmitterToListenerDistance << "\n");
			SoundDef *s = wo->soundDef;
			if (true || !wo->stopped) {
				s->voice->SetFrequencyRatio(dspSettings.DopplerFactor);
				s->voice->SetOutputMatrix(submixVoiceEffect, emitter.ChannelCount, dspSettings.DstChannelCount, dspSettings.pMatrixCoefficients);
				XAUDIO2_FILTER_PARAMETERS FilterParameters = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * dspSettings.LPFDirectCoefficient), 1.0f };
				float current_volume;
				submixVoiceEffect->GetVolume(&current_volume);
				submixVoiceEffect->SetVolume(current_volume);
			}
			/*if (!wo->playing) {
			V(wo->cue->Play());
			wo->playing = true;
			}
			if (!wo->stopped && distance > maxDistance) {
			// moved outside listening distance
			wo->cue->Pause(true);
			wo->stopped = true;
			oss << " stop \n";
			Blender::Log(oss.str());
			}
			else if (wo->stopped && distance <= maxDistance) {
			// moved inside listening distance
			wo->cue->Pause(false);
			wo->stopped = false;
			oss << " run \n";
			Blender::Log(oss.str());
			}*/

		}
	}
}

#ifdef _XBOX //Big-Endian
#define fourccRIFF 'RIFF'
#define fourccDATA 'data'
#define fourccFMT 'fmt '
#define fourccWAVE 'WAVE'
#define fourccXWMA 'XWMA'
#define fourccDPDS 'dpds'
#endif

#ifndef _XBOX //Little-Endian
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
#endif
HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD & dwChunkSize, DWORD & dwChunkDataPosition)
{
	HRESULT hr = S_OK;
	if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
		return HRESULT_FROM_WIN32(GetLastError());

	DWORD dwChunkType;
	DWORD dwChunkDataSize;
	DWORD dwRIFFDataSize = 0;
	DWORD dwFileType;
	DWORD bytesRead = 0;
	DWORD dwOffset = 0;

	while (hr == S_OK)
	{
		DWORD dwRead;
		if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
			hr = HRESULT_FROM_WIN32(GetLastError());

		if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
			hr = HRESULT_FROM_WIN32(GetLastError());

		switch (dwChunkType)
		{
		case fourccRIFF:
			dwRIFFDataSize = dwChunkDataSize;
			dwChunkDataSize = 4;
			if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
				hr = HRESULT_FROM_WIN32(GetLastError());
			break;

		default:
			if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
				return HRESULT_FROM_WIN32(GetLastError());
		}

		dwOffset += sizeof(DWORD)* 2;

		if (dwChunkType == fourcc)
		{
			dwChunkSize = dwChunkDataSize;
			dwChunkDataPosition = dwOffset;
			return S_OK;
		}

		dwOffset += dwChunkDataSize;

		if (bytesRead >= dwRIFFDataSize) return S_FALSE;

	}

	return S_OK;

}

HRESULT ReadChunkData(HANDLE hFile, void * buffer, DWORD buffersize, DWORD bufferoffset)
{
	HRESULT hr = S_OK;
	if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
		return HRESULT_FROM_WIN32(GetLastError());
	DWORD dwRead;
	if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
		hr = HRESULT_FROM_WIN32(GetLastError());
	return hr;
}

// taken from msdn: http://msdn.microsoft.com/en-us/library/windows/desktop/ee415781%28v=vs.85%29.aspx
void openSoundWithXAudio2(SoundDef &soundDef, wstring binFile) {
	HRESULT hr;
	//soundDef.wfx = { 0 };
	//soundDef.buffer = { 0 };
#ifdef _XBOX
	char * strFileName = "game:\\media\\MusicMono.wav";
#else
	const wchar_t *strFileName = binFile.c_str();
#endif
	// Open the file
	HANDLE hFile = CreateFile(
		strFileName,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (INVALID_HANDLE_VALUE == hFile) {
		hr = HRESULT_FROM_WIN32(GetLastError());
		ThrowIfFailed(hr);
	}

	if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN)) {
		hr = HRESULT_FROM_WIN32(GetLastError());
		ThrowIfFailed(hr);
	}
	DWORD dwChunkSize;
	DWORD dwChunkPosition;
	//check the file type, should be fourccWAVE or 'XWMA'
	ThrowIfFailed(FindChunk(hFile, fourccRIFF, dwChunkSize, dwChunkPosition));
	DWORD filetype;
	ThrowIfFailed(ReadChunkData(hFile, &filetype, sizeof(DWORD), dwChunkPosition));
	if (filetype != fourccWAVE) {
		Error(L"wrong filetype");
	}
	ThrowIfFailed(FindChunk(hFile, fourccFMT, dwChunkSize, dwChunkPosition));
	ThrowIfFailed(ReadChunkData(hFile, &soundDef.wfx, dwChunkSize, dwChunkPosition));

	//fill out the audio data buffer with the contents of the fourccDATA chunk
	ThrowIfFailed(FindChunk(hFile, fourccDATA, dwChunkSize, dwChunkPosition));
	BYTE * pDataBuffer = new BYTE[dwChunkSize];
	ThrowIfFailed(ReadChunkData(hFile, pDataBuffer, dwChunkSize, dwChunkPosition));
	soundDef.buffer.AudioBytes = dwChunkSize;  //buffer containing audio data
	soundDef.buffer.pAudioData = pDataBuffer;  //size of the audio buffer in bytes
	soundDef.buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer

	// loop seetings:
	if (soundDef.loop) {
		soundDef.buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
	}
}

VOID Sound::openSoundFile(wstring fileName, string id, bool loop)
{
	HWND hWnd = nullptr;//DXUTGetHWND();
	//HRESULT hr;
	wstring binFile = xapp().findFile(fileName.c_str(), XApp::SOUND);
	SoundDef sd;
	if (sounds.count(id) == 0) {
		ZeroMemory(&sd, sizeof(sd));
	} else {
		sd = sounds[id];
	}
	sd.loop = loop;
	openSoundWithXAudio2(sd, binFile);
	sounds[id] = sd;
	return;
}

void Sound::playSound(string id, SoundCategory category) {
	HRESULT hr;
	assert(sounds.count(id) > 0);
	SoundDef *sound = &sounds[id];
	sound->category = category;
	XAUDIO2_VOICE_SENDS *sendsList = category == MUSIC ? sfxSendsListMusic : sfxSendsListEffect;
	hr = xaudio2->CreateSourceVoice(&sound->voice, (WAVEFORMATEX*)&sound->wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, sendsList, nullptr);
	ThrowIfFailed(hr);
	hr = sound->voice->SubmitSourceBuffer(&sound->buffer);
	ThrowIfFailed(hr);
	hr = sound->voice->Start(0);
	float current_volume;
	sound->voice->GetVolume(&current_volume);
	if (category == MUSIC) {
		sound->voice->SetVolume(current_volume/15.0f); // TODO cleanup
	}
	ThrowIfFailed(hr);
}

void Sound::lowBackgroundMusicVolume(bool volumeDown) {
	//SoundDef *sound = &sounds[id];
	if (volumeDown) {
		this->submixVoiceBackground->SetVolume(0.5f);
	} else {
		this->submixVoiceBackground->SetVolume(1.0f);
	}
}

int Sound::addWorldObject(WorldObject* wo, char *cueName) {
	audibleWorldObjects.push_back(wo);
	return -1;
}

#define DO_NOTHING_FRAME_COUNT 5

bool Sound::recalculateSound() {
	if (numDoNothingFrames <= 0) {
		numDoNothingFrames = DO_NOTHING_FRAME_COUNT;
		return true;
	} else {
		numDoNothingFrames--;
		return false;
	}
}

//XApp* Sound::xapp = nullptr;