/*
 * libmm-player
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: JongHyuk Choi <jhchoi.choi@samsung.com>, naveen cherukuri <naveen.ch@samsung.com>,
 * YeJin Cho <cho.yejin@samsung.com>, YoungHwan An <younghwan_.an@samsung.com>
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
 #ifdef _LITEW_OPT_
#include <mm_debug.h>
#include <mm_error.h>
#include "mm_player_plugin_loader.h"
#include "mm_player_utils.h"
#include "mm_player_priv.h"

/*---------------------------------------------------------------------------------------
|    LOCAL FUNCTION PROTOTYPES:								                                                        |
---------------------------------------------------------------------------------------*/
static GList* __mmplayer_get_list_to_load(MMPlayerMediaType_e type);

/*=======================================================================================
|  FUNCTION DEFINITIONS									                                                                                      |
=======================================================================================*/

static GList* __mmplayer_get_list_to_load(MMPlayerMediaType_e type)
{
	GList *loading_list = NULL;
	#define _MAKE_LOADING_LIST(x_list, x_plugins)	\
		int items = 0; \
		int i = 0; \
		items = ARRAY_SIZE(x_plugins); \
		for (i = 0; i < items; i++) { \
			x_list = g_list_append(x_list, g_strdup(x_plugins[i])); \
		}

	if (type == _AUDIO_MP3) {
		const char* plugins[] =
		{
			_GST_AUDIO_ID3DEMUX,
			_GST_AUDIO_MP3PARSER,
			_GST_AUDIO_MP3CODEC
		};

		_MAKE_LOADING_LIST(loading_list, plugins);
	} else if (type == _MEDIA_OGG) {
		const char* plugins[] =
		{
			_GST_AUDIO_VORBIS,
			_GST_OGGDEMUXER
		};

		_MAKE_LOADING_LIST(loading_list, plugins);
	} else if (type == _AUDIO_WAV) {
		const char* plugins[] =
		{
			_GST_AUDIO_WAVPARSER
		};

		_MAKE_LOADING_LIST(loading_list, plugins);
	} else if (type == _MEDIA_3GP) {
		const char* plugins[] =
		{
			_GST_3GPDEMUXER,
			_GST_VIDEO_SINK,
			_GST_VIDEO_CODEC
		};

		_MAKE_LOADING_LIST(loading_list, plugins);
	} else if (type == _AUDIO_AMR) {
		const char* plugins[] =
		{
			_GST_AUDIO_AMRNBCODEC,
			_GST_AUDIO_AMRWBCODEC
		};

		_MAKE_LOADING_LIST(loading_list, plugins);
	} else if (type == _DEFAULT) {
		const char* plugins[] =
		{
			_GST_CORE_TYPEFINDFUNC,
			_GST_CORE_ELEMENTS,
			_GST_CORE_AUDIOPARSERS,
			_GST_CORE_INDEXERS,
			_GST_AUDIO_RESAMPLER,
			_GST_AUDIO_CONVERTER,
			_GST_AUDIO_SINK
		};

		_MAKE_LOADING_LIST(loading_list, plugins);
	} else if (type == _AUDIO_AAC) {
		const char* plugins[] =
		{
			_GST_AUDIO_AACCODEC
		};

		_MAKE_LOADING_LIST(loading_list, plugins);
	} else if (type == _AUDIO_EFFECT) {
		const char* plugins[] =
		{
			_GST_AUDIO_FILTER
		};

		_MAKE_LOADING_LIST(loading_list, plugins);
	}

	return (loading_list) ? loading_list : NULL;
}

bool __mmplayer_post_load_plugins(mm_player_t *player, const gchar *caps_string)
{
	GList *list = NULL;
	GList *tmp = NULL;
	GstPlugin *newplugin = NULL;
	MMPlayerMediaType_e type;

	if ((player->loaded == FALSE) && ((g_strrstr(caps_string, "application/x-id3")) ||
		(g_strrstr(caps_string, "audio/mpeg") &&
		g_strrstr(caps_string, "mpegversion=(int)1")))) {
		type = _AUDIO_MP3;
		player->loaded = TRUE;
	}else if (g_strrstr(caps_string, "audio/mpeg")) {
		type = _AUDIO_AAC;
	}else if (g_strrstr(caps_string, "application/ogg")) {
		type = _MEDIA_OGG;
	}else if (g_strrstr(caps_string, "audio/x-wav")) {
		type = _AUDIO_WAV;
	}else if (g_strrstr(caps_string, "application/x-3gp") ||
			g_strrstr(caps_string, "video/quicktime") ||
			g_strrstr(caps_string, "video/mj2") ||
			g_strrstr(caps_string, "audio/x-m4a")) {
		type = _MEDIA_3GP;
	}else if (g_strrstr(caps_string, "audio/AMR")) {
		type = _AUDIO_AMR;
	}else if (g_strrstr(caps_string, "audio/effect")) {
		type = _AUDIO_EFFECT;
	}else if (g_strrstr(caps_string, "default")) {
		type = _DEFAULT;
	}else {
		debug_log("no need to load plugin more");
		return FALSE;
	}

	tmp = list = __mmplayer_get_list_to_load(type);

	for ( ; tmp; tmp = g_list_next(tmp)) {
		debug_log("loading : %s", tmp->data);
		newplugin = gst_plugin_load_file(tmp->data, NULL);

		if (newplugin) {
			gst_default_registry_add_plugin(newplugin);
			gst_object_unref (newplugin);
		} else {
			debug_log("failed to load %s", tmp->data);
		}
	}

	MMPLAYER_FREEIF_GLIST(list);
	return TRUE;
}
#endif
