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

#include <v8.h>
#include <node.h>
#include <SDL.h>
#include <SDL_mixer.h>

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

static void _music_finished_set_callback(Handle<Function> callback);
static void _music_finished_callback(void);

static Persistent<Function> s_music_finished_callback;

static void _music_finished_set_callback(Handle<Function> callback)
{
	NanDisposePersistent(s_music_finished_callback);
	if (!callback->IsNull())
	{
		NanAssignPersistent(s_music_finished_callback, callback);
	}
}

class TaskMusicFinished : public node_sdl2::SimpleTask
{
public:
	void DoWork() {}
	void DoAfterWork(int status)
	{
		if (!s_music_finished_callback.IsEmpty())
		{
			Mix_HookMusicFinished(NULL);
			NanMakeCallback(NanGetCurrentContext()->Global(), NanNew<Function>(s_music_finished_callback), 0, NULL);
			Mix_HookMusicFinished(_music_finished_callback);
		}
	}
};

static void _music_finished_callback(void)
{
	int err = node_sdl2::SimpleTask::Run(new TaskMusicFinished());
}

static void _music_finished_init(void)
{
	Mix_HookMusicFinished(_music_finished_callback);
}

static void _music_finished_quit(void)
{
	Mix_HookMusicFinished(NULL);
	NanDisposePersistent(s_music_finished_callback);
}

// channel finished callback

static void _channel_finished_set_callback(Handle<Function> callback);
static void _channel_finished_callback(int channel);

static Persistent<Function> s_channel_finished_callback;

static void _channel_finished_set_callback(Handle<Function> callback)
{
	NanDisposePersistent(s_channel_finished_callback);
	if (!callback->IsNull())
	{
		NanAssignPersistent(s_channel_finished_callback, callback);
	}
}

static void _channel_finished_init(void)
{
	Mix_ChannelFinished(_channel_finished_callback);
}

static void _channel_finished_quit(void)
{
	Mix_ChannelFinished(NULL);
	NanDisposePersistent(s_channel_finished_callback);
}

class TaskChannelFinished : public node_sdl2::SimpleTask
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
			Handle<Value> argv[] = { NanNew<Integer>(m_channel) };
			NanMakeCallback(NanGetCurrentContext()->Global(), NanNew<Function>(s_channel_finished_callback), countof(argv), argv);
		}
	}
};

static void _channel_finished_callback(int channel)
{
	int err = node_sdl2::SimpleTask::Run(new TaskChannelFinished(channel));
}

// load chunk

class Task_MIX_LoadWav : public node_sdl2::SimpleTask
{
public:
	Persistent<Function> m_callback;
	char* m_file;
	Mix_Chunk* m_chunk;
public:
	Task_MIX_LoadWav(Handle<String> file, Handle<Function> callback) : 
		m_file(strdup(*String::Utf8Value(file))), 
		m_chunk(NULL)
	{
		NanAssignPersistent(m_callback, callback);
	}
	~Task_MIX_LoadWav()
	{
		NanDisposePersistent(m_callback);
		free(m_file); m_file = NULL; // strdup
		if (m_chunk) { Mix_FreeChunk(m_chunk); m_chunk = NULL; }
	}
	void DoWork()
	{
		m_chunk = Mix_LoadWAV(m_file);
	}
	void DoAfterWork(int status)
	{
		NanScope();
		Handle<Value> argv[] = { WrapChunk::Hold(m_chunk) };
		NanMakeCallback(NanGetCurrentContext()->Global(), NanNew<Function>(m_callback), countof(argv), argv);
		m_chunk = NULL; // script owns pointer
	}
};

// load music

