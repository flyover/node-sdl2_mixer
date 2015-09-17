/**
 * Copyright (c) Flyover Games, LLC.  All rights reserved. 
 *  
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated 
 * documentation files (the "Software"), to deal in the Software 
 * without restriction, including without limitation the rights 
 * to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to 
 * whom the Software is furnished to do so, subject to the 
 * following conditions: 
 *  
 * The above copyright notice and this permission notice shall 
 * be included in all copies or substantial portions of the 
 * Software. 
 *  
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY 
 * KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE 
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR 
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 */

#include "node-sdl2_mixer.h"

#include <stdlib.h> // malloc, free
#include <string.h> // strdup

#ifndef strdup
#define strdup(str) strcpy((char*)malloc(strlen(str)+1),str)
#endif

#if defined(__ANDROID__)
#include <android/log.h>
#define printf(...) __android_log_print(ANDROID_LOG_INFO, "printf", __VA_ARGS__)
#endif

#define countof(_a) (sizeof(_a)/sizeof((_a)[0]))

using namespace v8;

namespace node_sdl2_mixer {

// music finished callback

static void _music_finished_set_callback(Local<Function> callback);
static void _music_finished_callback(void);

static Nan::Persistent<Function> s_music_finished_callback;

static void _music_finished_set_callback(Local<Function> callback)
{
	s_music_finished_callback.Reset();
	if (!callback->IsNull())
	{
		s_music_finished_callback.Reset(callback);
	}
}

class TaskMusicFinished : public Nanx::SimpleTask
{
public:
	void DoWork() {}
	void DoAfterWork(int status)
	{
		if (!s_music_finished_callback.IsEmpty())
		{
			Nan::HandleScope scope;
			Mix_HookMusicFinished(NULL);
			Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New<Function>(s_music_finished_callback), 0, NULL);
			Mix_HookMusicFinished(_music_finished_callback);
		}
	}
};

static void _music_finished_callback(void)
{
	int err = Nanx::SimpleTask::Run(new TaskMusicFinished());
}

static void _music_finished_init(void)
{
	Mix_HookMusicFinished(_music_finished_callback);
}

static void _music_finished_quit(void)
{
	Mix_HookMusicFinished(NULL);
	s_music_finished_callback.Reset();
}

// channel finished callback

static void _channel_finished_set_callback(Local<Function> callback);
static void _channel_finished_callback(int channel);

static Nan::Persistent<Function> s_channel_finished_callback;

static void _channel_finished_set_callback(Local<Function> callback)
{
	s_channel_finished_callback.Reset();
	if (!callback->IsNull())
	{
		s_channel_finished_callback.Reset(callback);
	}
}

static void _channel_finished_init(void)
{
	Mix_ChannelFinished(_channel_finished_callback);
}

static void _channel_finished_quit(void)
{
	Mix_ChannelFinished(NULL);
	s_channel_finished_callback.Reset();
}

class TaskChannelFinished : public Nanx::SimpleTask
{
public:
	int m_channel;
public:
	TaskChannelFinished(int channel) : m_channel(channel) {}
	void DoWork() {}
	void DoAfterWork(int status)
	{
		if (!s_channel_finished_callback.IsEmpty())
		{
			Nan::HandleScope scope;
			Local<Value> argv[] = { Nan::New(m_channel) };
			Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New<Function>(s_channel_finished_callback), countof(argv), argv);
		}
	}
};

static void _channel_finished_callback(int channel)
{
	int err = Nanx::SimpleTask::Run(new TaskChannelFinished(channel));
}

// load chunk

class Task_MIX_LoadWav : public Nanx::SimpleTask
{
public:
	Nan::Persistent<Function> m_callback;
	char* m_file;
	Mix_Chunk* m_chunk;
public:
	Task_MIX_LoadWav(Local<String> file, Local<Function> callback) : 
		m_file(strdup(*String::Utf8Value(file))), 
		m_chunk(NULL)
	{
		m_callback.Reset(callback);
	}
	~Task_MIX_LoadWav()
	{
		m_callback.Reset();
		free(m_file); m_file = NULL; // strdup
		if (m_chunk) { Mix_FreeChunk(m_chunk); m_chunk = NULL; }
	}
	void DoWork()
	{
		m_chunk = Mix_LoadWAV(m_file);
	}
	void DoAfterWork(int status)
	{
		Nan::HandleScope scope;
		Local<Value> argv[] = { WrapChunk::Hold(m_chunk) };
		Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New<Function>(m_callback), countof(argv), argv);
		m_chunk = NULL; // script owns pointer
	}
};

