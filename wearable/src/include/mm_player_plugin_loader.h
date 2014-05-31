/*
 * libmm-player
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: JongHyuk Choi <jhchoi.choi@samsung.com>, YeJin Cho <cho.yejin@samsung.com>,
 * Seungbae Shin <seungbae.shin@samsung.com>, YoungHwan An <younghwan_.an@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __MM_PLAYER_LOAD_PLUGINS_H__
#define __MM_PLAYER_LOAD_PLUGINS_H__

/*=======================================================================================
| INCLUDE FILES										|
========================================================================================*/
//#include <mm_types.h>
#include "mm_player_priv.h"

#ifdef __cplusplus
	extern "C" {
#endif

/*=======================================================================================
| GLOBAL FUNCTION PROTOTYPES								|
========================================================================================*/

#ifdef _LITEW_OPT_
#define _PLUGIN_PATH	 			"/usr/lib/gstreamer-0.10/"
#define _GST_APPSRC_PATH 			_PLUGIN_PATH"libgstapp.so"
#define _GST_AUDIO_ID3DEMUX		_PLUGIN_PATH"libgstid3demux.so"
#define _GST_AUDIO_MP3PARSER		_PLUGIN_PATH"libgstaudioparsers.so"
#define _GST_AUDIO_MP3CODEC 		_PLUGIN_PATH"libgstsavsdecmp3.so"
#define _GST_AUDIO_AACCODEC		_PLUGIN_PATH"libgstsavsdecaac.so"
#define _GST_AUDIO_AMRNBCODEC 	_PLUGIN_PATH"libgstsavsdecamrnb.so"
#define _GST_AUDIO_AMRWBCODEC 	_PLUGIN_PATH"libgstsavsdecamrwb.so"
#define _GST_AUDIO_SOUNDBOOSTER 	_PLUGIN_PATH"libgstsoundalive.so"
#define _GST_AUDIO_VORBIS			_PLUGIN_PATH"libgstvorbis.so"
#define _GST_AUDIO_WAVPARSER		_PLUGIN_PATH"libgstwavparse.so"
#define _GST_AUDIO_SINK			_PLUGIN_PATH"libgstavsyssink.so"
#define _GST_AUDIO_CONVERTER		_PLUGIN_PATH"libgstaudioconvert.so"
#define _GST_AUDIO_RESAMPLER		_PLUGIN_PATH"libgstaudioresample.so"
#define _GST_AUDIO_FILTER			_PLUGIN_PATH"libgstsoundalive.so"
#define _GST_OGGDEMUXER			_PLUGIN_PATH"libgstogg.so"
#define _GST_3GPDEMUXER			_PLUGIN_PATH"libgstisomp4.so"
#define _GST_VIDEO_SINK			_PLUGIN_PATH"libgstxvimagesink.so"
#define _GST_VIDEO_CODEC			_PLUGIN_PATH"libgstomx.so"

#define _GST_CORE_TYPEFINDFUNC	_PLUGIN_PATH"libgsttypefindfunctions.so"
#define _GST_CORE_ELEMENTS		_PLUGIN_PATH"libgstcoreelements.so"
#define _GST_CORE_AUDIOPARSERS	_PLUGIN_PATH"libgstaudioparsers.so"
#define _GST_CORE_INDEXERS			_PLUGIN_PATH"libgstcoreindexers.so"

typedef enum {
	_AUDIO_MP3 = 0,
	_AUDIO_WAV,
	_AUDIO_AMR,
	_AUDIO_AAC,
	_AUDIO_EFFECT,
	_MEDIA_OGG,
	_MEDIA_3GP,
	_DEFAULT,
}MMPlayerMediaType_e;

/* if needed to load and update, it returns TRUE. */
bool __mmplayer_post_load_plugins(mm_player_t *player, const gchar *mime);
#endif

#ifdef __cplusplus
	}
#endif

#endif