class Task_MIX_LoadMUS : public node_sdl2::SimpleTask
{
public:
	Persistent<Function> m_callback;
	char* m_file;
	Mix_Music* m_music;
public:
	Task_MIX_LoadMUS(Handle<String> file, Handle<Function> callback) : 
		m_file(strdup(*String::Utf8Value(file))), 
		m_music(NULL)
	{
		NanAssignPersistent(m_callback, callback);
	}
	~Task_MIX_LoadMUS()
	{
		NanDisposePersistent(m_callback);
		free(m_file); m_file = NULL; // strdup
		if (m_music) { Mix_FreeMusic(m_music); m_music = NULL; }
	}
	void DoWork()
	{
		m_music = Mix_LoadMUS(m_file);
	}
	void DoAfterWork(int status)
	{
		NanScope();
		Handle<Value> argv[] = { WrapMusic::Hold(m_music) };
		NanMakeCallback(NanGetCurrentContext()->Global(), NanNew<Function>(m_callback), countof(argv), argv);
		m_music = NULL; // script owns pointer
	}
};

MODULE_EXPORT_IMPLEMENT(Mix_Init)
{
	NanScope();
	::Uint32 flags = args[0]->Uint32Value();
	int err = Mix_Init(flags);
	if (err < 0)
	{
		printf("Mix_Init error: %d\n", err);
	}
	_music_finished_init();
	_channel_finished_init();
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_Quit)
{
	NanScope();
	_channel_finished_quit();
	_music_finished_quit();
	Mix_Quit();
	NanReturnUndefined();
}

MODULE_EXPORT_IMPLEMENT(Mix_GetError)
{
	NanScope();
	const char *sdl_mixer_error = Mix_GetError();
	NanReturnValue(NanNew<String>(sdl_mixer_error));
}

MODULE_EXPORT_IMPLEMENT(Mix_ClearError)
{
	NanScope();
	SDL_ClearError();
	NanReturnUndefined();
}

MODULE_EXPORT_IMPLEMENT(Mix_OpenAudio)
{
	NanScope();
	int frequency = args[0]->Int32Value();
	::Uint16 format = (::Uint16) args[1]->Uint32Value();
	int channels = args[2]->Int32Value();
	int chunksize = args[3]->Int32Value();
	int err = Mix_OpenAudio(frequency, format, channels, chunksize);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_AllocateChannels)
{
	NanScope();
	int numchans = args[0]->Int32Value();
	int err = Mix_AllocateChannels(numchans);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_QuerySpec)
{
	NanScope();
	Local<Array> _frequency = Local<Array>::Cast(args[0]);
	Local<Array> _format = Local<Array>::Cast(args[1]);
	Local<Array> _channels = Local<Array>::Cast(args[2]);
	int frequency = 0;
	::Uint16 format = 0;
	int channels = 0;
	int err = Mix_QuerySpec(&frequency, &format, &channels);
	_frequency->Set(0, NanNew<Integer>(frequency));
	_format->Set(0, NanNew<Integer>(format));
	_channels->Set(0, NanNew<Integer>(channels));
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_LoadWAV)
{
	NanScope();
	Local<String> file = Local<String>::Cast(args[0]);
	Local<Function> callback = Local<Function>::Cast(args[1]);
	int err = node_sdl2::SimpleTask::Run(new Task_MIX_LoadWav(file, callback));
	NanReturnValue(NanNew<v8::Int32>(err));
}

MODULE_EXPORT_IMPLEMENT_TODO(Mix_LoadWAV_RW)

MODULE_EXPORT_IMPLEMENT(Mix_LoadMUS)
{
	NanScope();
	Local<String> file = Local<String>::Cast(args[0]);
	Local<Function> callback = Local<Function>::Cast(args[1]);
	int err = node_sdl2::SimpleTask::Run(new Task_MIX_LoadMUS(file, callback));
	NanReturnValue(NanNew<v8::Int32>(err));
}

MODULE_EXPORT_IMPLEMENT_TODO(Mix_LoadMUS_RW)

MODULE_EXPORT_IMPLEMENT_TODO(Mix_LoadMUSType_RW)

MODULE_EXPORT_IMPLEMENT_TODO(Mix_QuickLoad_WAV)

MODULE_EXPORT_IMPLEMENT_TODO(Mix_QuickLoad_RAW)