// load music

class Task_MIX_LoadMUS : public Nanx::SimpleTask
{
public:
	Nan::Persistent<Function> m_callback;
	char* m_file;
	Mix_Music* m_music;
public:
	Task_MIX_LoadMUS(Local<String> file, Local<Function> callback) : 
		m_file(strdup(*String::Utf8Value(file))), 
		m_music(NULL)
	{
		m_callback.Reset(callback);
	}
	~Task_MIX_LoadMUS()
	{
		m_callback.Reset();
		free(m_file); m_file = NULL; // strdup
		if (m_music) { Mix_FreeMusic(m_music); m_music = NULL; }
	}
	void DoWork()
	{
		m_music = Mix_LoadMUS(m_file);
	}
	void DoAfterWork(int status)
	{
		Nan::HandleScope scope;
		Local<Value> argv[] = { WrapMusic::Hold(m_music) };
		Nan::MakeCallback(Nan::GetCurrentContext()->Global(), Nan::New<Function>(m_callback), countof(argv), argv);
		m_music = NULL; // script owns pointer
	}
};

NANX_EXPORT(Mix_Init)
{
	::Uint32 flags = NANX_Uint32(info[0]);
	int err = Mix_Init(flags);
	if (err < 0)
	{
		printf("Mix_Init error: %d\n", err);
	}
	_music_finished_init();
	_channel_finished_init();
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_Quit)
{
	_channel_finished_quit();
	_music_finished_quit();
	Mix_Quit();
}

NANX_EXPORT(Mix_GetError)
{
	const char *sdl_mixer_error = Mix_GetError();
	info.GetReturnValue().Set(NANX_STRING(sdl_mixer_error));
}

NANX_EXPORT(Mix_ClearError)
{
	SDL_ClearError();
}

NANX_EXPORT(Mix_OpenAudio)
{
	int frequency = NANX_int(info[0]);
	::Uint16 format = NANX_Uint16(info[1]);
	int channels = NANX_int(info[2]);
	int chunksize = NANX_int(info[3]);
	int err = Mix_OpenAudio(frequency, format, channels, chunksize);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_AllocateChannels)
{
	int numchans = NANX_int(info[0]);
	int err = Mix_AllocateChannels(numchans);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_QuerySpec)
{
	Local<Array> _frequency = Local<Array>::Cast(info[0]);
	Local<Array> _format = Local<Array>::Cast(info[1]);
	Local<Array> _channels = Local<Array>::Cast(info[2]);
	int frequency = 0;
	::Uint16 format = 0;
	int channels = 0;
	int err = Mix_QuerySpec(&frequency, &format, &channels);
	_frequency->Set(0, Nan::New(frequency));
	_format->Set(0, Nan::New(format));
	_channels->Set(0, Nan::New(channels));
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_LoadWAV)
{
	Local<String> file = Local<String>::Cast(info[0]);
	Local<Function> callback = Local<Function>::Cast(info[1]);
	int err = Nanx::SimpleTask::Run(new Task_MIX_LoadWav(file, callback));
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_LoadWAV_RW) { Nan::ThrowError("TODO"); }

NANX_EXPORT(Mix_LoadMUS)
{
	Local<String> file = Local<String>::Cast(info[0]);
	Local<Function> callback = Local<Function>::Cast(info[1]);
	int err = Nanx::SimpleTask::Run(new Task_MIX_LoadMUS(file, callback));
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_LoadMUS_RW) { Nan::ThrowError("TODO"); }

NANX_EXPORT(Mix_LoadMUSType_RW) { Nan::ThrowError("TODO"); }

NANX_EXPORT(Mix_QuickLoad_WAV) { Nan::ThrowError("TODO"); }

NANX_EXPORT(Mix_QuickLoad_RAW) { Nan::ThrowError("TODO"); }

