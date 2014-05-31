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
/*===========================================================================================
|																							|
|  INCLUDE FILES																			|
|  																							|
========================================================================================== */
//#define MTRACE;
#include <glib.h>
#include <mm_types.h>
#include <mm_error.h>
#include <mm_message.h>
#include<mm_debug.h>
#include <mm_player.h>
#include <mm_sound.h> // set earphone sound path for dnse
#include <iniparser.h>
#include <mm_ta.h>
#include <mm_player_audioeffect.h>
#include <mm_player_internal.h>
#include <pthread.h>
#include <mm_util_imgp.h> // video capture
#include <glib/gprintf.h>       /* g_sprintf */
#ifdef MTRACE
#include <unistd.h> //mtrace
#include <stdlib.h> //mtrace
#endif

#include <dlfcn.h>

#if defined(_USE_EFL)
#include <appcore-efl.h>
//#include <Ecore_X.h>
#include <Elementary.h>
#endif

#include <mm_session.h>

gboolean quit_pushing;

#define PACKAGE "mm_player_testsuite"

/*===========================================================================================
|																							|
|  LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE											|
|  																							|
========================================================================================== */
/*---------------------------------------------------------------------------
|    GLOBAL VARIABLE DEFINITIONS:											|
---------------------------------------------------------------------------*/
char g_file_list[9][256];

#define PCM_DUMP

#ifdef PCM_DUMP
FILE* g_pcm_dump_fp;
#endif
/*---------------------------------------------------------------------------
|    GLOBAL CONSTANT DEFINITIONS:											|
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|    IMPORTED VARIABLE DECLARATIONS:										|
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|    IMPORTED FUNCTION DECLARATIONS:										|
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
|    LOCAL #defines:														|
---------------------------------------------------------------------------*/
#define MAX_STRING_LEN		2048

#ifdef GST_EXT_OVERLAYSINK_WQVGA
#define FULL_WIDTH			240
#define FULL_HEIGHT		180
#define FULL_LCD_WIDTH		240
#define FULL_LCD_HEIGHT	400
#else
#define FULL_WIDTH			320
#define FULL_HEIGHT		240
#define FULL_LCD_WIDTH		320
#define FULL_LCD_HEIGHT	320
#endif

#ifdef _USE_V4L2SINK
#ifndef _MM_PROJECT_VOLANS
#define TES_WIDTH			800
#define TES_HEIGHT			400
#define TES_X				0
#define TES_Y				0
#define TES_VISIBLE		TRUE
#else	// for Volans test
#define TES_WIDTH			400
#define TES_HEIGHT			240
#define TES_X				0
#define TES_Y				0
#define TES_VISIBLE		TRUE
#endif
#endif

#define MM_PLAYER_TS_PD_PATH		"/mnt/ums/pd_test.mp4"

#ifdef PCM_DUMP
#define DUMP_PCM_NAME "/opt/test.pcm"
#endif

#define TS_ROTATE_DEGREE		MM_DISPLAY_ROTATION_270

/* macro */
#define MMTESTSUITE_INI_GET_STRING( x_item, x_ini, x_default ) \
do \
{ \
	gchar* str = iniparser_getstring(dict, x_ini, x_default); \
 \
	if ( str &&  \
		( strlen( str ) > 1 ) && \
		( strlen( str ) < 80 ) ) \
	{ \
		strcpy ( x_item, str ); \
	} \
	else \
	{ \
		strcpy ( x_item, x_default ); \
	} \
}while(0)

#define SET_TOGGLE( flag ) flag = ( flag ) ? 0 : 1;

#define	CHECK_RET_SET_ATTRS(x_err_attrs_name)	\
	if ( x_err_attrs_name )	\
		g_print("failed to set %s", (*x_err_attrs_name)); \
                free(x_err_attrs_name);


#define MMTS_SAMPLELIST_INI_DEFAULT_PATH "/opt/etc/mmts_filelist.ini"
#define MMTS_DEFAULT_INI	\
"\
[list] \n\
\n\
sample1 = /opt/media/Sounds and Music/Music/Over the horizon.mp3\n\
\n\
sample2 = /opt/media/Images and videos/My video clips/Helicopter.mp4\n\
\n\
sample3 = \n\
\n\
sample4 = \n\
\n\
sample5 = \n\
\n\
sample6 = \n\
\n\
sample7 = \n\
\n\
sample8 = \n\
\n\
sample9 = \n\
\n\
"
#define DEFAULT_SAMPLE_PATH ""
#define INI_SAMPLE_LIST_MAX 9

/*---------------------------------------------------------------------------
|    LOCAL CONSTANT DEFINITIONS:											|
---------------------------------------------------------------------------*/
enum
{
	CURRENT_STATUS_MAINMENU = 0,
	CURRENT_STATUS_FILENAME,
	CURRENT_STATUS_FILENAME_NOT_REALIZE,
	CURRENT_STATUS_VOLUME,
	CURRENT_STATUS_POSITION_TIME,
	CURRENT_STATUS_POSITION_PERCENT,
	CURRENT_STATUS_DISPLAYMETHOD,
	CURRENT_STATUS_DISPLAY_VISIBLE,
	CURRENT_STATUS_PLAYCOUNT,
	CURRENT_STATUS_SPEED_PLAYBACK,
	CURRENT_STATUS_SECTION_REPEAT,
#ifdef USE_AUDIO_EFFECT
	CURRENT_STATUS_AUDIO_EFFECT,
	CURRENT_STATUS_AUDIO_EFFECT_EQ,
	CURRENT_STATUS_AUDIO_EFFECT_EXT,
#endif
	CURRENT_STATUS_ADJUST_SUBTITLE_POSITION,
	CURRENT_STATUS_SUBTITLE_FILENAME,
	CURRENT_STATUS_RESIZE_VIDEO,
	CURRENT_STATUS_CHANGE_SURFACE,
};
/*---------------------------------------------------------------------------
|    LOCAL DATA TYPE DEFINITIONS:											|
---------------------------------------------------------------------------*/

#ifdef _USE_EFL

/* for video display */
Ecore_X_Window g_xid;
Evas_Object *g_eo;
int g_current_surface_type;
int g_w;
int g_h;

struct appdata
{
	Evas *evas;
	Ecore_Evas *ee;
	Evas_Object *win;
	Evas_Object *eo;

	Evas_Object *layout_main; /* layout widget based on EDJ */
	Ecore_X_Window xid;

	/* add more variables here */
};

static void win_del(void *data, Evas_Object *obj, void *event)
{
	elm_exit();
}

static Evas_Object* create_win(const char *name)
{
	Evas_Object *eo = NULL;
	int w, h;
	w = h = 0;

	debug_log ("[%s][%d] name=%s\n", __func__, __LINE__, name);

	eo = elm_win_add(NULL, name, ELM_WIN_BASIC);
	if (eo) {
		elm_win_title_set(eo, name);
		elm_win_borderless_set(eo, EINA_TRUE);
		evas_object_smart_callback_add(eo, "delete,request",win_del, NULL);
//		ecore_x_window_size_get(ecore_x_window_root_first_get(),&w, &h);
//		evas_object_resize(eo, w, h);
	}
	g_w = w;
	g_h = h;

	return eo;
}

static int app_create(void *data)
{
	struct appdata *ad = data;
	Evas_Object *win = NULL;

	/* use gl backend */
	elm_config_preferred_engine_set("opengl_x11");

	/* create window */
	win = create_win(PACKAGE);
	if (win == NULL)
		return -1;
	ad->win = win;

	/* Create xwindow for X surface */
	g_xid = elm_win_xwindow_get(ad->win);

	/* Create evas image object for Evas surface */
	ad->evas = evas_object_evas_get(ad->win);

	/* Set evas image object for drawing */
	g_eo = evas_object_image_add(ad->evas);
	evas_object_image_size_set(g_eo, g_w, g_h);
	evas_object_image_fill_set(g_eo, 0, 0, g_w, g_h);
	evas_object_resize(g_eo, g_w, g_h);

	evas_object_show(win);

	return 0;
}

static int app_terminate(void *data)
{
		struct appdata *ad = data;

		if (g_eo) {
			evas_object_del(g_eo);
			g_eo = NULL;
		}
		if (ad->win) {
			evas_object_del(ad->win);
			ad->win = NULL;
		}

		return 0;
}


struct appcore_ops ops = {
		.create = app_create,
		.terminate = app_terminate,
};

//int r;
struct appdata ad;
#endif /* _USE_EFL */


/*---------------------------------------------------------------------------
|    LOCAL VARIABLE DEFINITIONS:											|
---------------------------------------------------------------------------*/

void * overlay = NULL;

int			g_current_state;
int			g_menu_state = CURRENT_STATUS_MAINMENU;
int 			g_memory_playback = 0;
bool			g_bArgPlay = FALSE;
static MMHandleType g_player = 0;
unsigned int		g_video_xid = 0;