MODULE_EXPORT_IMPLEMENT(Mix_FreeChunk)
{
	NanScope();
	Mix_Chunk* chunk = WrapChunk::Drop(args[0]);
	Mix_FreeChunk(chunk);
	NanReturnUndefined();
}

MODULE_EXPORT_IMPLEMENT(Mix_FreeMusic)
{
	NanScope();
	Mix_Music* music = WrapMusic::Drop(args[0]);
	Mix_FreeMusic(music);
	NanReturnUndefined();
}

MODULE_EXPORT_IMPLEMENT(Mix_GetNumChunkDecoders)
{
	NanScope();
	int num = Mix_GetNumChunkDecoders();
	NanReturnValue(NanNew<Integer>(num));
}

MODULE_EXPORT_IMPLEMENT(Mix_GetChunkDecoder)
{
	NanScope();
	int index = args[0]->Int32Value();
	const char* name = Mix_GetChunkDecoder(index);
	NanReturnValue(NanNew<String>(name));
}

MODULE_EXPORT_IMPLEMENT(Mix_GetNumMusicDecoders)
{
	NanScope();
	int num = Mix_GetNumMusicDecoders();
	NanReturnValue(NanNew<Integer>(num));
}

MODULE_EXPORT_IMPLEMENT(Mix_GetMusicDecoder)
{
	NanScope();
	int index = args[0]->Int32Value();
	const char* name = Mix_GetMusicDecoder(index);
	NanReturnValue(NanNew<String>(name));
}

MODULE_EXPORT_IMPLEMENT(Mix_GetMusicType)
{
	NanScope();
	Mix_Music* music = WrapMusic::Peek(args[0]);
	Mix_MusicType type = Mix_GetMusicType(music);
	NanReturnValue(NanNew<Integer>(type));
}

MODULE_EXPORT_IMPLEMENT_TODO(Mix_SetPostMix)

MODULE_EXPORT_IMPLEMENT_TODO(Mix_HookMusic)

MODULE_EXPORT_IMPLEMENT(Mix_HookMusicFinished)
{
	NanScope();
	Local<Function> callback = Local<Function>::Cast(args[0]);
	_music_finished_set_callback(callback);
	NanReturnUndefined();
}

MODULE_EXPORT_IMPLEMENT_TODO(Mix_GetMusicHookData)

MODULE_EXPORT_IMPLEMENT(Mix_ChannelFinished)
{
	NanScope();
	Local<Function> callback = Local<Function>::Cast(args[0]);
	_channel_finished_set_callback(callback);
	NanReturnUndefined();
}

MODULE_EXPORT_IMPLEMENT_TODO(Mix_RegisterEffect)

MODULE_EXPORT_IMPLEMENT_TODO(Mix_UnregisterEffect)