NANX_EXPORT(Mix_FreeChunk)
{
	Mix_Chunk* chunk = WrapChunk::Drop(info[0]);
	Mix_FreeChunk(chunk);
}

NANX_EXPORT(Mix_FreeMusic)
{
	Mix_Music* music = WrapMusic::Drop(info[0]);
	Mix_FreeMusic(music);
}

NANX_EXPORT(Mix_GetNumChunkDecoders)
{
	int num = Mix_GetNumChunkDecoders();
	info.GetReturnValue().Set(Nan::New(num));
}

NANX_EXPORT(Mix_GetChunkDecoder)
{
	int index = NANX_int(info[0]);
	const char* name = Mix_GetChunkDecoder(index);
	info.GetReturnValue().Set(NANX_STRING(name));
}

NANX_EXPORT(Mix_GetNumMusicDecoders)
{
	int num = Mix_GetNumMusicDecoders();
	info.GetReturnValue().Set(Nan::New(num));
}

NANX_EXPORT(Mix_GetMusicDecoder)
{
	int index = NANX_int(info[0]);
	const char* name = Mix_GetMusicDecoder(index);
	info.GetReturnValue().Set(NANX_STRING(name));
}

NANX_EXPORT(Mix_GetMusicType)
{
	Mix_Music* music = WrapMusic::Peek(info[0]);
	Mix_MusicType type = Mix_GetMusicType(music);
	info.GetReturnValue().Set(Nan::New(type));
}

NANX_EXPORT(Mix_SetPostMix) { Nan::ThrowError("TODO"); }

NANX_EXPORT(Mix_HookMusic) { Nan::ThrowError("TODO"); }

NANX_EXPORT(Mix_HookMusicFinished)
{
	Local<Function> callback = Local<Function>::Cast(info[0]);
	_music_finished_set_callback(callback);
}

NANX_EXPORT(Mix_GetMusicHookData) { Nan::ThrowError("TODO"); }

NANX_EXPORT(Mix_ChannelFinished)
{
	Local<Function> callback = Local<Function>::Cast(info[0]);
	_channel_finished_set_callback(callback);
}

NANX_EXPORT(Mix_RegisterEffect) { Nan::ThrowError("TODO"); }

NANX_EXPORT(Mix_UnregisterEffect) { Nan::ThrowError("TODO"); }