/* for audio effect */
int g_custom_effect_type = 0;
int g_eq_band_num = 0;
int g_eq_index = 0;

int g_audio_dsp = FALSE;
int g_video_dsp = FALSE;

char g_subtitle_uri[MAX_STRING_LEN];
int g_subtitle_width = 800;
int g_subtitle_height = 480;
bool g_subtitle_silent = FALSE;
unsigned int g_subtitle_xid = 0;
char *g_err_name = NULL;

/*---------------------------------------------------------------------------
|    LOCAL FUNCTION PROTOTYPES:												|
---------------------------------------------------------------------------*/
static void player_play();
gboolean timeout_quit_program(void* data);
gboolean ts_generate_default_ini(void);
void toggle_audiosink_fadeup();

#ifndef PROTECTOR_VODA_3RD
void TestFileInfo (char* filename);
#endif

bool testsuite_sample_cb(void *stream, int stream_size, void *user_param);

/*===========================================================================================
|																							|
|  FUNCTION DEFINITIONS																		|
|  																							|
========================================================================================== */

/*---------------------------------------------------------------------------
  |    LOCAL FUNCTION DEFINITIONS:											|
  ---------------------------------------------------------------------------*/

static void reset_menu_state()
{
	g_menu_state = 0;
}

gboolean
ts_generate_default_ini(void)
{
	FILE* fp = NULL;
	gchar* default_ini = MMTS_DEFAULT_INI;

	/* create new file */
	fp = fopen(MMTS_SAMPLELIST_INI_DEFAULT_PATH, "wt");

	if ( !fp )
	{
		return FALSE;
	}

	/* writing default ini file */
	if ( strlen(default_ini) != fwrite(default_ini, 1, strlen(default_ini), fp) )
	{
		fclose(fp);
		return FALSE;
	}

	fclose(fp);
	return TRUE;

}

//#define CAPTUERD_IMAGE_SAVE_PATH	"./capture_image"
int	_save(unsigned char * src, int length)
{
	//unlink(CAPTUERD_IMAGE_SAVE_PATH);
	FILE* fp;
	char filename[256] = {0,};
	static int WRITE_COUNT = 0;

	//gchar *filename  = CAPTUERD_IMAGE_SAVE_PATH;

	sprintf (filename, "VIDEO_CAPTURE_IMAGE_%d.rgb", WRITE_COUNT);
	WRITE_COUNT++;

	fp=fopen(filename, "w+");

	if(fp==NULL)
	{
		debug_error("file open error!!\n");
		return FALSE;
	}
	else
	{
		debug_log("open success\n");

		if(fwrite(src, length, 1, fp )!=1)
		{
			debug_error("file write error!!\n");
			fclose(fp);
			return FALSE;
		}
		debug_log("write success(%s)\n", filename);
		fclose(fp);
	}

	return TRUE;
}

static int msg_callback(int message, void *param, void *user_param)
{
	MMMessageParamType *msg_param = (MMMessageParamType *)param;

	switch (message) {
		case MM_MESSAGE_PD_DOWNLOADER_START:
			g_print("                                                            ==> [MMPlayerTestsuite] PD downloader START\n");
			break;
		case MM_MESSAGE_PD_DOWNLOADER_END:
			g_print("                                                            ==> [MMPlayerTestsuite] PD downloader END\n");
			break;
		case MM_MESSAGE_ERROR:
			quit_pushing = TRUE;
			g_print("error : code = %x\n", msg_param->code);
			if (msg_param->code == MM_ERROR_PLAYER_CODEC_NOT_FOUND)
				g_print("##  error string = %s\n", (char *)msg_param->data);
			break;

		case MM_MESSAGE_WARNING:
			// g_print("warning : code = %d\n", param->code);
			break;

		case MM_MESSAGE_END_OF_STREAM:
			g_print("end of stream\n");

			if (g_bArgPlay == TRUE ) {

				g_timeout_add(100, timeout_quit_program, 0);
			}
			break;

		case MM_MESSAGE_STATE_CHANGED:
			g_current_state = msg_param->state.current;

			switch(g_current_state)
			{
				case MM_PLAYER_STATE_NONE:
					g_print("                                                            ==> [MMPlayerTestsuite] Player is [NULL]\n");
					break;
				case MM_PLAYER_STATE_READY:
					g_print("                                                            ==> [MMPlayerTestsuite] Player is [READY]\n");
					break;
				case MM_PLAYER_STATE_PLAYING:
					g_print("                                                            ==> [MMPlayerTestsuite] Player is [PLAYING]\n");
					break;
				case MM_PLAYER_STATE_PAUSED:
					g_print("                                                            ==> [MMPlayerTestsuite] Player is [PAUSED]\n");
					break;
			}
			break;
		case MM_MESSAGE_BEGIN_OF_STREAM:
		{
					g_print("                                                            ==> [MMPlayerTestsuite] BOS\n");
		}
		break;

		case MM_MESSAGE_RESUMED_BY_REW:
			g_print("resumed by fast rewind duing trick play\n");
			break;

		case MM_MESSAGE_VIDEO_CAPTURED:
		{
			/* NOTE : video capture sample
			 * 1. get original video frame as rgb888 in the case of C110 HW Codec
			 * Othrewise, format is I420.
			 * 2. resize it as half size
			 * 3. save resized image
			 *
			 * CAUTION : Application should free received buffer from framework.
			 */
			unsigned char *dst = NULL;
			int src_w;
			int src_h;
			unsigned int dst_width;
			unsigned int dst_height;
			unsigned int  dst_size;
			mm_util_img_format img_fmt;

			MMPlayerVideoCapture* capture = (MMPlayerVideoCapture *)msg_param->data;

			if (mm_player_get_attribute(g_player,
					&g_err_name,
					"content_video_width", &src_w,
					"content_video_height", &src_h,
					NULL) != MM_ERROR_NONE) {
					g_print("failed to get info\n");
					return FALSE;
			}

			dst_width = src_w/2;
			dst_height = src_h/2;

			g_print("video capture src w=%d, h=%d\n", src_w, src_h);
			g_print("video capture dst w=%d, h=%d\n", dst_width, dst_height);

			if (capture->fmt == MM_PLAYER_COLORSPACE_RGB888) //due to ST12
			{
				img_fmt = MM_UTIL_IMG_FMT_RGB888;

				if (mm_util_get_image_size(img_fmt, dst_width, dst_height, &dst_size) != 0)
					return FALSE;

				dst = (unsigned char *)g_malloc(dst_size);
				if (!dst)
					return FALSE;

				if (mm_util_resize_image (capture->data, src_w, src_h, img_fmt, dst, &dst_width, &dst_height) != 0)
					return FALSE;

				_save(dst, dst_size);
			}
			else
			{
				_save(capture->data, capture->size);
			}

			if (capture->data)
			{
				g_free(capture->data);
				capture->data = NULL;
			}

			if (dst)
			{
				g_free(dst);
				dst = NULL;
			}
		}
		break;

		case MM_MESSAGE_SEEK_COMPLETED:
			g_print("                                                            ==> [mm_player_testsuite] SEEK_COMPLETED\n");
			break;

		case MM_MESSAGE_UPDATE_SUBTITLE:
			break;

		default:
			return FALSE;
	}

	return TRUE;
}

bool
testsuite_audio_cb(void *stream, int stream_size, void *user_param)
{
	#ifdef PCM_DUMP
	if(fwrite((char *)stream, stream_size, 1, g_pcm_dump_fp )!=1)
	{
		debug_error("file write error!!\n");
		fclose(g_pcm_dump_fp);
		return FALSE;
	}
	debug_log("write success\n");
	return TRUE;
	#endif
}

bool
testsuite_video_cb(void *stream, int stream_size, void *user_param, int width, int height)
{
	static int count = 0;
	static int org_stream_size;
	int org_w, org_h;

	if (!count)
	{
		org_stream_size = stream_size;

		if (mm_player_get_attribute(g_player, &g_err_name,
								"content_video_width", &org_w,
								"content_video_height", &org_h,
								NULL) == MM_ERROR_NONE) {
			g_print("stream_size = %d[w:%d, h:%d]\n", stream_size, width, height);
			g_print("content width = %d, height = %d\n", org_w, org_h);
			count++;
		} else {
			g_print("failed to get info\n");
			return FALSE;
		}
	}

	if (org_stream_size != stream_size)
	{
		if (mm_player_get_attribute(g_player, &g_err_name,
							"content_video_width", &org_w,
							"content_video_height", &org_h,
							NULL) == MM_ERROR_NONE) {
			g_print("stream_size = %d[w:%d, h:%d]\n", stream_size, width, height);
			g_print("content width = %d, height = %d\n", org_w, org_h);
		} else {
			g_print("failed to get info\n");
			return FALSE;
		}
	}

	return TRUE;
}