MODULE_EXPORT_IMPLEMENT(Mix_UnregisterAllEffects)
{
	NanScope();
	int channel = args[0]->Int32Value();
	int err = Mix_UnregisterAllEffects(channel);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_SetPanning)
{
	NanScope();
	int channel = args[0]->Int32Value();
	::Uint8 left = (::Uint8) args[1]->Uint32Value();
	::Uint8 right = (::Uint8) args[2]->Uint32Value();
	int err = Mix_SetPanning(channel, left, right);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_SetPosition)
{
	NanScope();
	int channel = args[0]->Int32Value();
	::Sint16 angle = (::Sint16) args[1]->Int32Value();
	::Uint8 distance = (::Uint8) args[2]->Uint32Value();
	int err = Mix_SetPosition(channel, angle, distance);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_SetDistance)
{
	NanScope();
	int channel = args[0]->Int32Value();
	::Uint8 distance = (::Uint8) args[1]->Uint32Value();
	int err = Mix_SetDistance(channel, distance);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_SetReverseStereo)
{
	NanScope();
	int channel = args[0]->Int32Value();
	int flip = (::Sint16) args[1]->Int32Value();
	int err = Mix_SetReverseStereo(channel, flip);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_ReserveChannels)
{
	NanScope();
	int num = args[0]->Int32Value();
	int err = Mix_ReserveChannels(num);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_GroupChannel)
{
	NanScope();
	int channel = args[0]->Int32Value();
	int tag = args[1]->Int32Value();
	int err = Mix_GroupChannel(channel, tag);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_GroupChannels)
{
	NanScope();
	int from = args[0]->Int32Value();
	int to = args[1]->Int32Value();
	int tag = args[2]->Int32Value();
	int err = Mix_GroupChannels(from, to, tag);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_GroupAvailable)
{
	NanScope();
	int tag = args[0]->Int32Value();
	int err = Mix_GroupAvailable(tag);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_GroupCount)
{
	NanScope();
	int tag = args[0]->Int32Value();
	int err = Mix_GroupCount(tag);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_GroupOldest)
{
	NanScope();
	int tag = args[0]->Int32Value();
	int err = Mix_GroupOldest(tag);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_GroupNewer)
{
	NanScope();
	int tag = args[0]->Int32Value();
	int err = Mix_GroupNewer(tag);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_PlayChannel)
{
	NanScope();
	int channel = args[0]->Int32Value();
	Mix_Chunk* chunk = WrapChunk::Peek(args[1]);
	int loops = args[2]->Int32Value();
	int err = Mix_PlayChannel(channel, chunk, loops);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_PlayChannelTimed)
{
	NanScope();
	int channel = args[0]->Int32Value();
	Mix_Chunk* chunk = WrapChunk::Peek(args[1]);
	int loops = args[2]->Int32Value();
	int ticks = args[3]->Int32Value();
	int err = Mix_PlayChannelTimed(channel, chunk, loops, ticks);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_PlayMusic)
{
	NanScope();
	Mix_Music* music = WrapMusic::Peek(args[0]);
	int loops = args[1]->Int32Value();
	int err = Mix_PlayMusic(music, loops);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_FadeInMusic)
{
	NanScope();
	Mix_Music* music = WrapMusic::Peek(args[0]);
	int loops = args[1]->Int32Value();
	int ms = args[2]->Int32Value();
	int err = Mix_FadeInMusic(music, loops, ms);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_FadeInMusicPos)
{
	NanScope();
	Mix_Music* music = WrapMusic::Peek(args[0]);
	int loops = args[1]->Int32Value();
	int ms = args[2]->Int32Value();
	double position = args[3]->NumberValue();
	int err = Mix_FadeInMusicPos(music, loops, ms, position);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_FadeInChannel)
{
	NanScope();
	int channel = args[0]->Int32Value();
	Mix_Chunk* chunk = WrapChunk::Peek(args[1]);
	int loops = args[2]->Int32Value();
	int ms = args[3]->Int32Value();
	int err = Mix_FadeInChannel(channel, chunk, loops, ms);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_FadeInChannelTimed)
{
	NanScope();
	int channel = args[0]->Int32Value();
	Mix_Chunk* chunk = WrapChunk::Peek(args[1]);
	int loops = args[2]->Int32Value();
	int ms = args[3]->Int32Value();
	int ticks = args[4]->Int32Value();
	int err = Mix_FadeInChannelTimed(channel, chunk, loops, ms, ticks);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_Volume)
{
	NanScope();
	int channel = args[0]->Int32Value();
	int volume = args[1]->Int32Value();
	int err = Mix_Volume(channel, volume);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_VolumeChunk)
{
	NanScope();
	Mix_Chunk* chunk = WrapChunk::Peek(args[0]);
	int volume = args[1]->Int32Value();
	int err = Mix_VolumeChunk(chunk, volume);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_VolumeMusic)
{
	NanScope();
	int volume = args[0]->Int32Value();
	int err = Mix_VolumeMusic(volume);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_HaltChannel)
{
	NanScope();
	int channel = args[0]->Int32Value();
	int err = Mix_HaltChannel(channel);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_HaltGroup)
{
	NanScope();
	int tag = args[0]->Int32Value();
	int err = Mix_HaltGroup(tag);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_HaltMusic)
{
	NanScope();
	int err = Mix_HaltMusic();
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_ExpireChannel)
{
	NanScope();
	int channel = args[0]->Int32Value();
	int ticks = args[1]->Int32Value();
	int err = Mix_ExpireChannel(channel, ticks);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_FadeOutChannel)
{
	NanScope();
	int channel = args[0]->Int32Value();
	int ms = args[1]->Int32Value();
	int err = Mix_FadeOutChannel(channel, ms);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_FadeOutGroup)
{
	NanScope();
	int tag = args[0]->Int32Value();
	int ms = args[1]->Int32Value();
	int err = Mix_FadeOutGroup(tag, ms);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_FadeOutMusic)
{
	NanScope();
	int ms = args[0]->Int32Value();
	int err = Mix_FadeOutMusic(ms);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_FadingMusic)
{
	NanScope();
	Mix_Fading fading = Mix_FadingMusic();
	NanReturnValue(NanNew<Integer>(fading));
}