NANX_EXPORT(Mix_UnregisterAllEffects)
{
	int channel = NANX_int(info[0]);
	int err = Mix_UnregisterAllEffects(channel);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_SetPanning)
{
	int channel = NANX_int(info[0]);
	::Uint8 left = NANX_Uint8(info[1]);
	::Uint8 right = NANX_Uint8(info[2]);
	int err = Mix_SetPanning(channel, left, right);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_SetPosition)
{
	int channel = NANX_int(info[0]);
	::Sint16 angle = NANX_Sint16(info[1]);
	::Uint8 distance = NANX_Uint8(info[2]);
	int err = Mix_SetPosition(channel, angle, distance);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_SetDistance)
{
	int channel = NANX_int(info[0]);
	::Uint8 distance = NANX_Uint8(info[1]);
	int err = Mix_SetDistance(channel, distance);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_SetReverseStereo)
{
	int channel = NANX_int(info[0]);
	int flip = NANX_Sint16(info[1]);
	int err = Mix_SetReverseStereo(channel, flip);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_ReserveChannels)
{
	int num = NANX_int(info[0]);
	int err = Mix_ReserveChannels(num);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_GroupChannel)
{
	int channel = NANX_int(info[0]);
	int tag = NANX_int(info[1]);
	int err = Mix_GroupChannel(channel, tag);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_GroupChannels)
{
	int from = NANX_int(info[0]);
	int to = NANX_int(info[1]);
	int tag = NANX_int(info[2]);
	int err = Mix_GroupChannels(from, to, tag);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_GroupAvailable)
{
	int tag = NANX_int(info[0]);
	int err = Mix_GroupAvailable(tag);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_GroupCount)
{
	int tag = NANX_int(info[0]);
	int err = Mix_GroupCount(tag);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_GroupOldest)
{
	int tag = NANX_int(info[0]);
	int err = Mix_GroupOldest(tag);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_GroupNewer)
{
	int tag = NANX_int(info[0]);
	int err = Mix_GroupNewer(tag);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_PlayChannel)
{
	int channel = NANX_int(info[0]);
	Mix_Chunk* chunk = WrapChunk::Peek(info[1]);
	int loops = NANX_int(info[2]);
	int err = Mix_PlayChannel(channel, chunk, loops);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_PlayChannelTimed)
{
	int channel = NANX_int(info[0]);
	Mix_Chunk* chunk = WrapChunk::Peek(info[1]);
	int loops = NANX_int(info[2]);
	int ticks = NANX_int(info[3]);
	int err = Mix_PlayChannelTimed(channel, chunk, loops, ticks);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_PlayMusic)
{
	Mix_Music* music = WrapMusic::Peek(info[0]);
	int loops = NANX_int(info[1]);
	int err = Mix_PlayMusic(music, loops);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_FadeInMusic)
{
	Mix_Music* music = WrapMusic::Peek(info[0]);
	int loops = NANX_int(info[1]);
	int ms = NANX_int(info[2]);
	int err = Mix_FadeInMusic(music, loops, ms);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_FadeInMusicPos)
{
	Mix_Music* music = WrapMusic::Peek(info[0]);
	int loops = NANX_int(info[1]);
	int ms = NANX_int(info[2]);
	double position = NANX_double(info[3]);
	int err = Mix_FadeInMusicPos(music, loops, ms, position);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_FadeInChannel)
{
	int channel = NANX_int(info[0]);
	Mix_Chunk* chunk = WrapChunk::Peek(info[1]);
	int loops = NANX_int(info[2]);
	int ms = NANX_int(info[3]);
	int err = Mix_FadeInChannel(channel, chunk, loops, ms);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_FadeInChannelTimed)
{
	int channel = NANX_int(info[0]);
	Mix_Chunk* chunk = WrapChunk::Peek(info[1]);
	int loops = NANX_int(info[2]);
	int ms = NANX_int(info[3]);
	int ticks = NANX_int(info[4]);
	int err = Mix_FadeInChannelTimed(channel, chunk, loops, ms, ticks);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_Volume)
{
	int channel = NANX_int(info[0]);
	int volume = NANX_int(info[1]);
	int err = Mix_Volume(channel, volume);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_VolumeChunk)
{
	Mix_Chunk* chunk = WrapChunk::Peek(info[0]);
	int volume = NANX_int(info[1]);
	int err = Mix_VolumeChunk(chunk, volume);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_VolumeMusic)
{
	int volume = NANX_int(info[0]);
	int err = Mix_VolumeMusic(volume);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_HaltChannel)
{
	int channel = NANX_int(info[0]);
	int err = Mix_HaltChannel(channel);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_HaltGroup)
{
	int tag = NANX_int(info[0]);
	int err = Mix_HaltGroup(tag);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_HaltMusic)
{
	int err = Mix_HaltMusic();
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_ExpireChannel)
{
	int channel = NANX_int(info[0]);
	int ticks = NANX_int(info[1]);
	int err = Mix_ExpireChannel(channel, ticks);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_FadeOutChannel)
{
	int channel = NANX_int(info[0]);
	int ms = NANX_int(info[1]);
	int err = Mix_FadeOutChannel(channel, ms);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_FadeOutGroup)
{
	int tag = NANX_int(info[0]);
	int ms = NANX_int(info[1]);
	int err = Mix_FadeOutGroup(tag, ms);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_FadeOutMusic)
{
	int ms = NANX_int(info[0]);
	int err = Mix_FadeOutMusic(ms);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_FadingMusic)
{
	Mix_Fading fading = Mix_FadingMusic();
	info.GetReturnValue().Set(Nan::New(fading));
}

NANX_EXPORT(Mix_FadingChannel)
{
	int channel = NANX_int(info[0]);
	Mix_Fading fading = Mix_FadingChannel(channel);
	info.GetReturnValue().Set(Nan::New(fading));
}

NANX_EXPORT(Mix_Pause)
{
	int channel = NANX_int(info[0]);
	Mix_Pause(channel);
}

