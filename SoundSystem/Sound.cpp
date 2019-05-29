#include "Sound.h"
#include <fstream>
#include <exception>

SoundSystem & SoundSystem::Get()
{
	static SoundSystem instance;
	return instance;
}

WAVEFORMATEX & SoundSystem::GetFormat()
{
	return Get().format;
}

void SoundSystem::PlaySound(Sound & s)
{
	if (idleChannelPtrs.size() > 0)
	{
		activeChannelPtrs.push_back(std::move(idleChannelPtrs.back()));
		idleChannelPtrs.pop_back(); // after moving need to delete unique ptr container
		activeChannelPtrs.back()->Play(s);
	}
}

SoundSystem::SoundSystem()
{
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 2;
	format.nSamplesPerSec = 44100;
	format.nAvgBytesPerSec = 176400;
	format.nBlockAlign = 4;
	format.wBitsPerSample = 16;
	format.cbSize = 0;
	
	XAudio2Create(&pSoundEngine);
	pSoundEngine->CreateMasteringVoice(&pMasterVoice);

	for (int i = 0; i < nChannels; i++)
	{
		idleChannelPtrs.push_back(std::make_unique<Channel>(*this)); //this is reference to sound system itself
	}
}

void SoundSystem::DeactivateChannel(Channel & channel)
{
	auto i = std::find_if(activeChannelPtrs.begin(), activeChannelPtrs.end(),
		[&channel](const std::unique_ptr<Channel>& pChain) -> bool
	{
		return &channel == pChain.get();
	});
	idleChannelPtrs.push_back(std::move(*i));
	activeChannelPtrs.erase(i);

}

Sound::Sound(const std::wstring & filename)
{
	unsigned int fileSize = 0;
	std::unique_ptr<BYTE[]> pFileIn;
	{
		std::ifstream file(filename, std::ios_base::binary);
		{
			int fourcc;
			file.read(reinterpret_cast<char*>(&fourcc), 4);
			if (fourcc != 'FFIR')
			{
				throw std::runtime_error("bad fourCC");
			}
		}

		file.read(reinterpret_cast<char*>(&fileSize), 4);
		if (fileSize <= 16)
		{
			throw std::runtime_error("file too small");
		}

		file.seekg(0, std::ios::beg);
		pFileIn = std::make_unique<BYTE[]>(fileSize);
		file.read(reinterpret_cast<char*>(pFileIn.get()), fileSize);

	}

	if (*reinterpret_cast<const int*>(&pFileIn[8]) != 'EVAW')
	{
		throw std::runtime_error("format not WAVE");
	}

	WAVEFORMATEX format;
	bool bFilledFormat = false;
	for (unsigned int i = 12; i < fileSize; )
	{
		if (*reinterpret_cast<const int*>(&pFileIn[i]) == ' tmf')
		{
			memcpy(&format, &pFileIn[i + 8], sizeof(format));
			bFilledFormat = true;
			break;
		}
		//chunk size + size entry size + chunk id entry size + word padding
		i += (*reinterpret_cast<const int*>(&pFileIn[i + 4]) + 9) & 0xFFFFFFFE;
	}
	if (!bFilledFormat)
	{
		throw std::runtime_error("fmt chunk not found");
	}

	//compare format with sound system format
	{
		const WAVEFORMATEX sysFormat = SoundSystem::GetFormat();

		if (format.nChannels != sysFormat.nChannels)
		{
			throw std::runtime_error("bad wave format (nChannels)");
		}
		else if (format.wBitsPerSample != sysFormat.wBitsPerSample)
		{
			throw std::runtime_error("bad wave format (wBitsPerSample)");
		}
		else if (format.nSamplesPerSec != sysFormat.nSamplesPerSec)
		{
			throw std::runtime_error("bad wave format (nSamplesPerSec)");
		}
		else if (format.wFormatTag != sysFormat.wFormatTag)
		{
			throw std::runtime_error("bad wave format (wFormatTag)");
		}
		else if (format.nBlockAlign != sysFormat.nBlockAlign)
		{
			throw std::runtime_error("bad wave format (nBlockAlign)");
		}
		else if (format.nAvgBytesPerSec != sysFormat.nAvgBytesPerSec)
		{
			throw std::runtime_error("bad wave format (nAvgBytesPerSec)");
		}
	}

	//look for data chunk id
	bool bFilledData = false;
	for (unsigned int i = 12; i < fileSize; )
	{
		const int chunkSize = *reinterpret_cast<const int*>(&pFileIn[i + 4]);
		if (*reinterpret_cast<const int*>(&pFileIn[i]) == 'atad')
		{
			pData = std::make_unique<BYTE[]>(chunkSize);
			nBytes = chunkSize;
			memcpy(pData.get(), &pFileIn[i + 8], nBytes);

			bFilledData = true;
			break;
		}
		//chunk size + size entry size + chunk id entry size + word padding
		i += (chunkSize + 9) & 0xFFFFFFFE;
	}
	if (!bFilledData)
	{
		throw std::runtime_error("data chunk not found");
	}
}

void Sound::Play()
{
	SoundSystem::Get().PlaySound(*this);
}

Sound::~Sound()
{
	for (auto pChannel : activeChannelPtrs)
	{
		pChannel->Stop();
	}
	while (activeChannelPtrs.size() > 0);
}

void Sound::RemoveChannel(SoundSystem::Channel & channel)
{
	activeChannelPtrs.erase(std::find(activeChannelPtrs.begin(),
		activeChannelPtrs.end(), &channel));
}

void Sound::AddChannel(SoundSystem::Channel & channel)
{
	activeChannelPtrs.push_back(&channel);
}

void SoundSystem::Channel::Play(Sound & s)
{
	assert(pSource && !pSound);
	s.AddChannel(*this);
	pSound = &s;
	xaBuffer.pAudioData = s.pData.get();
	xaBuffer.AudioBytes = s.nBytes;
	pSource->SubmitSourceBuffer(&xaBuffer, nullptr);
	pSource->Start();
}

void SoundSystem::Channel::VoiceCallback::OnBufferEnd(void * pBufferContext)
{
	Channel& chan = *(Channel*)pBufferContext;
	chan.pSound->RemoveChannel(chan);
	chan.pSound = nullptr;
	SoundSystem::Get().DeactivateChannel(chan);
}