MODULE_EXPORT_IMPLEMENT(Mix_FadingChannel)
{
	NanScope();
	int channel = args[0]->Int32Value();
	Mix_Fading fading = Mix_FadingChannel(channel);
	NanReturnValue(NanNew<Integer>(fading));
}

MODULE_EXPORT_IMPLEMENT(Mix_Pause)
{
	NanScope();
	int channel = args[0]->Int32Value();
	Mix_Pause(channel);
	NanReturnUndefined();
}

MODULE_EXPORT_IMPLEMENT(Mix_Resume)
{
	NanScope();
	int channel = args[0]->Int32Value();
	Mix_Resume(channel);
	NanReturnUndefined();
}

MODULE_EXPORT_IMPLEMENT(Mix_Paused)
{
	NanScope();
	int channel = args[0]->Int32Value();
	int ret = Mix_Paused(channel);
	NanReturnValue(NanNew<Integer>(ret));
}

MODULE_EXPORT_IMPLEMENT(Mix_PauseMusic)
{
	NanScope();
	Mix_PauseMusic();
	NanReturnUndefined();
}

MODULE_EXPORT_IMPLEMENT(Mix_ResumeMusic)
{
	NanScope();
	Mix_ResumeMusic();
	NanReturnUndefined();
}

MODULE_EXPORT_IMPLEMENT(Mix_RewindMusic)
{
	NanScope();
	Mix_RewindMusic();
	NanReturnUndefined();
}

MODULE_EXPORT_IMPLEMENT(Mix_PausedMusic)
{
	NanScope();
	int ret = Mix_PausedMusic();
	NanReturnValue(NanNew<Integer>(ret));
}

MODULE_EXPORT_IMPLEMENT(Mix_SetMusicPosition)
{
	NanScope();
	double position = args[0]->NumberValue();
	int err = Mix_SetMusicPosition(position);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_Playing)
{
	NanScope();
	int channel = args[0]->Int32Value();
	int ret = Mix_Playing(channel);
	NanReturnValue(NanNew<Integer>(ret));
}

MODULE_EXPORT_IMPLEMENT(Mix_PlayingMusic)
{
	NanScope();
	int ret = Mix_PlayingMusic();
	NanReturnValue(NanNew<Integer>(ret));
}

MODULE_EXPORT_IMPLEMENT(Mix_SetMusicCMD)
{
	NanScope();
	Local<String> command = Local<String>::Cast(args[0]);
	int err = Mix_SetMusicCMD(*String::Utf8Value(command));
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_SetSynchroValue)
{
	NanScope();
	int value = args[0]->Int32Value();
	int err = Mix_SetSynchroValue(value);
	NanReturnValue(NanNew<Integer>(err));
}