NANX_EXPORT(Mix_Resume)
{
	int channel = NANX_int(info[0]);
	Mix_Resume(channel);
}

NANX_EXPORT(Mix_Paused)
{
	int channel = NANX_int(info[0]);
	int ret = Mix_Paused(channel);
	info.GetReturnValue().Set(Nan::New(ret));
}

NANX_EXPORT(Mix_PauseMusic)
{
	Mix_PauseMusic();
}

NANX_EXPORT(Mix_ResumeMusic)
{
	Mix_ResumeMusic();
}

NANX_EXPORT(Mix_RewindMusic)
{
	Mix_RewindMusic();
}

NANX_EXPORT(Mix_PausedMusic)
{
	int ret = Mix_PausedMusic();
	info.GetReturnValue().Set(Nan::New(ret));
}

NANX_EXPORT(Mix_SetMusicPosition)
{
	double position = NANX_double(info[0]);
	int err = Mix_SetMusicPosition(position);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_Playing)
{
	int channel = NANX_int(info[0]);
	int ret = Mix_Playing(channel);
	info.GetReturnValue().Set(Nan::New(ret));
}

NANX_EXPORT(Mix_PlayingMusic)
{
	int ret = Mix_PlayingMusic();
	info.GetReturnValue().Set(Nan::New(ret));
}

NANX_EXPORT(Mix_SetMusicCMD)
{
	Local<String> command = Local<String>::Cast(info[0]);
	int err = Mix_SetMusicCMD(*String::Utf8Value(command));
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_SetSynchroValue)
{
	int value = NANX_int(info[0]);
	int err = Mix_SetSynchroValue(value);
	info.GetReturnValue().Set(Nan::New(err));
}

NANX_EXPORT(Mix_GetSynchroValue)
{
	int ret = Mix_GetSynchroValue();
	info.GetReturnValue().Set(Nan::New(ret));
}

NANX_EXPORT(Mix_SetSoundFonts)
{
	#ifdef MID_MUSIC
	Local<String> fonts = Local<String>::Cast(info[0]);
	int err = Mix_SetSoundFonts(*String::Utf8Value(fonts));
	info.GetReturnValue().Set(Nan::New(err));
	#else
	info.GetReturnValue().Set(Nan::New(-1));
	#endif
}

NANX_EXPORT(Mix_GetSoundFonts)
{
	#ifdef MID_MUSIC
	const char* fonts = Mix_GetSoundFonts();
	info.GetReturnValue().Set(NANX_STRING(fonts));
	#else
	info.GetReturnValue().SetNull();
	#endif
}

#ifdef MID_MUSIC
static Local<Function> s_each_sound_font_callback;
static Local<Value> s_each_sound_font_data;
static int _each_sound_font(const char* font, void* data)
{
	Local<Value> argv[] = { NANX_STRING(font), s_each_sound_font_data };
	Local<Value> res = s_each_sound_font_callback->Call(Context::GetCurrent()->Global(), countof(argv), argv);
	return NANX_int(res);
}
#endif

NANX_EXPORT(Mix_EachSoundFont)
{
	#ifdef MID_MUSIC
	s_each_sound_font_callback = Local<Function>::Cast(info[0]);
	s_each_sound_font_data = info[1];
	int err = Mix_EachSoundFont(_each_sound_font);
	s_each_sound_font_callback.Clear();
	s_each_sound_font_data.Clear();;
	info.GetReturnValue().Set(Nan::New(err));
	#else
	info.GetReturnValue().Set(Nan::New(-1));
	#endif
}

NANX_EXPORT(Mix_GetChunk)
{
	int channel = NANX_int(info[0]);
	Mix_Chunk* chunk = Mix_GetChunk(channel);
	info.GetReturnValue().Set(WrapChunk::Hold(chunk));
}

NANX_EXPORT(Mix_CloseAudio)
{
	Mix_CloseAudio();
}

