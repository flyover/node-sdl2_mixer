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

#ifndef _NODE_SDL2_MIXER_H_
#define _NODE_SDL2_MIXER_H_

#include <nan.h>

#include <SDL.h>
#include <SDL_mixer.h>

#include "node-sdl2.h"

namespace node_sdl2_mixer {

// wrap Mix_Music pointer

class WrapMusic : public Nan::ObjectWrap
{
private:
	Mix_Music* m_music;
public:
	WrapMusic(Mix_Music* music) : m_music(music) {}
	~WrapMusic() { Free(m_music); m_music = NULL; }
public:
	Mix_Music* Peek() { return m_music; }
	Mix_Music* Drop() { Mix_Music* music = m_music; m_music = NULL; return music; }
public:
	static WrapMusic* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapMusic* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapMusic>(object); }
	static Mix_Music* Peek(v8::Local<v8::Value> value) { WrapMusic* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static v8::Local<v8::Value> Hold(Mix_Music* music) { return NewInstance(music); }
	static Mix_Music* Drop(v8::Local<v8::Value> value) { WrapMusic* wrap = Unwrap(value); return (wrap)?(wrap->Drop()):(NULL); }
	static void Free(Mix_Music* music)
	{
		if (music) { Mix_FreeMusic(music); music = NULL; }
	}
public:
	static v8::Local<v8::Object> NewInstance(Mix_Music* music)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::ObjectTemplate> object_template = GetObjectTemplate();
		v8::Local<v8::Object> instance = object_template->NewInstance();
		WrapMusic* wrap = new WrapMusic(music);
		wrap->Wrap(instance);
		wrap->MakeWeak(); // TODO: is this necessary?
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::ObjectTemplate> GetObjectTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::ObjectTemplate> g_object_template;
		if (g_object_template.IsEmpty())
		{
			v8::Local<v8::ObjectTemplate> object_template = Nan::New<v8::ObjectTemplate>();
			g_object_template.Reset(object_template);
			object_template->SetInternalFieldCount(1);
		}
		v8::Local<v8::ObjectTemplate> object_template = Nan::New<v8::ObjectTemplate>(g_object_template);
		return scope.Escape(object_template);
	}
};

// wrap Mix_Chunk pointer

class WrapChunk : public Nan::ObjectWrap
{
private:
	Mix_Chunk* m_chunk;
public:
	WrapChunk(Mix_Chunk* chunk) : m_chunk(chunk) {}
	~WrapChunk() { Free(m_chunk); m_chunk = NULL; }
public:
	Mix_Chunk* Peek() { return m_chunk; }
	Mix_Chunk* Drop() { Mix_Chunk* chunk = m_chunk; m_chunk = NULL; return chunk; }
public:
	static WrapChunk* Unwrap(v8::Local<v8::Value> value) { return (value->IsObject())?(Unwrap(v8::Local<v8::Object>::Cast(value))):(NULL); }
	static WrapChunk* Unwrap(v8::Local<v8::Object> object) { return Nan::ObjectWrap::Unwrap<WrapChunk>(object); }
	static Mix_Chunk* Peek(v8::Local<v8::Value> value) { WrapChunk* wrap = Unwrap(value); return (wrap)?(wrap->Peek()):(NULL); }
public:
	static v8::Local<v8::Value> Hold(Mix_Chunk* chunk) { return NewInstance(chunk); }
	static Mix_Chunk* Drop(v8::Local<v8::Value> value) { WrapChunk* wrap = Unwrap(value); return (wrap)?(wrap->Drop()):(NULL); }
	static void Free(Mix_Chunk* chunk)
	{
		if (chunk) { Mix_FreeChunk(chunk); chunk = NULL; }
	}
public:
	static v8::Local<v8::Object> NewInstance(Mix_Chunk* chunk)
	{
		Nan::EscapableHandleScope scope;
		v8::Local<v8::ObjectTemplate> object_template = GetObjectTemplate();
		v8::Local<v8::Object> instance = object_template->NewInstance();
		WrapChunk* wrap = new WrapChunk(chunk);
		wrap->Wrap(instance);
		wrap->MakeWeak(); // TODO: is this necessary?
		return scope.Escape(instance);
	}
private:
	static v8::Local<v8::ObjectTemplate> GetObjectTemplate()
	{
		Nan::EscapableHandleScope scope;
		static Nan::Persistent<v8::ObjectTemplate> g_object_template;
		if (g_object_template.IsEmpty())
		{
			v8::Local<v8::ObjectTemplate> object_template = Nan::New<v8::ObjectTemplate>();
			g_object_template.Reset(object_template);
			object_template->SetInternalFieldCount(1);
		}
		v8::Local<v8::ObjectTemplate> object_template = Nan::New<v8::ObjectTemplate>(g_object_template);
		return scope.Escape(object_template);
	}
};

NAN_MODULE_INIT(init);

} // namespace node_sdl2_mixer

#endif // _NODE_SDL2_MIXER_H_