MODULE_EXPORT_IMPLEMENT(Mix_GetSynchroValue)
{
	NanScope();
	int ret = Mix_GetSynchroValue();
	NanReturnValue(NanNew<Integer>(ret));
}

MODULE_EXPORT_IMPLEMENT(Mix_SetSoundFonts)
{
	#ifdef MID_MUSIC
	NanScope();
	Local<String> fonts = Local<String>::Cast(args[0]);
	int err = Mix_SetSoundFonts(*String::Utf8Value(fonts));
	NanReturnValue(NanNew<Integer>(err));
	#else
	NanScope();
	NanReturnValue(NanNew<Integer>(-1));
	#endif
}

MODULE_EXPORT_IMPLEMENT(Mix_GetSoundFonts)
{
	#ifdef MID_MUSIC
	NanScope();
	const char* fonts = Mix_GetSoundFonts();
	NanReturnValue(NanNew<String>(fonts));
	#else
	NanScope();
	NanReturnNull();
	#endif
}

#ifdef MID_MUSIC
static Handle<Function> s_each_sound_font_callback;
static Handle<Value> s_each_sound_font_data;
static int _each_sound_font(const char* font, void* data)
{
	Handle<Value> argv[] = { NanNew<String>(font), s_each_sound_font_data };
	Handle<Value> res = s_each_sound_font_callback->Call(Context::GetCurrent()->Global(), countof(argv), argv);
	return res->Int32Value();
}
#endif

MODULE_EXPORT_IMPLEMENT(Mix_EachSoundFont)
{
	#ifdef MID_MUSIC
	NanScope();
	s_each_sound_font_callback = Handle<Function>::Cast(args[0]);
	s_each_sound_font_data = args[1];
	int err = Mix_EachSoundFont(_each_sound_font);
	s_each_sound_font_callback.Clear();
	s_each_sound_font_data.Clear();;
	NanReturnValue(NanNew<Integer>(err));
	#else
	NanScope();
	NanReturnValue(NanNew<Integer>(-1));
	#endif
}

MODULE_EXPORT_IMPLEMENT(Mix_GetChunk)
{
	NanScope();
	int channel = args[0]->Int32Value();
	Mix_Chunk* chunk = Mix_GetChunk(channel);
	NanReturnValue(WrapChunk::Hold(chunk));
}

MODULE_EXPORT_IMPLEMENT(Mix_CloseAudio)
{
	NanScope();
	Mix_CloseAudio();
	NanReturnUndefined();
}