static void input_filename(char *filename, bool do_realize)
{
	int len = strlen(filename);
	gchar uri[100]; //<-
	gchar *ext;
	gsize file_size;
	GMappedFile *file;
	GError *error = NULL;
	guint8* g_media_mem = NULL; //-> memory playback

	if ( len < 0 || len > MAX_STRING_LEN )
		return;

	if (g_player)
		mm_player_unrealize(g_player);

	if ( mm_player_create(&g_player) != MM_ERROR_NONE )
	{
		g_print("player create is failed\n");
	}

	mm_player_set_attribute(g_player,
								&g_err_name,
								"subtitle_uri", g_subtitle_uri, strlen(g_subtitle_uri),
								"subtitle_silent", g_subtitle_silent,
								NULL
								);
	strcpy(g_subtitle_uri,"");

	if (g_memory_playback)
	{
		ext = filename;

		file = g_mapped_file_new (ext, FALSE, &error);
		file_size = g_mapped_file_get_length (file);
		g_media_mem = (guint8 *) g_mapped_file_get_contents (file);

		g_sprintf(uri, "mem://ext=%s,size=%d", ext ? ext : "", file_size);
		g_print("[uri] = %s\n", uri);

		mm_player_set_attribute(g_player,
						NULL,
						"profile_uri", uri, strlen(uri),
						"profile_user_param", g_media_mem, file_size,
						NULL);
	}
	else
	{
		mm_player_set_attribute(g_player,
						NULL,
						"profile_uri", filename, strlen(filename),
						NULL);
	}

	/* display surface type and overlay setting */
	mm_player_set_attribute(g_player,
					&g_err_name,
					"display_surface_type", g_current_surface_type,
					NULL);

	if (g_current_surface_type == MM_DISPLAY_SURFACE_X)
		mm_player_set_attribute(g_player, NULL, "display_overlay", &g_xid, sizeof(g_xid), NULL);
	else
		mm_player_set_attribute(g_player, NULL, "display_overlay", g_eo, sizeof(g_eo), NULL);

	mm_player_set_message_callback(g_player, msg_callback, (void*)g_player);

	if (do_realize)
	{
		if (mm_player_realize(g_player) != MM_ERROR_NONE)
		{
			g_print("player realize is failed\n");
		}
	}
}

static void input_subtitle_filename(char *subtitle_filename)
{
	int len = strlen(subtitle_filename);

	if ( len < 1 || len > MAX_STRING_LEN )
		return;

	strcpy (g_subtitle_uri, subtitle_filename);

	g_print("subtitle uri is set to %s\n", g_subtitle_uri);
}

static void toggle_subtitle_silent(bool silent)
{
	if ( mm_player_set_subtitle_silent(g_player, silent) != MM_ERROR_NONE )
	{
		g_print("failed to set subtitle silent\n");
	}
}

static void adjust_subtitle_position(int position)
{
	if ( mm_player_adjust_subtitle_position(g_player, MM_PLAYER_POS_FORMAT_TIME, position) != MM_ERROR_NONE )
	{
		g_print("failed to adjust subtitle position\n");
	}
}


static void set_volume(MMPlayerVolumeType *pvolume)
{
	if ( mm_player_set_volume(g_player, pvolume) != MM_ERROR_NONE )
	{
		g_print("failed to set volume\n");
	}
}


static void get_volume(MMPlayerVolumeType* pvolume)
{
	int i;

	mm_player_get_volume(g_player, pvolume);

	for (i = 0; i < MM_VOLUME_CHANNEL_NUM; i++)
	{
		g_print("                                                            ==> [MediaPlayerApp] channel [%d] = %f\n", i, pvolume->level[i]);
	}
}

#ifdef MTRACE
static gboolean
progress_timer_cb(gpointer u_data)
{
	int format = MM_PLAYER_POS_FORMAT_TIME;
	int position = 0;
	int duration = 0;

	mm_player_get_position(g_player, format, &position);

	if (position >= 10000)
	{
		char str[50];
		pid_t pid = getpid();
		sprintf(str, "memps -t %d", pid);
		g_print ("hyunil pos = [%d], pid = [%d], str=[%s] \n", position, pid,str);
		system (str);
		muntrace();


		return FALSE;
	}

	return TRUE;
}
#endif

static void player_play()
{
	//int bRet = FALSE;

	//bRet = mm_player_start(g_player);
	mm_player_start(g_player);
#ifdef MTRACE
	g_timeout_add( 500,  progress_timer_cb, g_player );
#endif
}

static void player_capture()
{
	if(mm_player_do_video_capture(g_player) != MM_ERROR_NONE)
	{
		debug_warning("failed to capture\n");
	}
}

static void player_stop()
{
	//int bRet = FALSE;

	//bRet = mm_player_stop(g_player);
	mm_player_stop(g_player);
}

static void player_resume()
{
	//int bRet = FALSE;

	//bRet = mm_player_resume(g_player);
	mm_player_resume(g_player);
}

static void player_pause()
{
	//int bRet = FALSE;

	//bRet = mm_player_pause(g_player);
	mm_player_pause(g_player);
}


static void player_rotate()
{
	static int degree = 0;
	degree++;

	if (degree == 4) degree = 0;

	mm_player_set_attribute(g_player, &g_err_name, "display_rotation", degree, NULL);
}


#ifdef MTRACE
//	usleep (1000000);
//	g_print ("aaaaa\n");
//	g_timeout_add( 500,  progress_timer_cb, g_player );
#endif

static void get_position()
{
	int format = MM_PLAYER_POS_FORMAT_TIME;
	int position = 0;
	int duration = 0;

	mm_player_get_position(g_player, format, &position);

	if (mm_player_get_attribute(g_player, &g_err_name, "content_duration", &duration, NULL) != MM_ERROR_NONE) {
		g_print("failed to get info\n");
		return;
	}

	g_print("                                                            ==> [MediaPlayerApp] Pos: [%d / %d] msec\n", position, duration);
}

static void set_position(int position, int format)
{
	if ( mm_player_set_position(g_player, format, position) != MM_ERROR_NONE )
	{
		g_print("failed to set position\n");
	}
}

static void set_display_method(int option)
{
	mm_player_set_attribute(g_player, &g_err_name, "display_method", option, NULL);
}

static void set_display_visible(int option)
{
	mm_player_set_attribute(g_player, &g_err_name, "display_visible", option, NULL);
}

static void resize_video(int option)
{
	int dst_width = 0;
	int dst_height = 0;

	switch (option)
	{
		case 1: //qcif
			dst_width = 176;
			dst_height = 144;
			break;

		case 2: //qvga
			dst_width = 320;
			dst_height = 240;
			break;

		default:
			break;
	}
	mm_player_set_attribute(g_player, &g_err_name, "display_width", dst_width, "display_height", dst_height, NULL);
}

static void change_surface(int option)
{
	int surface_type = 0;
	int ret = MM_ERROR_NONE;

	switch (option)
	{
	case 0: /* X surface */
		surface_type = MM_DISPLAY_SURFACE_X;
		g_print("change surface type to X\n");
		break;
	case 1: /* EVAS surface */
		surface_type = MM_DISPLAY_SURFACE_EVAS;
		g_print("change surface type to EVAS\n");
		break;
	default:
		g_print("invalid surface type\n");
		return;
	}

	if (surface_type == g_current_surface_type)
	{
		g_print("same with the previous surface type(%d)\n", g_current_surface_type);
		return;
	}
	else
	{
		/* state check */
		if (g_current_state == MM_PLAYER_STATE_NULL || g_current_state == MM_PLAYER_STATE_NONE )
		{
			ret = mm_player_set_attribute(g_player, &g_err_name, "display_surface_type", surface_type, NULL);

			if (g_current_surface_type == MM_DISPLAY_SURFACE_X)
				mm_player_set_attribute(g_player, NULL, "display_overlay", &g_xid, sizeof(g_xid), NULL);
			else
				mm_player_set_attribute(g_player, NULL, "display_overlay", g_eo, sizeof(g_eo), NULL);

			if (ret)
			{
				g_print("failed to set attributes\n");
				return;
			}
		}
		else /* if pipeline(include videobin) was already created */
		{
			void *display_overlay = NULL;

			if (surface_type == MM_DISPLAY_SURFACE_X)
				display_overlay = &g_xid;
			else
				display_overlay = g_eo;

			ret = mm_player_change_videosink(g_player, surface_type, display_overlay);
			if (ret)
			{
				if (ret == MM_ERROR_PLAYER_INVALID_STATE && g_current_state == MM_PLAYER_STATE_READY)
				{
					g_print(" >>> Can not progress, player state is READY. You should select [a. Initialize Media] to change display surface type\n");
				}
				else
				{
					g_print("failed to mm_player_change_videosink(), error(%d)\n", ret);
				}
				return;
			}
		}
		if (surface_type == MM_DISPLAY_SURFACE_X)
		{
			evas_object_resize(g_eo, 100, 100); /* this code for hiding the evas image object */
		}
		else
		{
			evas_object_resize(g_eo, g_w, g_h);
		}
		g_current_surface_type = surface_type;
	}
}