NAN_MODULE_INIT(init)
{

	// SDL_mixer.h

	NANX_CONSTANT(target, SDL_MIXER_MAJOR_VERSION);
	NANX_CONSTANT(target, SDL_MIXER_MINOR_VERSION);
	NANX_CONSTANT(target, SDL_MIXER_PATCHLEVEL);

	NANX_CONSTANT(target, MIX_INIT_FLAC);
	NANX_CONSTANT(target, MIX_INIT_MOD);
	NANX_CONSTANT(target, MIX_INIT_MODPLUG);
	NANX_CONSTANT(target, MIX_INIT_MP3);
	NANX_CONSTANT(target, MIX_INIT_OGG);
	NANX_CONSTANT(target, MIX_INIT_FLUIDSYNTH);

	NANX_CONSTANT(target, MIX_CHANNELS);
	NANX_CONSTANT(target, MIX_DEFAULT_FREQUENCY);
	NANX_CONSTANT(target, MIX_DEFAULT_FORMAT);
	NANX_CONSTANT(target, MIX_DEFAULT_CHANNELS);
	NANX_CONSTANT(target, MIX_MAX_VOLUME);

	// Mix_Fading
	Local<Object> Fading = Nan::New<Object>();
	target->Set(NANX_SYMBOL("Mix_Fading"), Fading);
	NANX_CONSTANT(Fading, MIX_NO_FADING);
	NANX_CONSTANT(Fading, MIX_FADING_OUT);
	NANX_CONSTANT(Fading, MIX_FADING_IN);

	// Mix_MusicType
	Local<Object> MusicType = Nan::New<Object>();
	target->Set(NANX_SYMBOL("Mix_MusicType"), MusicType);
	NANX_CONSTANT(MusicType, MUS_NONE);
	NANX_CONSTANT(MusicType, MUS_CMD);
	NANX_CONSTANT(MusicType, MUS_WAV);
	NANX_CONSTANT(MusicType, MUS_MOD);
	NANX_CONSTANT(MusicType, MUS_MID);
	NANX_CONSTANT(MusicType, MUS_OGG);
	NANX_CONSTANT(MusicType, MUS_MP3);
	NANX_CONSTANT(MusicType, MUS_MP3_MAD);
	NANX_CONSTANT(MusicType, MUS_FLAC);
	NANX_CONSTANT(MusicType, MUS_MODPLUG);

	NANX_CONSTANT_STRING(target, MIX_EFFECTSMAXSPEED);

	NANX_EXPORT_APPLY(target, Mix_Init);
	NANX_EXPORT_APPLY(target, Mix_Quit);
	NANX_EXPORT_APPLY(target, Mix_GetError);
	NANX_EXPORT_APPLY(target, Mix_ClearError);
	NANX_EXPORT_APPLY(target, Mix_OpenAudio);
	NANX_EXPORT_APPLY(target, Mix_AllocateChannels);
	NANX_EXPORT_APPLY(target, Mix_QuerySpec);
	NANX_EXPORT_APPLY(target, Mix_LoadWAV);
	NANX_EXPORT_APPLY(target, Mix_LoadWAV_RW);
	NANX_EXPORT_APPLY(target, Mix_LoadMUS);
	NANX_EXPORT_APPLY(target, Mix_LoadMUS_RW);
	NANX_EXPORT_APPLY(target, Mix_LoadMUSType_RW);
	NANX_EXPORT_APPLY(target, Mix_QuickLoad_WAV);
	NANX_EXPORT_APPLY(target, Mix_QuickLoad_RAW);
	NANX_EXPORT_APPLY(target, Mix_FreeChunk);
	NANX_EXPORT_APPLY(target, Mix_FreeMusic);
	NANX_EXPORT_APPLY(target, Mix_GetNumChunkDecoders);
	NANX_EXPORT_APPLY(target, Mix_GetChunkDecoder);
	NANX_EXPORT_APPLY(target, Mix_GetNumMusicDecoders);
	NANX_EXPORT_APPLY(target, Mix_GetMusicDecoder);
	NANX_EXPORT_APPLY(target, Mix_GetMusicType);
	NANX_EXPORT_APPLY(target, Mix_SetPostMix);
	NANX_EXPORT_APPLY(target, Mix_HookMusic);
	NANX_EXPORT_APPLY(target, Mix_HookMusicFinished);
	NANX_EXPORT_APPLY(target, Mix_GetMusicHookData);
	NANX_EXPORT_APPLY(target, Mix_ChannelFinished);
	NANX_EXPORT_APPLY(target, Mix_RegisterEffect);
	NANX_EXPORT_APPLY(target, Mix_UnregisterEffect);
	NANX_EXPORT_APPLY(target, Mix_UnregisterAllEffects);
	NANX_EXPORT_APPLY(target, Mix_SetPanning);
	NANX_EXPORT_APPLY(target, Mix_SetPosition);
	NANX_EXPORT_APPLY(target, Mix_SetDistance);
	NANX_EXPORT_APPLY(target, Mix_SetReverseStereo);
	NANX_EXPORT_APPLY(target, Mix_ReserveChannels);
	NANX_EXPORT_APPLY(target, Mix_GroupChannel);
	NANX_EXPORT_APPLY(target, Mix_GroupChannels);
	NANX_EXPORT_APPLY(target, Mix_GroupAvailable);
	NANX_EXPORT_APPLY(target, Mix_GroupCount);
	NANX_EXPORT_APPLY(target, Mix_GroupOldest);
	NANX_EXPORT_APPLY(target, Mix_GroupNewer);
	NANX_EXPORT_APPLY(target, Mix_PlayChannel);
	NANX_EXPORT_APPLY(target, Mix_PlayChannelTimed);
	NANX_EXPORT_APPLY(target, Mix_PlayMusic);
	NANX_EXPORT_APPLY(target, Mix_FadeInMusic);
	NANX_EXPORT_APPLY(target, Mix_FadeInMusicPos);
	NANX_EXPORT_APPLY(target, Mix_FadeInChannel);
	NANX_EXPORT_APPLY(target, Mix_FadeInChannelTimed);
	NANX_EXPORT_APPLY(target, Mix_Volume);
	NANX_EXPORT_APPLY(target, Mix_VolumeChunk);
	NANX_EXPORT_APPLY(target, Mix_VolumeMusic);
	NANX_EXPORT_APPLY(target, Mix_HaltChannel);
	NANX_EXPORT_APPLY(target, Mix_HaltGroup);
	NANX_EXPORT_APPLY(target, Mix_HaltMusic);
	NANX_EXPORT_APPLY(target, Mix_ExpireChannel);
	NANX_EXPORT_APPLY(target, Mix_FadeOutChannel);
	NANX_EXPORT_APPLY(target, Mix_FadeOutGroup);
	NANX_EXPORT_APPLY(target, Mix_FadeOutMusic);
	NANX_EXPORT_APPLY(target, Mix_FadingMusic);
	NANX_EXPORT_APPLY(target, Mix_FadingChannel);
	NANX_EXPORT_APPLY(target, Mix_Pause);
	NANX_EXPORT_APPLY(target, Mix_Resume);
	NANX_EXPORT_APPLY(target, Mix_Paused);
	NANX_EXPORT_APPLY(target, Mix_PauseMusic);
	NANX_EXPORT_APPLY(target, Mix_ResumeMusic);
	NANX_EXPORT_APPLY(target, Mix_RewindMusic);
	NANX_EXPORT_APPLY(target, Mix_PausedMusic);
	NANX_EXPORT_APPLY(target, Mix_SetMusicPosition);
	NANX_EXPORT_APPLY(target, Mix_Playing);
	NANX_EXPORT_APPLY(target, Mix_PlayingMusic);
	NANX_EXPORT_APPLY(target, Mix_SetMusicCMD);
	NANX_EXPORT_APPLY(target, Mix_SetSynchroValue);
	NANX_EXPORT_APPLY(target, Mix_GetSynchroValue);
	NANX_EXPORT_APPLY(target, Mix_SetSoundFonts);
	NANX_EXPORT_APPLY(target, Mix_GetSoundFonts);
	NANX_EXPORT_APPLY(target, Mix_EachSoundFont);
	NANX_EXPORT_APPLY(target, Mix_GetChunk);
	NANX_EXPORT_APPLY(target, Mix_CloseAudio);
}

} // namespace node_sdl2_mixer

NODE_MODULE(node_sdl2_mixer, node_sdl2_mixer::init)