#if NODE_VERSION_AT_LEAST(0,11,0)
void init(Handle<Object> exports, Handle<Value> module, Handle<Context> context)
#else
void init(Handle<Object> exports/*, Handle<Value> module*/)
#endif
{
	NanScope();

	// SDL_mixer.h

	MODULE_CONSTANT(exports, SDL_MIXER_MAJOR_VERSION);
	MODULE_CONSTANT(exports, SDL_MIXER_MINOR_VERSION);
	MODULE_CONSTANT(exports, SDL_MIXER_PATCHLEVEL);

	MODULE_CONSTANT(exports, MIX_INIT_FLAC);
	MODULE_CONSTANT(exports, MIX_INIT_MOD);
	MODULE_CONSTANT(exports, MIX_INIT_MODPLUG);
	MODULE_CONSTANT(exports, MIX_INIT_MP3);
	MODULE_CONSTANT(exports, MIX_INIT_OGG);
	MODULE_CONSTANT(exports, MIX_INIT_FLUIDSYNTH);

	MODULE_CONSTANT(exports, MIX_CHANNELS);
	MODULE_CONSTANT(exports, MIX_DEFAULT_FREQUENCY);
	MODULE_CONSTANT(exports, MIX_DEFAULT_FORMAT);
	MODULE_CONSTANT(exports, MIX_DEFAULT_CHANNELS);
	MODULE_CONSTANT(exports, MIX_MAX_VOLUME);

	// Mix_Fading
	Local<Object> Fading = NanNew<Object>();
	exports->Set(NanNew<String>("Mix_Fading"), Fading);
	MODULE_CONSTANT(Fading, MIX_NO_FADING);
	MODULE_CONSTANT(Fading, MIX_FADING_OUT);
	MODULE_CONSTANT(Fading, MIX_FADING_IN);

	// Mix_MusicType
	Local<Object> MusicType = NanNew<Object>();
	exports->Set(NanNew<String>("Mix_MusicType"), MusicType);
	MODULE_CONSTANT(MusicType, MUS_NONE);
	MODULE_CONSTANT(MusicType, MUS_CMD);
	MODULE_CONSTANT(MusicType, MUS_WAV);
	MODULE_CONSTANT(MusicType, MUS_MOD);
	MODULE_CONSTANT(MusicType, MUS_MID);
	MODULE_CONSTANT(MusicType, MUS_OGG);
	MODULE_CONSTANT(MusicType, MUS_MP3);
	MODULE_CONSTANT(MusicType, MUS_MP3_MAD);
	MODULE_CONSTANT(MusicType, MUS_FLAC);
	MODULE_CONSTANT(MusicType, MUS_MODPLUG);

	MODULE_CONSTANT_STRING(exports, MIX_EFFECTSMAXSPEED);

	MODULE_EXPORT_APPLY(exports, Mix_Init);
	MODULE_EXPORT_APPLY(exports, Mix_Quit);
	MODULE_EXPORT_APPLY(exports, Mix_GetError);
	MODULE_EXPORT_APPLY(exports, Mix_ClearError);
	MODULE_EXPORT_APPLY(exports, Mix_OpenAudio);
	MODULE_EXPORT_APPLY(exports, Mix_AllocateChannels);
	MODULE_EXPORT_APPLY(exports, Mix_QuerySpec);
	MODULE_EXPORT_APPLY(exports, Mix_LoadWAV);
	MODULE_EXPORT_APPLY(exports, Mix_LoadWAV_RW);
	MODULE_EXPORT_APPLY(exports, Mix_LoadMUS);
	MODULE_EXPORT_APPLY(exports, Mix_LoadMUS_RW);
	MODULE_EXPORT_APPLY(exports, Mix_LoadMUSType_RW);
	MODULE_EXPORT_APPLY(exports, Mix_QuickLoad_WAV);
	MODULE_EXPORT_APPLY(exports, Mix_QuickLoad_RAW);
	MODULE_EXPORT_APPLY(exports, Mix_FreeChunk);
	MODULE_EXPORT_APPLY(exports, Mix_FreeMusic);
	MODULE_EXPORT_APPLY(exports, Mix_GetNumChunkDecoders);
	MODULE_EXPORT_APPLY(exports, Mix_GetChunkDecoder);
	MODULE_EXPORT_APPLY(exports, Mix_GetNumMusicDecoders);
	MODULE_EXPORT_APPLY(exports, Mix_GetMusicDecoder);
	MODULE_EXPORT_APPLY(exports, Mix_GetMusicType);
	MODULE_EXPORT_APPLY(exports, Mix_SetPostMix);
	MODULE_EXPORT_APPLY(exports, Mix_HookMusic);
	MODULE_EXPORT_APPLY(exports, Mix_HookMusicFinished);
	MODULE_EXPORT_APPLY(exports, Mix_GetMusicHookData);
	MODULE_EXPORT_APPLY(exports, Mix_ChannelFinished);
	MODULE_EXPORT_APPLY(exports, Mix_RegisterEffect);
	MODULE_EXPORT_APPLY(exports, Mix_UnregisterEffect);
	MODULE_EXPORT_APPLY(exports, Mix_UnregisterAllEffects);
	MODULE_EXPORT_APPLY(exports, Mix_SetPanning);
	MODULE_EXPORT_APPLY(exports, Mix_SetPosition);
	MODULE_EXPORT_APPLY(exports, Mix_SetDistance);
	MODULE_EXPORT_APPLY(exports, Mix_SetReverseStereo);
	MODULE_EXPORT_APPLY(exports, Mix_ReserveChannels);
	MODULE_EXPORT_APPLY(exports, Mix_GroupChannel);
	MODULE_EXPORT_APPLY(exports, Mix_GroupChannels);
	MODULE_EXPORT_APPLY(exports, Mix_GroupAvailable);
	MODULE_EXPORT_APPLY(exports, Mix_GroupCount);
	MODULE_EXPORT_APPLY(exports, Mix_GroupOldest);
	MODULE_EXPORT_APPLY(exports, Mix_GroupNewer);
	MODULE_EXPORT_APPLY(exports, Mix_PlayChannel);
	MODULE_EXPORT_APPLY(exports, Mix_PlayChannelTimed);
	MODULE_EXPORT_APPLY(exports, Mix_PlayMusic);
	MODULE_EXPORT_APPLY(exports, Mix_FadeInMusic);
	MODULE_EXPORT_APPLY(exports, Mix_FadeInMusicPos);
	MODULE_EXPORT_APPLY(exports, Mix_FadeInChannel);
	MODULE_EXPORT_APPLY(exports, Mix_FadeInChannelTimed);
	MODULE_EXPORT_APPLY(exports, Mix_Volume);
	MODULE_EXPORT_APPLY(exports, Mix_VolumeChunk);
	MODULE_EXPORT_APPLY(exports, Mix_VolumeMusic);
	MODULE_EXPORT_APPLY(exports, Mix_HaltChannel);
	MODULE_EXPORT_APPLY(exports, Mix_HaltGroup);
	MODULE_EXPORT_APPLY(exports, Mix_HaltMusic);
	MODULE_EXPORT_APPLY(exports, Mix_ExpireChannel);
	MODULE_EXPORT_APPLY(exports, Mix_FadeOutChannel);
	MODULE_EXPORT_APPLY(exports, Mix_FadeOutGroup);
	MODULE_EXPORT_APPLY(exports, Mix_FadeOutMusic);
	MODULE_EXPORT_APPLY(exports, Mix_FadingMusic);
	MODULE_EXPORT_APPLY(exports, Mix_FadingChannel);
	MODULE_EXPORT_APPLY(exports, Mix_Pause);
	MODULE_EXPORT_APPLY(exports, Mix_Resume);
	MODULE_EXPORT_APPLY(exports, Mix_Paused);
	MODULE_EXPORT_APPLY(exports, Mix_PauseMusic);
	MODULE_EXPORT_APPLY(exports, Mix_ResumeMusic);
	MODULE_EXPORT_APPLY(exports, Mix_RewindMusic);
	MODULE_EXPORT_APPLY(exports, Mix_PausedMusic);
	MODULE_EXPORT_APPLY(exports, Mix_SetMusicPosition);
	MODULE_EXPORT_APPLY(exports, Mix_Playing);
	MODULE_EXPORT_APPLY(exports, Mix_PlayingMusic);
	MODULE_EXPORT_APPLY(exports, Mix_SetMusicCMD);
	MODULE_EXPORT_APPLY(exports, Mix_SetSynchroValue);
	MODULE_EXPORT_APPLY(exports, Mix_GetSynchroValue);
	MODULE_EXPORT_APPLY(exports, Mix_SetSoundFonts);
	MODULE_EXPORT_APPLY(exports, Mix_GetSoundFonts);
	MODULE_EXPORT_APPLY(exports, Mix_EachSoundFont);
	MODULE_EXPORT_APPLY(exports, Mix_GetChunk);
	MODULE_EXPORT_APPLY(exports, Mix_CloseAudio);
}

} // namespace node_sdl2_mixer

#if NODE_VERSION_AT_LEAST(0,11,0)
NODE_MODULE_CONTEXT_AWARE_BUILTIN(node_sdl2_mixer, node_sdl2_mixer::init)
#else
NODE_MODULE(node_sdl2_mixer, node_sdl2_mixer::init)
#endif

