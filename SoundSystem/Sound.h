#pragma once
#pragma comment(lib, "Xaudio2.lib")
#include <xaudio2.h>
#include <vector>
#include <memory>
#include <algorithm>
#include <string>
#include <assert.h>
#include <wrl/client.h>
#include "ComManager.h"


class SoundSystem
{
public:
	class Channel
	{
	private:
		class VoiceCallback : public IXAudio2VoiceCallback
		{
		public:
			void STDMETHODCALLTYPE OnStreamEnd() override
			{}
			void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override
			{}
			void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 SampleRequired) override
			{}
			void STDMETHODCALLTYPE OnBufferEnd(void* pBufferContext) override;
			void STDMETHODCALLTYPE OnBufferStart(void* pBufferContext) override
			{}
			void STDMETHODCALLTYPE OnLoopEnd(void* pBufferContext) override
			{}
			void STDMETHODCALLTYPE OnVoiceError(void* pBufferContext, HRESULT Error) override
			{}
			
		};
	public:
		Channel(SoundSystem& sys)
		{
			static VoiceCallback vcb;
			ZeroMemory(&xaBuffer, sizeof(xaBuffer));
			xaBuffer.pContext = this;
			sys.pSoundEngine->CreateSourceVoice(&pSource, &sys.format, 0u, 2.0f, &vcb);
		}
		~Channel()
		{
			assert(!pSound);
			if (pSource)
			{
				pSource->DestroyVoice();
				pSource = nullptr;
			}
		}
		void Play(class Sound& s);
		void Stop()
		{
			assert(pSource&& pSound);
			pSource->Stop();
			pSource->FlushSourceBuffers();
		}
	private:
		XAUDIO2_BUFFER xaBuffer;
		IXAudio2SourceVoice* pSource = nullptr;
		class Sound* pSound = nullptr;

	};
public:
	static SoundSystem& Get();
	static WAVEFORMATEX& GetFormat();
	void PlaySound(class Sound& s);
private:
	SoundSystem();
	void DeactivateChannel(Channel& channel);

private:
	//Added ComManager to Init and UnInitCom
	ComManager comMan;
	Microsoft::WRL::ComPtr<IXAudio2> pSoundEngine;
	IXAudio2MasteringVoice* pMasterVoice = nullptr;
	WAVEFORMATEX format;
	const int nChannels = 64;
	std::vector<std::unique_ptr<Channel>> idleChannelPtrs;
	std::vector<std::unique_ptr<Channel>> activeChannelPtrs;
};

class Sound
{
	friend SoundSystem::Channel;

public:
	Sound(const std::wstring& filename);
	void Play();
	~Sound();

private:
	void RemoveChannel(SoundSystem::Channel& channel);
	void AddChannel(SoundSystem::Channel& channel);

private:
	UINT32 nBytes = 0;
	std::unique_ptr<BYTE[]> pData;
	std::vector<SoundSystem::Channel*> activeChannelPtrs;

};