void set_audio_callback()
{
	int start_pos = 0;
	int end_pos = 0;
	int samplerate = 0; // Hz

	g_print("start pos?\n");
	if (scanf("%d", &start_pos) == 0)
		return;

	g_print("end pos?\n");
	if (scanf("%d", &end_pos) == 0)
		return;

	g_print("samplerate?\n");
	if (scanf("%d", &samplerate) == 0)
		return;

	mm_player_set_attribute(g_player,
								&g_err_name,
								"pcm_extraction", TRUE,
								"pcm_extraction_start_msec", start_pos,
								"pcm_extraction_end_msec", end_pos,
								"pcm_extraction_samplerate", samplerate,
								NULL);

	mm_player_set_audio_stream_callback(g_player, testsuite_audio_cb, NULL);
}

void set_video_callback()
{
	 mm_player_set_video_stream_callback(g_player, testsuite_video_cb, NULL);
}
#ifdef USE_AUDIO_EFFECT
static int set_audio_effect(char* char_mode)
{
	int effect_type = MM_AUDIO_EFFECT_TYPE_NONE;
	int type = 0;

	if(strncmp(char_mode, "01", 2 ) == 0)
	{
		mm_player_audio_effect_custom_clear_eq_all(g_player);
		mm_player_audio_effect_custom_apply(g_player);
		return 0;
	}
	else if(strncmp(char_mode, "02", 2 ) == 0)
	{
		mm_player_audio_effect_custom_clear_ext_all(g_player);
		mm_player_audio_effect_custom_apply(g_player);
		return 0;
	}
	else if(strncmp(char_mode, "03", 2 ) == 0)
	{
		/* Show width of each EQ band */
		return 2;
	}
	else if(strncmp(char_mode, "04", 2 ) == 0)
	{
		/* Show frequency of each EQ band */
		return 3;
	}
	else if(strncmp(char_mode, "0", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_NONE;
	}
/* Preset */
	else if(strncmp(char_mode, "a", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_PRESET;
		type = MM_AUDIO_EFFECT_PRESET_AUTO;
	}
	else if(strncmp(char_mode, "b", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_PRESET;
		type = MM_AUDIO_EFFECT_PRESET_NORMAL;
	}
	else if(strncmp(char_mode, "c", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_PRESET;
		type = MM_AUDIO_EFFECT_PRESET_POP;
	}
	else if(strncmp(char_mode, "d", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_PRESET;
		type = MM_AUDIO_EFFECT_PRESET_ROCK;
	}
	else if(strncmp(char_mode, "e", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_PRESET;
		type = MM_AUDIO_EFFECT_PRESET_DANCE;
	}
	else if(strncmp(char_mode, "f", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_PRESET;
		type = MM_AUDIO_EFFECT_PRESET_JAZZ;
	}
	else if(strncmp(char_mode, "g", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_PRESET;
		type = MM_AUDIO_EFFECT_PRESET_CLASSIC;
	}
	else if(strncmp(char_mode, "h", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_PRESET;
		type = MM_AUDIO_EFFECT_PRESET_VOCAL;
	}
	else if(strncmp(char_mode, "i", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_PRESET;
		type = MM_AUDIO_EFFECT_PRESET_BASS_BOOST;
	}
	else if(strncmp(char_mode, "j", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_PRESET;
		type = MM_AUDIO_EFFECT_PRESET_TREBLE_BOOST;
	}
	else if(strncmp(char_mode, "k", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_PRESET;
		type = MM_AUDIO_EFFECT_PRESET_MTHEATER;
	}
	else if(strncmp(char_mode, "l", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_PRESET;
		type = MM_AUDIO_EFFECT_PRESET_EXT;
	}
	else if(strncmp(char_mode, "m", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_PRESET;
		type = MM_AUDIO_EFFECT_PRESET_CAFE;
	}
	else if(strncmp(char_mode, "n", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_PRESET;
		type = MM_AUDIO_EFFECT_PRESET_CONCERT_HALL;
	}
	else if(strncmp(char_mode, "o", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_PRESET;
		type = MM_AUDIO_EFFECT_PRESET_VOICE;
	}
	else if(strncmp(char_mode, "p", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_PRESET;
		type = MM_AUDIO_EFFECT_PRESET_MOVIE;
	}
	else if(strncmp(char_mode, "q", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_PRESET;
		type = MM_AUDIO_EFFECT_PRESET_VIRT51;
	}
	else if(strncmp(char_mode, "r", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_PRESET;
		type = MM_AUDIO_EFFECT_PRESET_HIPHOP;
	}
	else if(strncmp(char_mode, "s", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_PRESET;
		type = MM_AUDIO_EFFECT_PRESET_RNB;
	}
	else if(strncmp(char_mode, "t", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_PRESET;
		type = MM_AUDIO_EFFECT_PRESET_FLAT;
	}
	else if(strncmp(char_mode, "u", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_PRESET;
		type = MM_AUDIO_EFFECT_PRESET_TUBE;
	}
/* Custom EQ */
	else if(strncmp(char_mode, "1", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_CUSTOM;
		type = MM_AUDIO_EFFECT_CUSTOM_EQ;
	}
/* Custom Extension */
	else if(strncmp(char_mode, "2", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_CUSTOM;
		type = MM_AUDIO_EFFECT_CUSTOM_3D;
	}
	else if(strncmp(char_mode, "3", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_CUSTOM;
		type = MM_AUDIO_EFFECT_CUSTOM_BASS;
	}
	else if(strncmp(char_mode, "4", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_CUSTOM;
		type = MM_AUDIO_EFFECT_CUSTOM_ROOM_SIZE;
	}
	else if(strncmp(char_mode, "5", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_CUSTOM;
		type = MM_AUDIO_EFFECT_CUSTOM_REVERB_LEVEL;
	}
	else if(strncmp(char_mode, "6", 1 ) == 0)
	{
		effect_type = MM_AUDIO_EFFECT_TYPE_CUSTOM;
		type = MM_AUDIO_EFFECT_CUSTOM_CLARITY;
	}
	else
	{
		return -1;
	}

	/* Apply Sound Effect */
	if (effect_type == MM_AUDIO_EFFECT_TYPE_NONE)
	{
		mm_player_audio_effect_bypass (g_player);
	}
	else if (effect_type == MM_AUDIO_EFFECT_TYPE_PRESET)
	{
		if (type >= MM_AUDIO_EFFECT_PRESET_NORMAL && type <= MM_AUDIO_EFFECT_PRESET_NUM)
		{
			mm_player_audio_effect_preset_apply (g_player, type);
		}
	}
	else if (effect_type == MM_AUDIO_EFFECT_TYPE_CUSTOM)
	{
		if (type == MM_AUDIO_EFFECT_CUSTOM_EQ)
		{
			g_custom_effect_type = type;
			return 1;
		}
		else if (type >= MM_AUDIO_EFFECT_CUSTOM_3D && type < MM_AUDIO_EFFECT_CUSTOM_NUM)
		{
			g_custom_effect_type = type;
			return 4;
		}
	}

	return 0;
}
#endif

static void print_info()
{
#if 0
	MMHandleType tag_prop = 0, content_prop = 0;

	/* set player configuration */
	MMPlayerGetAttrs(g_player, MM_PLAYER_ATTRS_TAG, &tag_prop);
	MMPlayerGetAttrs(g_player, MM_PLAYER_ATTRS_CONTENT, &content_prop);

	if (tag_prop == 0)
		return;

	char* tmp_string = NULL;
	int tmp_string_len = 0;
	int tmp_int = 0;

	debug_log( "====================================================================================\n" );
	MMAttrsGetString (tag_prop, MM_PLAYER_TAG_ARTIST, &tmp_string, &tmp_string_len);
	debug_log("artist: %s\n", tmp_string);

	MMAttrsGetString (tag_prop, MM_PLAYER_TAG_TITLE, &tmp_string, &tmp_string_len);
	debug_log("title: %s\n", tmp_string);

	MMAttrsGetString (tag_prop, MM_PLAYER_TAG_ALBUM, &tmp_string, &tmp_string_len);
	debug_log("album: %s\n", tmp_string);

	MMAttrsGetString (tag_prop, MM_PLAYER_TAG_GENRE, &tmp_string, &tmp_string_len);
	debug_log("genre: %s\n", tmp_string);


	// printf("author: %s\n", mmf_attrs_get_string(tag_prop, MM_FILE_TAG_AUTHOR));

	MMAttrsGetString (tag_prop, MM_PLAYER_TAG_COPYRIGHT, &tmp_string, &tmp_string_len);
	debug_log("copyright: %s\n", tmp_string);

	MMAttrsGetString (tag_prop, MM_PLAYER_TAG_DATE, &tmp_string, &tmp_string_len);
	debug_log("date: %s\n", tmp_string);

	MMAttrsGetString (tag_prop, MM_PLAYER_TAG_DESCRIPTION, &tmp_string, &tmp_string_len);
	debug_log("description: %s\n", tmp_string);

	MMAttrsGetInt (tag_prop, MM_PLAYER_TAG_TRACK_NUM,&tmp_int);
	debug_log("track num: %d\n", tmp_int);



	MMAttrsGetString (content_prop, MM_PLAYER_CONTENT_VIDEO_CODEC, &tmp_string, &tmp_string_len);
	debug_log("video codec: %s\n", tmp_string);

	MMAttrsGetInt (content_prop, MM_PLAYER_CONTENT_AUDIO_TRACK_NUM,&tmp_int);
	debug_log("audio_track_num: %d\n", tmp_int);
	if ( tmp_int > 0 )
    {
		debug_log( "------------------------------------------------------------------------------------\n" );
		MMAttrsGetString (content_prop, MM_PLAYER_CONTENT_AUDIO_CODEC, &tmp_string, &tmp_string_len);
		debug_log("audio codec: %s\n", tmp_string);

		MMAttrsGetInt (content_prop, MM_PLAYER_CONTENT_AUDIO_SAMPLERATE,&tmp_int);
		debug_log("audio samplerate: %d\n", tmp_int);

		MMAttrsGetInt (content_prop, MM_PLAYER_CONTENT_AUDIO_BITRATE,&tmp_int);
		debug_log("audio bitrate: %d\n", tmp_int);

		MMAttrsGetInt (content_prop, MM_PLAYER_CONTENT_AUDIO_CHANNELS,&tmp_int);
		debug_log("audio channel: %d\n", tmp_int);

		MMAttrsGetInt (content_prop, MM_PLAYER_CONTENT_AUDIO_TRACK_ID,&tmp_int);
		debug_log("audio track id: %d\n", tmp_int);
		debug_log( "------------------------------------------------------------------------------------\n" );
	}


	MMAttrsGetInt (content_prop, MM_PLAYER_CONTENT_VIDEO_TRACK_NUM,&tmp_int);
	debug_log("video_track_num: %d\n", tmp_int);
	if ( tmp_int > 0 )
	{
		debug_log( "------------------------------------------------------------------------------------\n" );
		MMAttrsGetString (content_prop, MM_PLAYER_CONTENT_VIDEO_CODEC, &tmp_string, &tmp_string_len);
		debug_log("video codec: %s\n", tmp_string);

		MMAttrsGetInt (content_prop, MM_PLAYER_CONTENT_VIDEO_WIDTH,&tmp_int);
		debug_log("video width: %d\n", tmp_int);

		MMAttrsGetInt (content_prop, MM_PLAYER_CONTENT_VIDEO_HEIGHT,&tmp_int);
		debug_log("video height: %d\n", tmp_int);

		MMAttrsGetInt (content_prop, MM_PLAYER_CONTENT_VIDEO_FPS,&tmp_int);
		debug_log("video fps: %d\n", tmp_int);

		MMAttrsGetInt (content_prop, MM_PLAYER_CONTENT_VIDEO_TRACK_ID,&tmp_int);
		debug_log("video track id: %d\n", tmp_int);
		debug_log( "------------------------------------------------------------------------------------\n" );
	}
	debug_log( "====================================================================================\n" );
	debug_log("Time analysis...\n");

	//MMTA_ACUM_ITEM_SHOW_RESULT();
#endif
}

void quit_program()
{
	MMTA_ACUM_ITEM_SHOW_RESULT_TO(MMTA_SHOW_STDOUT);
	MMTA_RELEASE();

	mm_player_unrealize(g_player);
	mm_player_destroy(g_player);
	g_player = 0;

#ifdef _USE_EFL
	elm_exit();
#endif
}

void destroy_player()
{
        mm_player_unrealize(g_player);
        mm_player_destroy(g_player);
        g_player = 0;
	g_print("player is destroyed.\n");
}

void display_sub_additional()
{

}

void display_sub_basic()
{
	int idx;

	g_print("\n");
	g_print("=========================================================================================\n");
	g_print("                          MM-PLAYER Testsuite (press q to quit) \n");
	g_print("-----------------------------------------------------------------------------------------\n");
	g_print("*. Sample List in [%s]\n", MMTS_SAMPLELIST_INI_DEFAULT_PATH);

	for( idx = 1; idx <= INI_SAMPLE_LIST_MAX ; idx++ )
	{
		if (strlen (g_file_list[idx-1]) > 0)
			g_print("%d. Play [%s]\n", idx, g_file_list[idx-1]);
	}

	g_print("-----------------------------------------------------------------------------------------\n");
	g_print("[playback] a. Initialize Media\t");
	g_print("b. Play  \t");
	g_print("c. Stop  \t");
	g_print("d. Resume\t");
	g_print("e. Pause \n");
	g_print("           aa. Create\t\t");
	g_print("ab. Realize\n");

	g_print("[ volume ] f. Set Volume\t");
	g_print("g. Get Volume\n");

	g_print("[position] h. Set Position (T)\t");
	g_print("i. Get Position\t");
	g_print("r. Set Position (%%)\n");

	g_print("[ preset ] o. Toggle Audio DSP\t");
	g_print("p. Toggle Video DSP\t");
	g_print("m. Set Play Count\n");

	g_print("[ option ] sp. Speed Playback \t");
	g_print("sx. Speed Playback ex \t");
	g_print("sr. Togle Section Play\n");

	g_print("[display ] x. Set geometry method\t");
	g_print("dv. display visible\t");
	g_print("rv. resize video - fimcconvert only\n");
	g_print("           ds. Change display surface type\n");

	g_print("[ sound  ] k. Toggle Fadeup\t");
	g_print("z. Apply Sound Effect\t\n");
	g_print("           sm. Set Session Mix\t");
	g_print("sd. Set Session Pre-listening mode\n");
#if 0	// not available now
  	g_print("        Progressive Download\n");
  	g_print("----------------------\n");
	g_print("!. set Progressive Download enable \n");
	g_print("@. set full-contents-size  \n");
	g_print("#. set Download Complete() \n");
	g_print("----------------------\n");
#endif

	g_print("[subtitle] $. Set subtitle uri\t");
	g_print("%%. Toggle subtitle silent  \t");
	g_print("<. Adjust subtitle postion  \n");

	g_print("[   etc  ] j. Information\t");
	g_print("l. Buffering Mode\n");
	g_print("[ preset ] mp. memory playback\n");
	g_print("[callback] ac. audio stream callback(segment extraction)\t");
	g_print("vc. video stream callback\n");

	g_print("[  video ] ca. capture\t");
	g_print("ro. rotate\n");
	g_print("[ PD ] pd. Set Progressive Download \n");
	g_print("cl [Language audio change] - to change audio language \n");
	g_print("tc [Track count] - to get total audio tracks \n");
	g_print("lc [Language code] - to get audio language code \n");
	g_print("ct [current audio track] - to current audio track \n");
	g_print("\n");
	g_print("=========================================================================================\n");
}

static bool display_current_effect_callback(int effect_type, int type, void *user)
{
	g_print("%d ", type);
	return TRUE;
}

void display_sub_audio_effect()
{
	g_print("*********** Choose Sound Effect You Want ***********\n");
	g_print("*** current supported Preset effect(index of array)\n");
	g_print(">>> ");
	mm_player_get_foreach_present_supported_effect_type(g_player, MM_AUDIO_EFFECT_TYPE_PRESET, display_current_effect_callback, NULL);
	g_print("\n*** current supported Custom effect(index of array)\n");
	g_print(">>> ");
	mm_player_get_foreach_present_supported_effect_type(g_player, MM_AUDIO_EFFECT_TYPE_CUSTOM, display_current_effect_callback, NULL);
	g_print("\n*** NOTE: Custom effect can be mixed together\n");
	g_print("----------------------------------------------------\n");
	g_print("0.  No Sound Effect\n");
	g_print("01. Clear All EQ Bands\n");
	g_print("02. Clear All Extension Effects\n");
	g_print("03. Show width of each EQ band\n");
	g_print("04. Show frequency of each EQ band\n");
	g_print("Preset----------------------------------------------\n");
	g_print("a. Auto\n");
	g_print("b. Normal\n");
	g_print("c. Pop\n");
	g_print("d. Rock\n");
	g_print("e. Dance\n");
	g_print("f. Jazz\n");
	g_print("g. Classic\n");
	g_print("h. Vocal\n");
	g_print("i. Bass Boost\n");
	g_print("j. Treble Boost\n");
	g_print("k. MThreater\n");
	g_print("l. Externalization\n");
	g_print("m. Cafe\n");
	g_print("n. Concert Hall\n");
	g_print("o. Voice\n");
	g_print("p. Movie\n");
	g_print("q. Virtual5.1\n");
	g_print("r. HipHop\n");
	g_print("s. R&B\n");
	g_print("t. Flat\n");
	g_print("u. Tube\n");
	g_print("Custom-Equalizer------------------------------------\n");
	g_print("1. EQ\n");
	g_print("Custom-Extension------------------------------------\n");
	g_print("2. 3D\n");
	g_print("3. Bass\n");
	g_print("4. Room Size\n");
	g_print("5. Reverb Level\n");
	g_print("6. Clarity\n");
	g_print("----------------------------------------------------\n");
}


static void displaymenu()
{
	if (g_menu_state == CURRENT_STATUS_MAINMENU)
	{
		display_sub_basic();
	}
	else if (g_menu_state == CURRENT_STATUS_FILENAME || g_menu_state == CURRENT_STATUS_FILENAME_NOT_REALIZE )
	{
		g_print("*** input mediapath.\n");
	}
	else if (g_menu_state == CURRENT_STATUS_VOLUME)
	{
		g_print("*** input volume value.\n");
	}
	else if (g_menu_state == CURRENT_STATUS_PLAYCOUNT)
	{
		g_print("*** input count num.\n");
	}
	else if (g_menu_state == CURRENT_STATUS_POSITION_TIME)
	{
		g_print("*** input position value(msec)\n");
	}
	else if (g_menu_state == CURRENT_STATUS_POSITION_PERCENT)
	{
		g_print("*** input position percent(%%)\n");
	}
	else if ( g_menu_state == CURRENT_STATUS_DISPLAYMETHOD)
	{
		g_print("*** input option(0:LETTER BOX, 1:ORIGIN SIZE, 2:FULL SCREEN, 3:CROPPED FULL, 4:ORIGIN_OR_LETTER)\n");
	}
	else if ( g_menu_state == CURRENT_STATUS_DISPLAY_VISIBLE)
	{
		g_print("*** input value(0,1)\n");
	}
	else if ( g_menu_state == CURRENT_STATUS_RESIZE_VIDEO)
	{
		g_print("*** input one (1:QCIF, 2:QVGA)\n");
	}
	else if ( g_menu_state == CURRENT_STATUS_CHANGE_SURFACE)
	{
		g_print("*** input one (0:X surface, 1:EVAS surface)\n");
	}
	else if (g_menu_state == CURRENT_STATUS_SPEED_PLAYBACK)
	{
		g_print("*** input playback rate.\n");
	}
	else if (g_menu_state == CURRENT_STATUS_PLAYCOUNT)
	{

	}
#ifdef USE_AUDIO_EFFECT
	else if (g_menu_state == CURRENT_STATUS_AUDIO_EFFECT)
	{
		display_sub_audio_effect();
	}
	else if (g_menu_state == CURRENT_STATUS_AUDIO_EFFECT_EQ)
	{
		int min = 0;
		int max = 0;
		int ret = 0;

		/* get number of eq bands */
		if (g_eq_index == 0)
		{
			ret = mm_player_audio_effect_custom_get_eq_bands_number(g_player, &g_eq_band_num);
			if (ret)
			{
				g_print("getting number of EQ band failed\n");
				reset_menu_state();
			}
			else
			{
				g_print("*** number of EQ band : %d\n", g_eq_band_num);
			}

			/* get min, max value of level */
			ret = mm_player_audio_effect_custom_get_level_range(g_player, MM_AUDIO_EFFECT_CUSTOM_EQ, &min, &max);
			if (ret)
			{
				g_print("getting range of level failed\n");
				reset_menu_state();
			}
			else
			{
				g_print("*** EQ level range : %d(MIN) - %d(MAX)\n", min, max);
			}
		}
		g_print("*** input band(%d) level of Equalizer\n",g_eq_index);
	}
	else if (g_menu_state == CURRENT_STATUS_AUDIO_EFFECT_EXT)
	{
		int min = 0;
		int max = 0;
		int ret = 0;

		ret = mm_player_audio_effect_custom_get_level_range(g_player, g_custom_effect_type, &min, &max);
		if (ret)
		{
			g_print("effect type(%d).getting range of level failed\n", g_custom_effect_type);
			reset_menu_state();
		}
		else
		{
			g_print("*** effect type(%d). level range : %d(MIN) - %d(MAX)\n", g_custom_effect_type, min, max);
		}
		g_print("*** input level of custom extension effect(%d)\n",g_custom_effect_type);
	}
#endif
	else if (g_menu_state == CURRENT_STATUS_SUBTITLE_FILENAME)
	{
		g_print(" ** input  subtitle file path.\n");
	}
	else if (g_menu_state == CURRENT_STATUS_ADJUST_SUBTITLE_POSITION)
	{
		g_print("*** input adjusted value(msec)\n");
	}
	else
	{
		g_print("*** unknown status.\n");
		quit_program();
	}
	g_print(" >>> ");
}

gboolean timeout_menu_display(void* data)
{
	displaymenu();
	return FALSE;
}

gboolean timeout_quit_program(void* data)
{
	quit_program();
	return FALSE;
}

void set_playcount(int count)
{
	mm_player_set_attribute(g_player, &g_err_name, "profile_play_count", count, NULL);
}

void toggle_audiosink_fadeup()
{
	static gboolean flag_fadeup = FALSE;

	SET_TOGGLE(flag_fadeup);

	g_print("fadeup value to set : %d\n", flag_fadeup);

	mm_player_set_attribute(g_player, &g_err_name, "no_sound_output", flag_fadeup, NULL);

}

void toggle_sound_spk_out_only()
{
	static gboolean flag_spk_out = FALSE;

	SET_TOGGLE(flag_spk_out);

	g_print("flag_spk_out value to set : %d\n", flag_spk_out);

	mm_player_set_attribute(g_player, &g_err_name, "sound_spk_out_only", flag_spk_out, NULL);

}

void toggle_section_repeat(void)
{
	static gboolean flag_sr = FALSE;
	int pos;
	int offset = 4000;

	SET_TOGGLE(flag_sr);

	if ( flag_sr )
	{
		g_print("section repeat activated\n");

		mm_player_get_position(g_player, MM_PLAYER_POS_FORMAT_TIME, &pos);

		mm_player_activate_section_repeat(g_player, pos, pos+offset);
	}
	else
	{
		g_print("section repeat deactivated\n");

		mm_player_deactivate_section_repeat(g_player);
	}
}

void play_with_ini(char *file_path)
		{
	input_filename(file_path, true);
				player_play();
}

gboolean start_pushing_buffers;
guint64 offset;
guint size_to_be_pushed;

gboolean seek_flag;

char filename_push[256];

void enough_data(void *player)
{
	g_print("__enough_data\n");
	start_pushing_buffers = FALSE;

}

void set_pd_test()
{
	mm_player_set_attribute(g_player, &g_err_name, "pd_mode", MM_PLAYER_PD_MODE_URI, NULL);
	mm_player_set_attribute(g_player, &g_err_name, "pd_location", MM_PLAYER_TS_PD_PATH, strlen(MM_PLAYER_TS_PD_PATH), NULL);
}

void feed_data(guint size, void *player)
{
	g_print("__feed_data:%d\n", size);
	start_pushing_buffers = TRUE;
	size_to_be_pushed = size;
}
void seek_data(guint64 _offset, void *player)
{
	g_print("__seek_data:%lld\n", _offset);
	start_pushing_buffers = TRUE;
	offset=_offset;
	seek_flag=TRUE;
}

void* push_buffer()
{
	FILE *fp=fopen(filename_push, "rb");
	unsigned char *buf=NULL;
	int size;
	guint64 read_position=0;
	//int err;
	g_print("filename:%s\n", filename_push);
	if(fp==NULL)
	{
		g_print("not a valid filename\n");
		quit_program();
		return NULL;
	}

	seek_flag=FALSE;

	while(!quit_pushing)
	{
		if(!start_pushing_buffers)
		{
			usleep(10000);
			continue;
		}

		//g_print("trying to push\n");
            if (read_position != offset && seek_flag==TRUE)
		{
	            //guint64 res;

		    //res = fseek (fp, offset, SEEK_SET);
	            fseek (fp, offset, SEEK_SET);

	            read_position = offset;

			g_print("seeking to %lld\n", offset);
            }

		seek_flag=FALSE;

		if(size_to_be_pushed)
			size = size_to_be_pushed;
		else
			size = 4096;

		buf = (unsigned char *)malloc(size);
		if(buf==NULL)
		{
			g_print("mem alloc failed\n");
			break;
		}
		size  = fread(buf, 1, size, fp);

		if(size<=0)
		{
			g_print("EOS\n");
			start_pushing_buffers=FALSE;
			mm_player_push_buffer((MMHandleType)g_player, buf, size);
			break;
//			continue;
		}
		//g_print("pushing:%d\n", size);
		read_position += size;
		size_to_be_pushed=0;
		mm_player_push_buffer((MMHandleType)g_player, buf, size);
		//usleep(10000);
	}
	fclose(fp);

	return NULL;
}

void _interpret_main_menu(char *cmd)
{
	if ( strlen(cmd) == 1 )
	{
		if (strncmp(cmd, "a", 1) == 0)
		{
			g_menu_state = CURRENT_STATUS_FILENAME;
		}
		else if (strncmp(cmd, "l", 1) == 0)
		{
			//play_with_ini(g_file_list[0]);
				input_filename("buff://", true);
				mm_player_set_buffer_need_data_callback(g_player, (mm_player_buffer_need_data_callback )(feed_data), NULL);
				mm_player_set_buffer_seek_data_callback(g_player, (mm_player_buffer_seek_data_callback )(seek_data), NULL);
				mm_player_set_buffer_enough_data_callback(g_player, (mm_player_buffer_enough_data_callback )(enough_data), NULL);
				pthread_t tid1;
				debug_log("provide the filename:");
				if (scanf("%s", filename_push) != 0)
				{
					pthread_create(&tid1,NULL,push_buffer,NULL);
					//	sleep(2);
					player_play();
				}
		}
		else if (strncmp(cmd, "1", 1) == 0)
		{
			play_with_ini(g_file_list[0]);
		}
		else if (strncmp(cmd, "2", 1) == 0)
		{
			play_with_ini(g_file_list[1]);
		}
		else if (strncmp(cmd, "3", 1) == 0)
		{
			play_with_ini(g_file_list[2]);
		}
		else if (strncmp(cmd, "4", 1) == 0)
		{
			play_with_ini(g_file_list[3]);
		}
		else if (strncmp(cmd, "5", 1) == 0)
		{
			play_with_ini(g_file_list[4]);
		}
		else if (strncmp(cmd, "6", 1) == 0)
		{
			play_with_ini(g_file_list[5]);
		}
		else if (strncmp(cmd, "7", 1) == 0)
		{
			play_with_ini(g_file_list[6]);
		}
		else if (strncmp(cmd, "8", 1) == 0)
		{
			play_with_ini(g_file_list[7]);
		}
		else if (strncmp(cmd, "9", 1) == 0)
		{
			play_with_ini(g_file_list[8]);
		}
		else if (strncmp(cmd, "b", 1) == 0)
		{
				player_play();
		}
		else if (strncmp(cmd, "c", 1) == 0)
		{
				player_stop();
		}
		else if (strncmp(cmd, "d", 1) == 0)
		{
				player_resume();
		}
		else if (strncmp(cmd, "e", 1) == 0)
		{
				player_pause();
		}
		else if (strncmp(cmd, "f", 1) == 0)
		{
				g_menu_state = CURRENT_STATUS_VOLUME;
		}
		else if (strncmp(cmd, "g", 1) == 0)
		{
			MMPlayerVolumeType volume;
			memset (volume.level, 0,  MM_VOLUME_CHANNEL_NUM);
			get_volume(&volume);
		}
		else if (strncmp(cmd, "h", 1) == 0 )
		{
				g_menu_state = CURRENT_STATUS_POSITION_TIME;
		}
		else if (strncmp(cmd, "r", 1) == 0 )
		{
				g_menu_state = CURRENT_STATUS_POSITION_PERCENT;
		}
		else if (strncmp(cmd, "i", 1) == 0 )
		{
				get_position();
		}
		else if (strncmp(cmd, "j", 1) == 0)
		{
			 	print_info();
		}
		else if (strncmp(cmd, "o", 1) == 0)
		{
				gboolean old = g_audio_dsp;
				if (old)
					g_audio_dsp =0;
				else
					g_audio_dsp =1;

			 	g_print (">> g_audio_dsp = [%d] => [%d]\n", old, g_audio_dsp);
		}
		else if (strncmp(cmd, "p", 1) == 0)
		{
				gboolean old = g_video_dsp;
				if (old)
					g_video_dsp =0;
				else
					g_video_dsp =1;

			 	g_print (">> g_video_dsp = [%d] => [%d]\n", old, g_video_dsp);
		}
		else if ( strncmp(cmd, "k", 1) == 0 )
		{
				toggle_audiosink_fadeup();
		}
		else if (strncmp(cmd, "m", 1) == 0)
		{
				g_menu_state = CURRENT_STATUS_PLAYCOUNT;
		}
		else if (strncmp (cmd, "x", 1) == 0)
		{
				g_menu_state = CURRENT_STATUS_DISPLAYMETHOD;
		}
#ifdef USE_AUDIO_EFFECT
		else if (strncmp (cmd, "z", 1) == 0)
		{
				g_menu_state = CURRENT_STATUS_AUDIO_EFFECT;
		}
#endif
		else if (strncmp(cmd, "u", 1) == 0)
		{
				destroy_player();
		}
		else if (strncmp(cmd, "q", 1) == 0)
		{
				quit_pushing = TRUE;
				quit_program();
		}
		else if (strncmp(cmd, "[", 1) == 0)
		{
			toggle_sound_spk_out_only ();
		}
		else if (strncmp(cmd, "{", 1) == 0)
		{
			g_print ("mm_session_init(MM_SESSION_TYPE_SHARE) = %d\n", mm_session_init (MM_SESSION_TYPE_SHARE));
		}
		else if (strncmp(cmd, "}", 1) == 0)
		{
			g_print ("mm_session_init(MM_SESSION_TYPE_EXCLUSIVE) = %d\n", mm_session_init (MM_SESSION_TYPE_EXCLUSIVE));
		}
		else if(strncmp(cmd, "$", 1) == 0)
		{
			g_menu_state=CURRENT_STATUS_SUBTITLE_FILENAME;
		}
		else if(strncmp(cmd, "%", 1) == 0)
		{
			if (g_subtitle_silent)
				g_subtitle_silent = FALSE;
			else
				g_subtitle_silent = TRUE;

			toggle_subtitle_silent (g_subtitle_silent);

			reset_menu_state();
		}
		else if(strncmp(cmd, "<", 1) == 0)
		{
				g_menu_state = CURRENT_STATUS_ADJUST_SUBTITLE_POSITION;
		}
		else
		{
				g_print("unknown menu \n");
		}
	}
	else
	{
		if (strncmp(cmd, "aa", 2) == 0)
		{
			g_menu_state = CURRENT_STATUS_FILENAME_NOT_REALIZE;
		}
		else if (strncmp(cmd, "ab", 2) == 0)
		{
			if (mm_player_realize(g_player) != MM_ERROR_NONE)
			{
				g_print("player realize is failed\n");
			}
		}
		else if (strncmp(cmd, "sp", 2) == 0)
		{
			g_menu_state = CURRENT_STATUS_SPEED_PLAYBACK;
		}
		else if (strncmp(cmd, "mp", 2) == 0)
		{
			SET_TOGGLE(g_memory_playback);
		}
		else if (strncmp(cmd, "ca", 2) == 0)
		{
			player_capture();
		}
		else if (strncmp(cmd, "ro", 2) == 0)
		{
			player_rotate();
		}
		else if (strncmp(cmd, "sr", 2) == 0)
		{
			toggle_section_repeat();
		}
		else if (strncmp(cmd, "ac", 2) == 0)
		{
			/* It should be called before starting play. */
			set_audio_callback();
		}
		else if (strncmp(cmd, "vc", 2) == 0)
		{
			set_video_callback();
		}
		else if (strncmp(cmd, "pd", 2) == 0)
		{
			set_pd_test();
		}
		else if (strncmp (cmd, "dv", 2) == 0)
		{
			g_menu_state = CURRENT_STATUS_DISPLAY_VISIBLE;
		}
		else if (strncmp (cmd, "rv", 2) == 0)
		{
			g_menu_state = CURRENT_STATUS_RESIZE_VIDEO;
		}
		else if (strncmp (cmd, "ds", 2) == 0)
		{
			g_menu_state = CURRENT_STATUS_CHANGE_SURFACE;
		}
		else if (strncmp(cmd, "help", 4) == 0)
		{
			g_timeout_add(100, timeout_menu_display, 0);
		}
 }
}

static void interpret (char *cmd)
{
	switch (g_menu_state)
	{
		case CURRENT_STATUS_MAINMENU:
		{
			_interpret_main_menu(cmd);
		}
			break;

		case CURRENT_STATUS_FILENAME:
		{
			input_filename(cmd, true);

			reset_menu_state();
		}
			break;
		case CURRENT_STATUS_FILENAME_NOT_REALIZE:
		{
			input_filename(cmd, false);

			reset_menu_state();
		}
			break;

		case CURRENT_STATUS_VOLUME:
			{
			MMPlayerVolumeType volume;
				int i;
				float level = atof(cmd);

				g_print ("level = [%f]\n", level);

				for(i=0; i<MM_VOLUME_CHANNEL_NUM; i++)
					volume.level[i] = (gfloat)level;

				set_volume(&volume);

				reset_menu_state();
			}
			break;

		case CURRENT_STATUS_SPEED_PLAYBACK:
		{
			float rate = atof(cmd);

			g_print("playback rate = %f", rate);

			mm_player_set_play_speed(g_player, rate);

			reset_menu_state();
		}
		break;

		case CURRENT_STATUS_POSITION_TIME:
		{
			unsigned long position = atol(cmd);
			set_position(position, MM_PLAYER_POS_FORMAT_TIME);

			reset_menu_state();
		}
		break;

		case CURRENT_STATUS_POSITION_PERCENT:
		{
			unsigned long position = atol(cmd);
			set_position(position, MM_PLAYER_POS_FORMAT_PERCENT);

			reset_menu_state();
		}
		break;

		case CURRENT_STATUS_DISPLAYMETHOD:
		{
			unsigned long option = atol(cmd);
			set_display_method(option);

			reset_menu_state();
		}
		break;

		case CURRENT_STATUS_DISPLAY_VISIBLE:
		{
			unsigned long option = atol(cmd);
			set_display_visible(option);
			reset_menu_state();
		}
		break;

		case CURRENT_STATUS_RESIZE_VIDEO:
		{
			unsigned int option = atoi(cmd);
			resize_video(option);
			reset_menu_state();
		}
		break;

		case CURRENT_STATUS_CHANGE_SURFACE:
		{
			unsigned int option = atoi(cmd);
			change_surface(option);
			reset_menu_state();
		}
		break;

#ifdef USE_AUDIO_EFFECT
		case CURRENT_STATUS_AUDIO_EFFECT:
		{
			int ret = 0;
			ret = set_audio_effect(cmd);

			if (ret == -1)
			{
				g_print("unknown menu - Sound Effect\n");
				reset_menu_state();
			}
			else if (ret==0)
			{
				reset_menu_state();
			}
			else if (ret==1) /* custom EQ */
			{
				g_menu_state = CURRENT_STATUS_AUDIO_EFFECT_EQ;
			}
			else if (ret==2) /* Show width of each EQ band */
			{
				int ret = 0;
				int i = 0;
				int width = 0;

				/* get number of eq bands */
				if (g_eq_band_num == 0)
				{
					ret = mm_player_audio_effect_custom_get_eq_bands_number(g_player, &g_eq_band_num);
					if (ret)
					{
						g_print("getting number of EQ band failed\n");
						reset_menu_state();
					}
					else
					{
						g_print("*** number of EQ band : %d\n", g_eq_band_num);
					}
				}
				g_print("*** list of EQ band width(Hz) : ");
				for (i = 0; i < g_eq_band_num; i++)
				{
					ret = mm_player_audio_effect_custom_get_eq_bands_width(g_player, i, &width);
					if (ret)
					{
						g_print("getting width of EQ band(%d) failed\n", i);
						break;
					}
					else
					{
						g_print("%d ", width);
					}
				}
				g_print("\n\n");
				reset_menu_state();
			}
			else if (ret==3) /* Show frequency of each EQ band */
			{
				int ret = 0;
				int i = 0;
				int freq = 0;

				/* get number of eq bands */
				if (g_eq_band_num == 0)
				{
					ret = mm_player_audio_effect_custom_get_eq_bands_number(g_player, &g_eq_band_num);
					if (ret)
					{
						g_print("getting number of EQ band failed\n");
						reset_menu_state();
					}
					else
					{
						g_print("*** number of EQ band : %d\n", g_eq_band_num);
					}
				}
				g_print("*** list of EQ band frequency(Hz) : ");
				for (i = 0; i < g_eq_band_num; i++)
				{
					ret = mm_player_audio_effect_custom_get_eq_bands_freq(g_player, i, &freq);
					if (ret)
					{
						g_print("getting frequency of EQ band(%d) failed\n", i);
						break;
					}
					else
					{
						g_print("%d ", freq);
					}
				}
				g_print("\n\n");
				reset_menu_state();
			}
			else if (ret==4) /* custom extension effect */
			{
				g_menu_state = CURRENT_STATUS_AUDIO_EFFECT_EXT;
			}
		}
		break;

		case CURRENT_STATUS_AUDIO_EFFECT_EQ:
		{
			int ret = 0;
			int level = 0;

			level = atoi(cmd);
			ret = mm_player_audio_effect_custom_set_level(g_player, MM_AUDIO_EFFECT_CUSTOM_EQ, g_eq_index, level);
			if (ret)
			{
				g_print("mm_player_audio_effect_custom_set_level() failed\n");
				g_eq_index = 1;
				reset_menu_state();
			}
			g_eq_index++;
			if(g_eq_index == g_eq_band_num)
			{
#if 0
				/* this code is for testing _set_level_eq_from_list() */
				int array[8] = {3,3,3,3,3,3,3,3};
				mm_player_audio_effect_custom_set_level_eq_from_list(g_player, array, 8);
#endif
				ret = mm_player_audio_effect_custom_apply(g_player);
				if (ret)
				{
					g_print("mm_player_audio_effect_custom_apply() failed\n");
				}
				g_eq_index = 0;
				reset_menu_state();
			}
		}
		break;

		case CURRENT_STATUS_AUDIO_EFFECT_EXT:
		{
			int ret = 0;
			int level = 0;

			level = atoi(cmd);
			ret = mm_player_audio_effect_custom_set_level(g_player, g_custom_effect_type, 0, level);
			if (ret)
			{
				g_print("mm_player_audio_effect_custom_set_level() failed\n");
			}
			mm_player_audio_effect_custom_apply(g_player);
			reset_menu_state();
		}
		break;
#endif
		case CURRENT_STATUS_PLAYCOUNT:
			{
				int count = atoi(cmd);
				set_playcount(count);

			reset_menu_state();
			}
			break;

		case CURRENT_STATUS_SUBTITLE_FILENAME:
		{
			input_subtitle_filename(cmd);

			reset_menu_state();
		}
		break;

		case CURRENT_STATUS_ADJUST_SUBTITLE_POSITION:
		{
			int position = atol(cmd);
			 g_print("\n ------------------ %d \n", position);
			adjust_subtitle_position(position);

			reset_menu_state();
		}
		break;

	}

	g_timeout_add(100, timeout_menu_display, 0);
}

gboolean input (GIOChannel *channel)
{
	char buf[MAX_STRING_LEN + 3];
	gsize read;
	GError *error = NULL;

	g_io_channel_read_chars(channel, buf, MAX_STRING_LEN, &read, &error);
	buf[read] = '\0';
	g_strstrip(buf);
	interpret (buf);
	return TRUE;
}

/* format:
 mm_player_testsuite [media-filename [-NUM]]
 NUM is the number of iterations to perform */
int main(int argc, char *argv[])
{
		GIOChannel *stdin_channel;
		MMTA_INIT();

#ifdef MTRACE
		mtrace();
		MMHandleType prop;
		GError *error = NULL;
#endif

		stdin_channel = g_io_channel_unix_new(0);
		g_io_channel_set_flags (stdin_channel, G_IO_FLAG_NONBLOCK, NULL);
		g_io_add_watch(stdin_channel, G_IO_IN, (GIOFunc)input, NULL);

		#ifdef PCM_DUMP
		g_pcm_dump_fp = fopen(DUMP_PCM_NAME, "w+");
		#endif

#ifdef __arm__
		// initialize sound path for using ear
#if 0
		ret = MMSoundSetPathEx2(MM_SOUND_GAIN_KEYTONE, MM_SOUND_PATH_SPK, MM_SOUND_PATH_NONE, MM_SOUND_PATH_OPTION_AUTO_HEADSET_CONTROL);

		if(ret < 0)
		{
				debug_error("path error\n");
				return -1;
		}
#endif

#endif /* __arm__ */
		displaymenu();

		memset(&ad, 0x0, sizeof(struct appdata));
		ops.data = &ad;

		return appcore_efl_main(PACKAGE, &argc, &argv, &ops);
}

