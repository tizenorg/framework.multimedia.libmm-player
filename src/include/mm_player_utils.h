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

#ifndef __MMF_PLAYER_UTILS_H__
#define __MMF_PLAYER_UTILS_H__

#include <glib.h>
#include <gst/gst.h>
#include <mm_player_ini.h>
#include <mm_types.h>
#include <mm_error.h>
#include <mm_message.h>

#ifdef __cplusplus
	extern "C" {
#endif

/* general */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr)		(sizeof(arr) / sizeof((arr)[0]))
#endif

#define MMPLAYER_MAX_INT	(2147483647)

#define MMPLAYER_FREEIF(x) \
if ( x ) \
	g_free( x ); \
x = NULL;

#define MMPLAYER_CMD_LOCK(x_player) \
do \
{ \
	GMutex* cmd_lock = ((mm_player_t *)x_player)->cmd_lock; \
	if (cmd_lock) \
		g_mutex_lock(cmd_lock); \
	else \
	{ \
		debug_log("don't get command lock"); \
		return MM_ERROR_PLAYER_NOT_INITIALIZED;	\
	} \
} while (0);

#define MMPLAYER_CMD_UNLOCK(x_player)	g_mutex_unlock( ((mm_player_t*)x_player)->cmd_lock )

#define MMPLAYER_MSG_POST_LOCK(x_player)	g_mutex_lock( ((mm_player_t*)x_player)->msg_cb_lock )
#define MMPLAYER_MSG_POST_UNLOCK(x_player)	g_mutex_unlock( ((mm_player_t*)x_player)->msg_cb_lock )

#define MMPLAYER_GET_ATTRS(x_player)		((mm_player_t*)x_player)->attrs

/* sbs : for bluetooth */
#define MAX_SOUND_DEVICE_LEN	18	

/* element linking */
#ifdef GST_EXT_PAD_LINK_UNCHECKED
#define GST_ELEMENT_LINK_FILTERED 	gst_element_link_filtered_unchecked
#define GST_ELEMENT_LINK_MANY 		gst_element_link_many_unchecked
#define GST_ELEMENT_LINK 			gst_element_link_unchecked
#define GST_ELEMENT_LINK_PADS 		gst_element_link_pads_unchecked
#define GST_PAD_LINK 				gst_pad_link_unchecked
#else
#define GST_ELEMENT_LINK_FILTERED 	gst_element_link_filtered
#define GST_ELEMENT_LINK_MANY 		gst_element_link_many
#define GST_ELEMENT_LINK 			gst_element_link
#define GST_ELEMENT_LINK_PADS 		gst_element_link_pads
#define GST_PAD_LINK 				gst_pad_link
#endif

/* debug caps string */
#define MMPLAYER_LOG_GST_CAPS_TYPE(x_caps) \
do \
{ \
	gchar* caps_type = NULL; \
	caps_type = gst_caps_to_string(x_caps); \
	debug_log ("caps: %s\n", caps_type ); \
	MMPLAYER_FREEIF (caps_type) \
} while (0);

/* message posting */
#define MMPLAYER_POST_MSG( x_player, x_msgtype, x_msg_param ) \
debug_log("posting %s to application\n", #x_msgtype); \
__mmplayer_post_message(x_player, x_msgtype, x_msg_param);

/* setting player state */
#define MMPLAYER_SET_STATE( x_player, x_state ) \
debug_log("setting player state to %d\n", x_state); \
__mmplayer_set_state(x_player, x_state);


#define MMPLAYER_CHECK_STATE_RETURN_IF_FAIL( x_player, x_command ) \
debug_log("checking player state before doing %s\n", #x_command); \
switch ( __mmplayer_check_state(x_player, x_command) ) \
{ \
	case MM_ERROR_PLAYER_INVALID_STATE: \
		return MM_ERROR_PLAYER_INVALID_STATE; \
	break; \
	/* NOTE : for robustness of player. we won't treat it as an error */ \
	case MM_ERROR_PLAYER_NO_OP: \
		return MM_ERROR_NONE; \
	break; \
	default: \
	break; \
}

/* setting element state */ 
#define MMPLAYER_ELEMENT_SET_STATE( x_element, x_state ) \
debug_log("setting state [%s:%d] to [%s]\n", #x_state, x_state, GST_ELEMENT_NAME( x_element ) ); \
if ( GST_STATE_CHANGE_FAILURE == gst_element_set_state ( x_element, x_state) ) \
{ \
	debug_error("failed to set state %s to %s\n", #x_state, GST_ELEMENT_NAME( x_element )); \
	goto STATE_CHANGE_FAILED; \
}

#define MMPLAYER_CHECK_NULL( x_var ) \
if ( ! x_var ) \
{ \
	debug_error("[%s] is NULL\n", #x_var ); \
	goto ERROR; \
}

#define MMPLAYER_CHECK_CMD_IF_EXIT( x_player ) \
if ( x_player->cmd == MMPLAYER_COMMAND_UNREALIZE || x_player->cmd == MMPLAYER_COMMAND_DESTROY ) \
{ \
	debug_log("it's exit state...\n");\
	goto ERROR;  \
}
/* volume */
/* 
|----|-------|-------|-------|-------|
|Res. | HFK(7)  |  BT(7)  |  E.J(7)  | SPK(7) |
|----|-------|-------|-------|-------|
*/

/* 090424 Fix me : Currently volume is 0~9, so bt volume can be only 0.0 ~ 0.9 */
#define GET_VOLUME_BT(volume) (volume/10.)

#if 0
#define GET_VOLUME_SPK(volume) ((volume) & 0x7F)
#define GET_VOLUME_EARJACK(volume) ((volume >> 7)& 0x7F)
#define GET_VOLUME_HFK(volume) ((volume >> 21) & 0x7F)

#define SET_VOLUME_SPK(volume,input) (volume |= (input &0x7F))
#define SET_VOLUME_EARJACK(volume,input) (volume |= ((input & 0x7F)<<7))
#define SET_VOLUME_BT(volume,input) (volume |= ((input & 0x7F)<<14))
#define SET_VOLUME_HFK(volume,input) (volume |= ((input & 0x7F)<<21))
#endif


/* pad probe for pipeilne debugging */
gboolean __util_gst_pad_probe(GstPad *pad, GstBuffer *buffer, gpointer u_data);

#define MM_PROBE_DEFAULT			(0)
#define MM_PROBE_TIMESTAMP			(1)
#define MM_PROBE_BUFFERSIZE			(1 << 1)
#define MM_PROBE_CAPS				(1 << 2)
#define MM_PROBE_BUFFER_DURATION 	(1 << 3)
#define MM_PROBE_DROP_BUFFER		(1 << 4)
#define MM_PROBE_CLOCK_TIME			(1 << 5)
/* ... add more */

/* messages are treated as warnings bcz those code should not be checked in. 
 * and no error handling will supported for same manner. 
 */
#define MMPLAYER_ADD_PROBE(x_pad, x_flag) \
debug_warning("adding pad probe\n"); \
if ( ! gst_pad_add_buffer_probe(x_pad, \
	G_CALLBACK(__util_gst_pad_probe), \
	(gpointer)x_flag) ) \
{ \
	debug_error("failed to add pad probe\n"); \
}


/* generating dot */
#define MMPLAYER_GENERATE_DOT_IF_ENABLED( x_player, x_name ) \
if ( PLAYER_INI()->generate_dot ) \
{ \
	debug_log("generating dot file(%s)\n", #x_name); \
	GST_DEBUG_BIN_TO_DOT_FILE (GST_BIN (player->pipeline->mainbin[MMPLAYER_M_PIPE].gst), \
	GST_DEBUG_GRAPH_SHOW_ALL, x_name); \
}

/* signal manipulation */
#define MMPLAYER_SIGNAL_CONNECT( x_player, x_object, x_signal, x_callback, x_arg ) \
do \
{ \
	MMPlayerSignalItem* item = NULL; \
	item = (MMPlayerSignalItem*) g_malloc( sizeof (MMPlayerSignalItem) ); \
	if ( ! item ) \
	{ \
		debug_error("cannot connect signal [%s]\n", x_signal ); \
	} \
	else \
	{ \
		item->obj = G_OBJECT( x_object ); \
		item->sig = g_signal_connect( G_OBJECT(x_object), x_signal, \
					x_callback, x_arg ); \
		x_player->signals = g_list_append(x_player->signals, item); \
	} \
} while ( 0 );


/* state */
#define	MMPLAYER_PREV_STATE(x_player)		((mm_player_t*)x_player)->prev_state 
#define	MMPLAYER_CURRENT_STATE(x_player)		((mm_player_t*)x_player)->state 
#define 	MMPLAYER_PENDING_STATE(x_player)		((mm_player_t*)x_player)->pending_state 
#define 	MMPLAYER_TARGET_STATE(x_player)		((mm_player_t*)x_player)->target_state 
#define 	MMPLAYER_STATE_GET_NAME(state) __get_state_name(state)

#define 	MMPLAYER_PRINT_STATE(x_player) \
debug_log("-----------------------PLAYER STATE-------------------------\n"); \
debug_log(" prev %s, current %s, pending %s, target %s \n", \
	MMPLAYER_STATE_GET_NAME(MMPLAYER_PREV_STATE(x_player)), \
 	MMPLAYER_STATE_GET_NAME(MMPLAYER_CURRENT_STATE(x_player)), \
	MMPLAYER_STATE_GET_NAME(MMPLAYER_PENDING_STATE(x_player)), \
	MMPLAYER_STATE_GET_NAME(MMPLAYER_TARGET_STATE(x_player))); \
debug_log("------------------------------------------------------------\n"); 


#define 	MMPLAYER_STATE_CHANGE_TIMEOUT(x_player )	 ((mm_player_t*)x_player)->state_change_timeout 

/* streaming */
#define MMPLAYER_IS_STREAMING(x_player)  			__is_streaming(x_player)
#define MMPLAYER_IS_RTSP_STREAMING(x_player) 	__is_rtsp_streaming(x_player)
#define MMPLAYER_IS_HTTP_STREAMING(x_player)  	__is_http_streaming(x_player)
#define MMPLAYER_IS_HTTP_PD(x_player)			__is_http_progressive_down(x_player)
#define MMPLAYER_IS_HTTP_LIVE_STREAMING(x_player)  __is_http_live_streaming(x_player)
#define MMPLAYER_IS_LIVE_STREAMING(x_player)  	__is_live_streaming(x_player)

/* etc */
#define	MMF_PLAYER_FILE_BACKUP_PATH		"/tmp/media_temp."
#define 	MMPLAYER_PT_IS_AUDIO( x_pt )		( strstr(x_pt, "_97") || strstr(x_pt, "audio") )
#define 	MMPLAYER_PT_IS_VIDEO( x_pt )		( strstr(x_pt, "_96") || strstr(x_pt, "video") )

bool util_is_sdp_file ( const char *path );
int64_t uti_get_time ( void );
int util_get_rank_increase ( const char *factory_class );
int util_factory_rank_compare(GstPluginFeature *f1, GstPluginFeature *f2); // @


bool util_exist_file_path(const char *file_path);
bool util_write_file_backup(const char *backup_path, char *data_ptr, int data_size);
bool util_remove_file_backup(const char *backup_path); /* For Midi Player */

int util_is_midi_type_by_mem(void *mem, int size);
int util_is_midi_type_by_file(const char *file_path);

char** util_get_cookie_list ( const char *cookies );
bool util_check_valid_url ( const char *proxy );

#ifdef __cplusplus
	}
#endif

#endif /* __MMF_PLAYER_UTILS_H__ */

